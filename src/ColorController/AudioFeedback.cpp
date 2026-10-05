#include "AudioFeedback.h"

/**
 * @file AudioFeedback.cpp
 * @brief Exclusive ES8311/I2S owner for microphone sampling and feedback tones.
 *
 * All codec and I2S access occurs in the audio coroutine after setup has
 * initialized the shared touch I2C bus. Feedback uses a complete preloaded DMA
 * waveform, avoiding scheduler jitter and TX underrun artifacts. Microphone
 * samples use latest-value semantics because old amplitude data has no value to
 * a real-time lighting effect.
 */

#include <Arduino.h>
#include <math.h>

#include "AwaitConfig.h"
#include "Config.h"
#include <Es8311.h>
#include "driver/i2s_std.h"

namespace AudioFeedback {

namespace {

constexpr float kTwoPi = 6.28318530718f;
constexpr uint8_t kStereoChannelCount = 2;
constexpr uint32_t kMillisecondsPerSecond = 1000;
constexpr uint32_t kToneFrames =
    Config::kAudioSampleRate * Config::kAudioBeepDurationMs / kMillisecondsPerSecond;
es8311_handle_t codec = nullptr;
i2s_chan_handle_t txChannel = nullptr;
i2s_chan_handle_t rxChannel = nullptr;
bool microphoneReady = false;
bool feedbackReady = false;
uint16_t latestMicrophoneMagnitude = 0;
bool microphoneSamplePending = false;
MicrophoneStatus currentMicrophoneStatus = MicrophoneStatus::kInitializing;
int16_t toneBuffer[kToneFrames * kStereoChannelCount] = {};

/** Releases only I2S channels owned by this module; the shared I2C bus remains external. */
void releaseI2sChannels() {
  if (txChannel != nullptr) {
    i2s_del_channel(txChannel);
    txChannel = nullptr;
  }
  if (rxChannel != nullptr) {
    i2s_del_channel(rxChannel);
    rxChannel = nullptr;
  }
}

bool beginI2s() {
  i2s_chan_config_t chanConfig = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  // Eight 64-frame descriptors hold the complete 30 ms stereo tone before TX
  // starts. RX shares the channel configuration and benefits from the same
  // bounded DMA depth without allocating application heap buffers.
  chanConfig.dma_desc_num = 8;
  chanConfig.dma_frame_num = Config::kAudioDmaFrames;
  // TX must fall back to silence if the cooperative producer misses a DMA
  // deadline; the ESP-IDF default repeats the last descriptor indefinitely.
  chanConfig.auto_clear_after_cb = true;
  if (i2s_new_channel(&chanConfig, &txChannel, &rxChannel) != ESP_OK) {
    releaseI2sChannels();
    return false;
  }

  i2s_std_config_t stdConfig = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(Config::kAudioSampleRate),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = static_cast<gpio_num_t>(Config::kAudioI2sMckPin),
              .bclk = static_cast<gpio_num_t>(Config::kAudioI2sBckPin),
              .ws = static_cast<gpio_num_t>(Config::kAudioI2sWsPin),
              .dout = static_cast<gpio_num_t>(Config::kAudioI2sDoutPin),
              .din = static_cast<gpio_num_t>(Config::kAudioI2sDinPin),
              .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
          },
  };
  stdConfig.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;

  // The first initialized channel owns BCLK/WS in an IDF full-duplex pair. RX
  // remains enabled continuously for music mode, so it must be initialized
  // first and act as the clock owner even while TX is disabled between cues.
  if (i2s_channel_init_std_mode(rxChannel, &stdConfig) != ESP_OK ||
      i2s_channel_init_std_mode(txChannel, &stdConfig) != ESP_OK) {
    releaseI2sChannels();
    return false;
  }
  if (i2s_channel_enable(rxChannel) != ESP_OK) {
    releaseI2sChannels();
    return false;
  }
  return true;
}

