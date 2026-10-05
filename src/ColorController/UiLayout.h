#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @file UiLayout.h
 * @brief Shared framebuffer dimensions, drawing metrics, and RGB565 theme tokens.
 *
 * Control objects receive their screen placement from the sketch composition
 * root. This file contains only reusable drawing measurements and colors, so a
 * control's implementation never depends on another control's location.
 */

namespace Ui {

/** Landscape framebuffer geometry required by display rotation 1. */
constexpr int16_t kWidth = 480;
constexpr int16_t kHeight = 320;
constexpr uint8_t kFramebufferColorDepth = 16;

/** Marker radii include the erase footprint needed when the marker crosses the wheel edge. */
constexpr int16_t kWheelMarkerRadius = 6;
constexpr int16_t kWheelMarkerOutlineRadius = 7;
constexpr int16_t kWheelMarkerRestoreRadius = 8;

// Preset and mode control styling.
constexpr int16_t kControlCornerRadius = 8;
constexpr int16_t kActiveControlInset = 2;
constexpr int16_t kActiveControlCornerRadius = 6;

// Rainbow-mode glyph.
constexpr uint8_t kRainbowDotCount = 6;
constexpr uint16_t kRainbowGlyphHueSpan = 300;
constexpr int16_t kRainbowDotStartX = 18;
constexpr int16_t kRainbowDotSpacing = 12;
constexpr int16_t kRainbowDotBaseY = 25;
constexpr int16_t kRainbowDotRadius = 7;
constexpr int16_t kRainbowArcVerticalScale = 2;

// Music-note glyph and microphone status marker.
constexpr int16_t kMusicHeadCenterOffsetX = -7;
constexpr int16_t kMusicHeadCenterY = 37;
constexpr int16_t kMusicHeadRadius = 8;
constexpr int16_t kMusicStemOffsetX = 6;
constexpr int16_t kMusicStemTopY = 12;
constexpr int16_t kMusicStemWidth = 4;
constexpr int16_t kMusicFlagLength = 20;
constexpr int16_t kMusicFlagRise = 8;
constexpr int16_t kMicStatusInset = 10;
constexpr int16_t kMicStatusRadius = 4;

// Power and brightness control styling.
constexpr int16_t kPowerCornerRadius = 9;
constexpr int16_t kSliderTrackHalfHeight = 4;
constexpr int16_t kSliderTrackHeight = 9;
constexpr int16_t kSliderTrackCornerRadius = 4;
constexpr int16_t kSliderKnobRadius = 11;

// RGB565 theme colors.
constexpr uint16_t kBackground = 0x1082;
constexpr uint16_t kMuted = 0x7BEF;
constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kBlack = 0x0000;
constexpr uint16_t kGreen = 0x0600;
constexpr uint16_t kRed = 0xB800;
constexpr uint16_t kActive = 0x07E0;
constexpr uint16_t kMusic = 0x4010;

// Integer luminance weights used to choose readable preset-label text.
constexpr uint16_t kRedLuminanceWeight = 299;
constexpr uint16_t kGreenLuminanceWeight = 587;
constexpr uint16_t kBlueLuminanceWeight = 114;
constexpr uint32_t kLightBackgroundThreshold = 140000;
constexpr uint16_t kMarkerLightColorThreshold = 420;
constexpr size_t kPresetLabelBufferSize = 8;
constexpr size_t kBrightnessLabelBufferSize = 5;
constexpr uint8_t kDefaultTextSize = 2;
constexpr uint8_t kBrightnessTextSize = 1;

}  // namespace Ui
