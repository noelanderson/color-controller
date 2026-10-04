#pragma once

#include <TFT_eSPI.h>

/** Owns power-button hit testing and rendering. */
class PowerButtonControl {
 public:
  explicit PowerButtonControl(TFT_eSprite& canvas);

  bool contains(int16_t x, int16_t y) const;
  void draw(bool powerOn);

 private:
  TFT_eSprite& canvas_;
};
