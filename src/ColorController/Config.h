#pragma once

#include <Arduino.h>

// Override these macros at build time to target an external NeoPixel array.
#ifndef COLOR_CONTROLLER_PIXEL_PIN
#define COLOR_CONTROLLER_PIXEL_PIN 40
#endif

#ifndef COLOR_CONTROLLER_PIXEL_COUNT
#define COLOR_CONTROLLER_PIXEL_COUNT 1
#endif

namespace Config {

// Hardware and interaction settings shared by the sketch and model classes.
constexpr uint8_t kPixelPin = COLOR_CONTROLLER_PIXEL_PIN;
constexpr uint16_t kPixelCount = COLOR_CONTROLLER_PIXEL_COUNT;
constexpr uint8_t kDisplayRotation = 1;
constexpr uint32_t kPresetHoldMs = 700;
constexpr uint32_t kSavedFeedbackMs = 650;
constexpr uint8_t kInitialBrightness = 160;
constexpr uint8_t kReleaseDebouncePolls = 6;

}  // namespace Config
