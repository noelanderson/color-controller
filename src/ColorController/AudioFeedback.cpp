#include "AudioFeedback.h"

#include <Arduino.h>
#include <math.h>

#include "Config.h"
#include "Es8311.h"
#include "driver/i2s_std.h"

namespace AudioFeedback {

namespace {

constexpr float kTwoPi = 6.28318530718f;

es8311_handle_t codec = nullptr;
i2s_chan_handle_t txChannel = nullptr;

bool beginI2s() {
  i2s_chan_config_t chanConfig = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chanConfig, &txChannel, nullptr) != ESP_OK) {
    return false;
  }

  i2s_std_config_t stdConfig = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(Config::kAudioSampleRate),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                      I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = static_cast<gpio_num_t>(Config::kAudioI2sMckPin),
              .bclk = static_cast<gpio_num_t>(Config::kAudioI2sBckPin),
              .ws = static_cast<gpio_num_t>(Config::kAudioI2sWsPin),
              .dout = static_cast<gpio_num_t>(Config::kAudioI2sDoutPin),
              .din = I2S_GPIO_UNUSED,
              .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
          },
  };
  stdConfig.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;

  return i2s_channel_init_std_mode(txChannel, &stdConfig) == ESP_OK;
}

/** Writes a short sine tone, fading the ends in/out to avoid speaker pops.
 *  The channel is enabled only for the duration of the tone; otherwise the
 *  TX DMA keeps looping the last buffer and the tone never stops. */
void playTone(uint32_t durationMs) {
  if (txChannel == nullptr) {
    return;
  }
  i2s_channel_enable(txChannel);
  const uint32_t totalSamples = Config::kAudioSampleRate * durationMs / 1000;
  const uint32_t fadeSamples = Config::kAudioSampleRate * 5 / 1000;
  const float angularStep =
      kTwoPi * Config::kAudioBeepFrequencyHz / static_cast<float>(Config::kAudioSampleRate);

  uint32_t sampleIndex = 0;
  while (sampleIndex < totalSamples) {
    int16_t buffer[128];  // 64 stereo frames per chunk.
    const uint32_t remaining = totalSamples - sampleIndex;
    const uint32_t framesThisChunk = remaining < 64 ? remaining : 64;

    for (uint32_t frame = 0; frame < framesThisChunk; ++frame, ++sampleIndex) {
      float envelope = 1.0f;
      if (sampleIndex < fadeSamples) {
        envelope = static_cast<float>(sampleIndex) / fadeSamples;
      } else if (sampleIndex > totalSamples - fadeSamples) {
        envelope = static_cast<float>(totalSamples - sampleIndex) / fadeSamples;
      }
      const int16_t sample = static_cast<int16_t>(
          sinf(angularStep * static_cast<float>(sampleIndex)) * 12000.0f * envelope);
      buffer[frame * 2] = sample;
      buffer[frame * 2 + 1] = sample;
    }

    size_t bytesWritten = 0;
    i2s_channel_write(txChannel, buffer, framesThisChunk * 2 * sizeof(int16_t),
                      &bytesWritten, portMAX_DELAY);
  }
  i2s_channel_disable(txChannel);
}

}  // namespace

bool begin(i2c_master_bus_handle_t touchBus) {
  pinMode(Config::kAudioEnablePin, OUTPUT);
  // Board-specific polarity: LOW enables the onboard speaker amplifier.
  digitalWrite(Config::kAudioEnablePin, LOW);

  codec = es8311_create(touchBus, Config::kAudioCodecI2cAddress);
  if (codec == nullptr || es8311_init(codec) != ESP_OK) {
    return false;
  }
  es8311_voice_volume_set(codec, 70, nullptr);
  es8311_voice_mute(codec, false);

  return beginI2s();
}

void beep() {
  playTone(Config::kAudioBeepDurationMs);
}

void beepLong() {
  playTone(Config::kAudioBeepDurationMs);
  delay(Config::kAudioBeepGapMs);
  playTone(Config::kAudioBeepDurationMs);
}

}  // namespace AudioFeedback
