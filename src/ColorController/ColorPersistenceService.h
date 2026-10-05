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
  /**
   * Opens Preferences storage and restores saved values into the model.
   *
   * @param model Model receiving any valid stored selected color and presets.
   * @return true when the namespace opened; false leaves defaults usable.
   */
  bool begin(ControllerModel& model);

  /**
   * Restarts the delayed-save timer for a manually selected solid color.
   *
   * @param color Stable solid color candidate.
   * @param now Current millis() timestamp.
   */
  void noteManualColor(const RgbColor& color, uint32_t now);
  /** Cancels a pending selected-color save when an effect becomes authoritative. */
  void cancelManualColor();
  /**
   * Attempts a preset save and records a retry when NVS rejects the write.
   *
   * @param index Zero-based preset index.
   * @param color Preset value to persist.
   * @param now Current millis() timestamp used to schedule retry.
   */
  void queuePresetSave(uint8_t index, const RgbColor& color, uint32_t now);
  /**
   * Performs due delayed and retry writes.
   *
   * This method is non-blocking apart from the underlying Preferences call and
   * is intended to run at the persistence task cadence.
   */
  /**
   * @return Bit mask of preset slots durably written during this call.
   */
  uint8_t process(uint32_t now, const ControllerModel& model);

 private:
  PersistentState state_;
  ManualColorSaveTracker manualColorSave_;
  RgbColor pendingPresetColors_[ControllerModel::kPresetCount] = {};
  bool pendingPresetSaves_[ControllerModel::kPresetCount] = {};
  uint32_t presetSaveRetryAt_[ControllerModel::kPresetCount] = {};
};
