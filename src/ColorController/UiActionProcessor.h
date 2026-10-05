#pragma once

#include "ControllerMessages.h"
#include "ControllerModel.h"
#include "UiAction.h"

/**
 * Application boundary between reusable controls and controller state.
 *
 * The application coroutine is the sole caller. The processor updates the
 * model and publishes fixed-memory messages; hardware-owning coroutines consume
 * those messages independently.
 */
class UiActionProcessor {
 public:
  UiActionProcessor(ControllerModel& model, ControllerMessages& messages)
      : model_(model), messages_(messages) {}

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
  ControllerMessages& messages_;
};
