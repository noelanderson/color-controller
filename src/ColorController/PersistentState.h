#pragma once

#include <Preferences.h>

#include "ControllerModel.h"

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
