#pragma once

#include <stdint.h>

// Override at build time to size the external P2 NeoPixel array; 0 disables it.
#ifndef COLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT
#define COLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT 0
#endif

namespace Config {

// Hardware and interaction settings shared by the sketch and model classes.
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
constexpr uint32_t kAudioBeepDurationMs = 70;
constexpr uint32_t kAudioBeepGapMs = 50;
constexpr uint32_t kEffectFrameMs = 25;
constexpr uint32_t kRainbowCycleMs = 12000;
constexpr uint32_t kRainbowBreathMs = 5000;

}  // namespace Config
