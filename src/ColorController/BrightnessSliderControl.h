#pragma once

#include <TFT_eSPI.h>

#include "ColorMath.h"
#include "InteractiveControl.h"
#include "UiElement.h"
#include "UiGeometry.h"

/** Owns brightness-slider hit testing, coordinate mapping, and rendering. */
class BrightnessSliderControl : public InteractiveControl, public UiElement {
 public:
  /** Binds a sketch-owned framebuffer and slider geometry without drawing. */
  BrightnessSliderControl(TFT_eSprite& canvas, UiElementId id, const Ui::SliderLayout& layout);

  /** @return Whether the point is inside the finger-friendly touch region. */
  bool contains(int16_t x, int16_t y) const;
  /** Maps and clamps a horizontal coordinate to absolute brightness 0-255. */
  uint8_t brightnessAt(int16_t x) const;
  TouchClaim tryTouchDown(const TouchEvent& event) override;
  UiAction touchMove(const TouchEvent& event) override;
  UiAction touchUp(const TouchEvent&) override { return {}; }
  /** Repaints the complete redraw bounds, including label, track, and thumb. */
  void draw(const UiRenderContext& context) override;

 private:
  TFT_eSprite& canvas_;
  Ui::SliderLayout layout_;
};
