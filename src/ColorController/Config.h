#pragma once

#include <stddef.h>
#include <stdint.h>

// Override at build time to size the external P2 addressable LED array; 0 disables it.
#ifndef COLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT
#define COLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT 0
#endif

namespace Config {

// Hardware and interaction settings shared by the sketch and model classes.
constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kFatalReportMs = 1000;
constexpr uint8_t kOnboardPixelPin = 40;
constexpr uint16_t kOnboardPixelCount = 1;
constexpr uint8_t kExternalPixelPin = 45;
constexpr uint16_t kExternalPixelCount = COLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT;
constexpr uint8_t kDisplayRotation = 1;
constexpr uint32_t kPresetHoldMs = 700;
constexpr uint32_t kSavedFeedbackMs = 650;
constexpr uint8_t kInitialBrightness = 160;
constexpr uint8_t kReleaseDebouncePolls = 6;

// Onboard speaker amplifier enable and I2S pins to the ES8311 codec.
constexpr uint8_t kAudioEnablePin = 1;
constexpr uint8_t kAudioI2sMckPin = 17;
constexpr uint8_t kAudioI2sBckPin = 18;
constexpr uint8_t kAudioI2sWsPin = 21;
constexpr uint8_t kAudioI2sDoutPin = 15;
constexpr uint8_t kAudioI2sDinPin = 16;
constexpr uint8_t kAudioCodecI2cAddress = 0x18;
constexpr uint32_t kAudioSampleRate = 16000;
constexpr uint16_t kAudioBeepFrequencyHz = 1800;
constexpr uint32_t kAudioBeepDurationMs = 30;
constexpr uint32_t kAudioBeepGapMs = 25;
constexpr uint32_t kAudioCodecResetMs = 20;
constexpr uint32_t kAudioServicePollMs = 5;
constexpr uint16_t kAudioDmaFrames = 64;
constexpr uint32_t kAudioFadeMs = 5;
constexpr int16_t kAudioToneAmplitude = 12000;
constexpr int kAudioCodecVolume = 70;
constexpr uint32_t kTouchPollMs = 5;
constexpr uint32_t kEffectFrameMs = 25;
constexpr uint32_t kUiServicePollMs = 25;
constexpr uint32_t kEffectUiFrameMs = 200;
constexpr uint32_t kRainbowCycleMs = 12000;
constexpr uint32_t kRainbowBreathMs = 5000;
constexpr uint32_t kManualColorSaveDelayMs = 120000;
constexpr uint32_t kPersistencePollMs = 250;
constexpr uint32_t kPersistenceRetryMs = 5000;
constexpr size_t kUiActionQueueCapacity = 8;

}  // namespace Config
