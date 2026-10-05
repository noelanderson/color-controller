#pragma once

#include "ColorPersistenceService.h"
#include "ControllerModel.h"
#include "LightingOutput.h"
#include "UiAction.h"

class UiRenderer;

/**
 * Application boundary between reusable controls and device side effects.
 *
 * The processor updates the model first, then coordinates LED output,
 * persistence, audio feedback, and the smallest required redraw. It owns none
 * of those collaborators; all references must outlive it.
 */
class UiActionProcessor {
 public:
  UiActionProcessor(ControllerModel& model, LightingOutput& lighting, UiRenderer& renderer,
                    ColorPersistenceService& persistence)
      : model_(model), lighting_(lighting), renderer_(renderer), persistence_(persistence) {}

  /**
   * Applies one action synchronously.
   *
   * @param action Semantic intent; kNone is accepted as a no-op.
   * @param now Current millis() timestamp for persistence and notifications.
   */
  void process(const UiAction& action, uint32_t now);

 private:
  void selectColor(const RgbColor& color, uint32_t now);
  void setBrightness(uint8_t brightness);
  void recallPreset(uint8_t index, uint32_t now);
  void storePreset(uint8_t index, uint32_t now);
  void activateMode(OutputMode mode);

  ControllerModel& model_;
  LightingOutput& lighting_;
  UiRenderer& renderer_;
  ColorPersistenceService& persistence_;
};
