#pragma once

#include <TFT_eSPI.h>

#include "ButtonControl.h"
#include "PresetGesture.h"
#include "UiElement.h"

/**
 * One independently capturable P1-P4 recall/store button.
 *
 * The control owns gesture timing and transient SAVED visibility. It does not
 * mutate presets or persistence directly; those operations are emitted as
 * semantic actions and notifications.
 */
class PresetButtonControl : public ButtonControl, public UiElement {
 public:
  /**
   * @param canvas Sketch-owned framebuffer used for all drawing.
   * @param id Stable scene identifier used for targeted redraw and notification.
   * @param bounds Button hit and drawing rectangle.
   * @param index Zero-based ControllerModel preset index represented by this button.
   */
  PresetButtonControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds, uint8_t index);

  /** Advances hold detection and SAVED-label expiration. */
  UiAction tick(uint32_t now) override;
  /** Draws the preset swatch, label, and any active SAVED feedback. */
  void draw(const UiRenderContext& context) override;
  /** Accepts preset-saved feedback addressed through UiScene. */
  void notify(const UiNotification& notification) override;

 protected:
  UiAction onPress(const TouchEvent& event) override;
  UiAction onMove(const TouchEvent& event, bool inside) override;
  UiAction onRelease(const TouchEvent& event, bool releasedInside) override;

 private:
  TFT_eSprite& canvas_;
  PresetGesture gesture_;
  uint8_t index_;
  uint32_t savedUntil_ = 0;
  bool touchInside_ = false;
  bool savedVisible_ = false;
};
