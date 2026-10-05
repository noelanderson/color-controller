#pragma once

#include <TFT_eSPI.h>

#include "ColorMath.h"
#include "InteractiveControl.h"
#include "UiElement.h"
#include "UiGeometry.h"

/**
 * Self-contained color-wheel UI control.
 *
 * Owns wheel hit testing, coordinate-to-color conversion, rendering, and the
 * previous marker position needed for efficient drag updates. It emits colors
 * to its caller rather than mutating controller state directly.
 */
class ColorWheelControl : public InteractiveControl, public UiElement {
 public:
  /**
   * Binds a sketch-owned framebuffer and immutable layout.
   *
   * The framebuffer and this control must outlive all scene registrations.
   */
  ColorWheelControl(TFT_eSprite& canvas, UiElementId id, const Ui::WheelLayout& layout);

  /** @return Whether the point is inside the circular wheel, not merely its box. */
  bool contains(int16_t x, int16_t y) const;
  /**
   * Converts a wheel point to full-value RGB.
   *
   * @return false for points outside the wheel, leaving @p color unchanged.
   */
  bool colorAt(int16_t x, int16_t y, RgbColor& color) const;
  TouchClaim tryTouchDown(const TouchEvent& event) override;
  UiAction touchMove(const TouchEvent& event) override;
  UiAction touchUp(const TouchEvent&) override { return {}; }
  void drawInitial(const UiRenderContext& context) override;
  /**
   * Updates only the selection marker after the initial wheel bitmap exists.
   *
   * drawInitial() must run before targeted dynamic redraws so marker repair can
   * reconstruct wheel pixels instead of storing a second bitmap.
   */
  void draw(const UiRenderContext& context) override;

 private:
  void drawWheel();
  void drawMarker(const RgbColor& selectedColor);
  void restoreMarkerBackground();

  TFT_eSprite& canvas_;
  Ui::WheelLayout layout_;
  int16_t markerX_ = -1;
  int16_t markerY_ = -1;
};
