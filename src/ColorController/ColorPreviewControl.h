#pragma once

#include <TFT_eSPI.h>

#include "ColorMath.h"
#include "UiElement.h"
#include "UiGeometry.h"

/**
 * Drawable-only strip showing the color currently produced by the controller.
 *
 * Solid mode uses the stable selection; effect modes use the latest preview
 * sampled from LightingOutput.
 */
class ColorPreviewControl : public UiElement {
 public:
  /** Binds framebuffer, scene identity, and preview bounds without drawing. */
  ColorPreviewControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds);

  /** Fills the preview bounds with UiRenderContext::displayedColor(). */
  void draw(const UiRenderContext& context) override;

 private:
  TFT_eSprite& canvas_;
  Ui::Rect bounds_;
};
