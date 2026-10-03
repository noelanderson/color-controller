#pragma once

#include <Arduino.h>

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

}  // namespace Config

