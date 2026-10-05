#pragma once

#include <TFT_eSPI.h>

#include "AudioFeedback.h"
#include "ButtonControl.h"
#include "UiElement.h"

/**
 * Shared release-inside behavior for the rainbow and music mode buttons.
 *
 * The music variant is the only drawable that queries microphone status; this
 * keeps optional audio state out of the generic UiRenderContext.
 */
class ModeButtonControl : public ButtonControl, public UiElement {
 public:
  /** Creates a mode button for kRainbow or kMusic without touching hardware. */
  ModeButtonControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds, OutputMode mode);

  /** Draws the mode glyph, active outline, and music fault indicator. */
  void draw(const UiRenderContext& context) override;

 protected:
  UiAction onRelease(const TouchEvent&, bool releasedInside) override;

 private:
  void drawActiveOutline(bool active);
  void drawRainbow(bool active);
  void drawMusic(bool active, AudioFeedback::MicrophoneStatus microphoneStatus);

  TFT_eSprite& canvas_;
  OutputMode mode_;
};
