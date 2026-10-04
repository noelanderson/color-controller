#pragma once

#include <ST77922.h>
#include <TFT_eSPI.h>

#include "ColorMath.h"
#include "ControllerModel.h"
#include "UiLayout.h"

/**
 * Owns framebuffer rendering state and transfers complete frames to the panel.
 *
 * Construction only binds references; setup must initialize the display and
 * allocate the canvas before any drawing method is called.
 */
class UiRenderer {
 public:
  UiRenderer(TFT_eSprite& canvas, ST77922& display, ControllerModel& model);

  void flushDisplay();
  void drawColorStrip(const RgbColor& effectPreviewColor);
  void drawControl(uint8_t index);
  void drawPowerControl();
  void drawBrightnessControl(const RgbColor& effectPreviewColor);
  void drawDynamicUi(const RgbColor& effectPreviewColor);
  void drawInitialUi(const RgbColor& effectPreviewColor);

  /** Displays and later expires the transient SAVED preset label. */
  void showPresetSaved(uint8_t index, uint32_t until);
  void expireSavedPreset(uint32_t now);

 private:
  uint16_t toRgb565(const RgbColor& color);
  void drawCenteredText(const char* text, int16_t centerX, int16_t centerY, uint16_t foreground,
                        uint16_t background, uint8_t size = Ui::kDefaultTextSize);
  void drawColorWheel();
  void restoreMarkerBackground();
  void drawWheelMarker();
  void drawControls();

  TFT_eSprite& canvas_;
  ST77922& display_;
  ControllerModel& model_;
  int8_t savedPreset_ = -1;
  uint32_t savedFeedbackUntil_ = 0;
  int16_t markerX_ = -1;
  int16_t markerY_ = -1;
};
