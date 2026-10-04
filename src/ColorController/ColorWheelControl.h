#pragma once

#include <TFT_eSPI.h>

#include "ColorMath.h"

/**
 * Self-contained color-wheel UI control.
 *
 * Owns wheel hit testing, coordinate-to-color conversion, rendering, and the
 * previous marker position needed for efficient drag updates. It emits colors
 * to its caller rather than mutating controller state directly.
 */
class ColorWheelControl {
 public:
  explicit ColorWheelControl(TFT_eSprite& canvas);

  bool contains(int16_t x, int16_t y) const;
  bool colorAt(int16_t x, int16_t y, RgbColor& color) const;
  void draw();
  void drawMarker(const RgbColor& selectedColor);

 private:
  void restoreMarkerBackground();

  TFT_eSprite& canvas_;
  int16_t markerX_ = -1;
  int16_t markerY_ = -1;
};
