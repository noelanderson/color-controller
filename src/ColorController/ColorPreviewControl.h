#pragma once

#include <TFT_eSPI.h>

#include "ColorMath.h"

/** Renders the selected or live effect color across the top preview strip. */
class ColorPreviewControl {
 public:
  explicit ColorPreviewControl(TFT_eSprite& canvas);

  void draw(const RgbColor& color);

 private:
  TFT_eSprite& canvas_;
};
