#pragma once

#include <ST77922.h>
#include <TFT_eSPI.h>

#include "BrightnessSliderControl.h"
#include "ColorMath.h"
#include "ColorPreviewControl.h"
#include "ColorWheelControl.h"
#include "ControllerModel.h"
#include "PowerButtonControl.h"
#include "UiLayout.h"

/**
 * Owns framebuffer rendering state and transfers complete frames to the panel.
 *
 * Construction only binds references; setup must initialize the display and
 * allocate the canvas before any drawing method is called.
 */
class UiRenderer {
 public:
  UiRenderer(TFT_eSprite& canvas, ST77922& display, ControllerModel& model, ColorWheelControl& colorWheel,
             BrightnessSliderControl& brightnessSlider, PowerButtonControl& powerButton,
             ColorPreviewControl& colorPreview);

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
  void drawControls();

  TFT_eSprite& canvas_;
  ST77922& display_;
  ControllerModel& model_;
  ColorWheelControl& colorWheel_;
  BrightnessSliderControl& brightnessSlider_;
  PowerButtonControl& powerButton_;
  ColorPreviewControl& colorPreview_;
  int8_t savedPreset_ = -1;
  uint32_t savedFeedbackUntil_ = 0;
};
