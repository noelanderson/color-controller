#pragma once

#include "ControllerModel.h"
#include "PersistencePolicy.h"
#include "PersistentState.h"

/**
 * Coordinates Preferences storage, delayed manual-color saves, and preset
 * retries. process() is called by the persistence coroutine at a fixed cadence.
 */
class ColorPersistenceService {
 public:
  bool begin(ControllerModel& model);

  void noteManualColor(const RgbColor& color, uint32_t now);
  void cancelManualColor();
  void queuePresetSave(uint8_t index, const RgbColor& color, uint32_t now);
  void process(uint32_t now, const ControllerModel& model);

 private:
  PersistentState state_;
  ManualColorSaveTracker manualColorSave_;
  RgbColor pendingPresetColors_[ControllerModel::kPresetCount] = {};
  bool pendingPresetSaves_[ControllerModel::kPresetCount] = {};
  uint32_t presetSaveRetryAt_[ControllerModel::kPresetCount] = {};
};