void prepareToneBuffer() {
  const uint32_t fadeSamples = Config::kAudioSampleRate * Config::kAudioFadeMs /
                               kMillisecondsPerSecond;
  const float angularStep =
      kTwoPi * Config::kAudioBeepFrequencyHz / static_cast<float>(Config::kAudioSampleRate);

  // Build once after hardware initialization. Keeping the 1.9 KB waveform in
  // static storage avoids placing it in a coroutine frame or allocating heap.
  for (uint32_t sampleIndex = 0; sampleIndex < kToneFrames; ++sampleIndex) {
    float envelope = 1.0f;
    if (sampleIndex < fadeSamples) {
      envelope = static_cast<float>(sampleIndex) / fadeSamples;
    } else if (sampleIndex > kToneFrames - fadeSamples) {
      envelope = static_cast<float>(kToneFrames - sampleIndex) / fadeSamples;
    }
    const int16_t sample =
        static_cast<int16_t>(sinf(angularStep * static_cast<float>(sampleIndex)) *
                             Config::kAudioToneAmplitude * envelope);
    toneBuffer[sampleIndex * kStereoChannelCount] = sample;
    toneBuffer[sampleIndex * kStereoChannelCount + 1] = sample;
  }
}

/** Preloads one complete tone so hardware clocks it without scheduler jitter. */
simpleawait::Task<void> playTone() {
  if (txChannel == nullptr) {
    co_return;
  }

  // Preloading while the channel is READY makes the first transmitted frame
  // valid audio. Enabling an empty channel would emit or repeat stale DMA data.
  size_t bytesLoaded = 0;
  const esp_err_t preloadResult =
      i2s_channel_preload_data(txChannel, toneBuffer, sizeof(toneBuffer), &bytesLoaded);
  if (preloadResult != ESP_OK || bytesLoaded != sizeof(toneBuffer)) {
    Serial.printf("WARNING: audio feedback preload incomplete: %u/%u bytes, error %d\n",
                  static_cast<unsigned>(bytesLoaded), static_cast<unsigned>(sizeof(toneBuffer)),
                  static_cast<int>(preloadResult));
  }
  if (bytesLoaded == 0) {
    co_return;
  }

  if (i2s_channel_enable(txChannel) != ESP_OK) {
    Serial.println("WARNING: unable to enable audio feedback channel");
    co_return;
  }

  // Delay by the duration actually accepted by DMA, not the requested buffer
  // size. This also gives a deterministic short cue if a future DMA capacity
  // change causes a partial preload.
  const uint32_t loadedFrames =
      bytesLoaded / (kStereoChannelCount * sizeof(toneBuffer[0]));
  const uint32_t playbackMs =
      (loadedFrames * kMillisecondsPerSecond + Config::kAudioSampleRate - 1) /
      Config::kAudioSampleRate;
  co_await simpleawait::delay_ms(playbackMs);

  if (i2s_channel_disable(txChannel) != ESP_OK) {
    Serial.println("WARNING: unable to disable audio feedback channel");
  }
}

bool sampleMicrophone(uint16_t& magnitude) {
  if (!microphoneReady || rxChannel == nullptr) {
    return false;
  }

  int16_t samples[Config::kAudioDmaFrames * kStereoChannelCount];
  uint64_t total = 0;
  size_t totalSamples = 0;
  // Drain every currently available RX block without waiting. The accumulated
  // mean absolute value represents one audio-service sample; no stale block is
  // left queued to distort the next lighting frame.
  while (true) {
    size_t bytesRead = 0;
    const esp_err_t result = i2s_channel_read(rxChannel, samples, sizeof(samples), &bytesRead, 0);
    if (result == ESP_ERR_TIMEOUT || bytesRead == 0) {
      break;
    }
    if (result != ESP_OK) {
      return false;
    }
    const size_t sampleCount = bytesRead / sizeof(samples[0]);
    for (size_t index = 0; index < sampleCount; ++index) {
      const int32_t sample = samples[index];
      total += sample < 0 ? -sample : sample;
    }
    totalSamples += sampleCount;
  }
  if (totalSamples == 0) {
    return false;
  }
  magnitude = static_cast<uint16_t>(total / totalSamples);
  return true;
}

