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
  bool begin(ControllerModel& model);
  bool savePreset(uint8_t index, const RgbColor& color);
  bool saveSelectedColor(const RgbColor& color);

 private:
  static constexpr uint32_t kMissingColor = 0xFFFFFFFF;

  Preferences preferences_;
  uint32_t savedPresets_[ControllerModel::kPresetCount] = {};
  uint32_t savedSelectedColor_ = kMissingColor;
  bool ready_ = false;
};
