#pragma once

#include <TFT_eSPI.h>

#include "ColorMath.h"

/** Owns brightness-slider hit testing, coordinate mapping, and rendering. */
class BrightnessSliderControl {
 public:
  explicit BrightnessSliderControl(TFT_eSprite& canvas);

  bool contains(int16_t x, int16_t y) const;
  uint8_t brightnessAt(int16_t x) const;
  void draw(uint8_t brightness, const RgbColor& knobColor);

 private:
  TFT_eSprite& canvas_;
};