simpleawait::Task<void> service(i2c_master_bus_handle_t touchBus, ControllerMessages& messages) {
  // GPIO and codec calls are intentionally delayed until coroutine execution;
  // global constructors in this module initialize data only.
  pinMode(Config::kAudioEnablePin, OUTPUT);
  digitalWrite(Config::kAudioEnablePin, LOW);

  codec = es8311_create(touchBus, Config::kAudioCodecI2cAddress);
  if (codec == nullptr || es8311_init_begin(codec) != ESP_OK) {
    currentMicrophoneStatus = MicrophoneStatus::kUnavailable;
    Serial.println("WARNING: audio codec reset failed; audio I/O unavailable");
    es8311_delete(codec);
    codec = nullptr;
    co_return;
  }
  co_await simpleawait::delay_ms(Config::kAudioCodecResetMs);
  if (es8311_init_finish(codec) != ESP_OK) {
    currentMicrophoneStatus = MicrophoneStatus::kUnavailable;
    Serial.println("WARNING: audio codec init failed; audio I/O unavailable");
    es8311_delete(codec);
    codec = nullptr;
    co_return;
  }

  es8311_voice_volume_set(codec, Config::kAudioCodecVolume, nullptr);
  es8311_voice_mute(codec, false);
  microphoneReady = es8311_microphone_config(codec) == ESP_OK;
  feedbackReady = beginI2s();
  microphoneReady = microphoneReady && feedbackReady;
  if (!feedbackReady) {
    currentMicrophoneStatus = MicrophoneStatus::kUnavailable;
    Serial.println("WARNING: audio I/O init failed; beeps and microphone unavailable");
    es8311_delete(codec);
    codec = nullptr;
    co_return;
  }
  if (!microphoneReady) {
    currentMicrophoneStatus = MicrophoneStatus::kUnavailable;
    Serial.println("WARNING: microphone init failed; music mode uses fallback glow");
  } else {
    currentMicrophoneStatus = MicrophoneStatus::kReady;
  }
  prepareToneBuffer();

  while (true) {
    // Publish only the newest amplitude. Lighting consumes at 25 ms while RX is
    // sampled at 5 ms, so a FIFO would create latency rather than information.
    uint16_t magnitude = 0;
    if (sampleMicrophone(magnitude)) {
      latestMicrophoneMagnitude = magnitude;
      microphoneSamplePending = true;
    }

    // Audio cues coalesce by priority in ControllerMessages. Playback is a
    // child coroutine so the service retains codec ownership while the parent
    // suspends and all unrelated controller tasks continue running.
    const AudioCue cue = messages.takeAudio();
    if (cue == AudioCue::kNone) {
      co_await simpleawait::delay_ms(Config::kAudioServicePollMs);
      continue;
    }

    co_await playTone();
    if (cue == AudioCue::kDouble) {
      co_await simpleawait::delay_ms(Config::kAudioBeepGapMs);
      co_await playTone();
    }
  }
}

}  // namespace

bool start(i2c_master_bus_handle_t touchBus, ControllerMessages& messages) {
  const bool started = simpleawait::create_task(service(touchBus, messages)).valid();
  if (!started) {
    currentMicrophoneStatus = MicrophoneStatus::kUnavailable;
  }
  return started;
}

MicrophoneStatus microphoneStatus() { return currentMicrophoneStatus; }

bool readMicrophoneLevel(uint16_t& magnitude) {
  if (!microphoneSamplePending) {
    return false;
  }
  magnitude = latestMicrophoneMagnitude;
  microphoneSamplePending = false;
  return true;
}

}  // namespace AudioFeedback
