#pragma once

#include <TFT_eSPI.h>

#include "ButtonControl.h"
#include "UiElement.h"

/**
 * Power toggle button with self-contained rendering and touch behavior.
 *
 * Release inside emits a toggle action; the button never clears color, mode,
 * or brightness state.
 */
class PowerButtonControl : public ButtonControl, public UiElement {
 public:
  /** Binds framebuffer, scene identity, and shared rectangular gesture bounds. */
  PowerButtonControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds);

  /** Draws ON/OFF state from the supplied model snapshot. */
  void draw(const UiRenderContext& context) override;

 protected:
  UiAction onRelease(const TouchEvent&, bool releasedInside) override;

 private:
  TFT_eSprite& canvas_;
};
