#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Ui {

// Framebuffer and top-level panel geometry.
constexpr int16_t kWidth = 480;
constexpr int16_t kHeight = 320;
constexpr uint8_t kFramebufferColorDepth = 16;
constexpr int16_t kColorStripHeight = 14;
constexpr int16_t kPanelX = 248;
constexpr int16_t kPanelY = 18;
constexpr int16_t kPanelWidth = 226;
constexpr int16_t kPanelHeight = 207;
constexpr int16_t kPanelCornerRadius = 10;

// Color wheel and selection marker.
constexpr int16_t kWheelCenterX = 120;
constexpr int16_t kWheelCenterY = 132;
constexpr int16_t kWheelRadius = 105;
constexpr int16_t kWheelMarkerRadius = 6;
constexpr int16_t kWheelMarkerOutlineRadius = 7;
constexpr int16_t kWheelMarkerRestoreRadius = 8;

// Preset and mode controls.
constexpr uint8_t kControlColumns = 2;
constexpr int16_t kPresetWidth = 96;
constexpr int16_t kPresetHeight = 56;
constexpr int16_t kPresetX[kControlColumns] = {260, 370};
constexpr int16_t kPresetY[3] = {24, 91, 158};
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

// Power and brightness controls.
constexpr int16_t kPowerX = 12;
constexpr int16_t kPowerY = 262;
constexpr int16_t kPowerWidth = 88;
constexpr int16_t kPowerHeight = 46;
constexpr int16_t kPowerCornerRadius = 9;
constexpr int16_t kSliderStartX = 128;
constexpr int16_t kSliderEndX = 462;
constexpr int16_t kSliderY = 286;
constexpr int16_t kSliderLabelY = 255;
constexpr int16_t kSliderRedrawX = kSliderStartX - 12;
constexpr int16_t kSliderRedrawY = 252;
constexpr int16_t kSliderRedrawWidth = kSliderEndX - kSliderStartX + 25;
constexpr int16_t kSliderRedrawHeight = 62;
constexpr int16_t kSliderTrackHalfHeight = 4;
constexpr int16_t kSliderTrackHeight = 9;
constexpr int16_t kSliderTrackCornerRadius = 4;
constexpr int16_t kSliderKnobRadius = 11;
constexpr int16_t kSliderTouchX = kSliderStartX - 12;
constexpr int16_t kSliderTouchY = kSliderY - 22;
constexpr int16_t kSliderTouchWidth = kSliderEndX - kSliderStartX + 24;
constexpr int16_t kSliderTouchHeight = 44;

// RGB565 theme colors.
constexpr uint16_t kBackground = 0x1082;
constexpr uint16_t kPanel = 0x2104;
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

inline bool contains(int16_t x, int16_t y, int16_t left, int16_t top, int16_t width, int16_t height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

inline bool presetContains(uint8_t index, int16_t x, int16_t y) {
  const uint8_t column = index % kControlColumns;
  const uint8_t row = index / kControlColumns;
  return contains(x, y, kPresetX[column], kPresetY[row], kPresetWidth, kPresetHeight);
}

}  // namespace Ui
