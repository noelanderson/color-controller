#pragma once

#include <Preferences.h>

#include "ControllerModel.h"

/**
 * Stores presets and the stable manual color in the ESP32 Preferences/NVS partition.
 *
 * Cached packed values suppress writes that would needlessly consume flash endurance.
 */
class PersistentState {
 public:
  /**
   * Opens the `colorctl` namespace and restores values into the model.
   *
   * @return true when Preferences is ready for subsequent writes.
   */
  bool begin(ControllerModel& model);
  /**
   * Persists a preset in packed 0x00RRGGBB form.
   *
   * A value equal to the cached stored value succeeds without writing flash.
   */
  bool savePreset(uint8_t index, const RgbColor& color);
  /** Persists the stable selected color, suppressing an unchanged flash write. */
  bool saveSelectedColor(const RgbColor& color);

 private:
  static constexpr uint32_t kMissingColor = 0xFFFFFFFF;

  Preferences preferences_;
  uint32_t savedPresets_[ControllerModel::kPresetCount] = {};
  uint32_t savedSelectedColor_ = kMissingColor;
  bool ready_ = false;
};
