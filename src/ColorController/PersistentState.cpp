#include "PersistentState.h"

#include "PersistencePolicy.h"

namespace {

constexpr char kNamespace[] = "colorctl";
constexpr char kSelectedColorKey[] = "selected";
constexpr char kPresetKeys[ControllerModel::kPresetCount][3] = {
    "p0",
    "p1",
    "p2",
    "p3",
};

bool isPackedColor(uint32_t packed) {
  // Valid colors occupy only 24 bits; the all-ones sentinel therefore cannot collide.
  return (packed & 0xFF000000) == 0;
}

}  // namespace

bool PersistentState::begin(ControllerModel& model) {
  ready_ = preferences_.begin(kNamespace, false);
  if (!ready_) {
    return false;
  }

  for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
    const uint32_t packed = preferences_.getUInt(kPresetKeys[index], kMissingColor);
    if (isPackedColor(packed)) {
      model.setPreset(index, unpackColor(packed));
      savedPresets_[index] = packed;
    } else {
      savedPresets_[index] = packColor(model.preset(index));
    }
  }

  const uint32_t packedSelected = preferences_.getUInt(kSelectedColorKey, kMissingColor);
  if (isPackedColor(packedSelected)) {
    model.select(unpackColor(packedSelected));
    savedSelectedColor_ = packedSelected;
  }
  return true;
}

bool PersistentState::savePreset(uint8_t index, const RgbColor& color) {
  if (!ready_ || index >= ControllerModel::kPresetCount) {
    return false;
  }
  const uint32_t packed = packColor(color);
  if (savedPresets_[index] == packed) {
    return true;
  }
  if (preferences_.putUInt(kPresetKeys[index], packed) != sizeof(packed)) {
    return false;
  }
  savedPresets_[index] = packed;
  return true;
}

bool PersistentState::saveSelectedColor(const RgbColor& color) {
  if (!ready_) {
    return false;
  }
  const uint32_t packed = packColor(color);
  if (savedSelectedColor_ == packed) {
    return true;
  }
  if (preferences_.putUInt(kSelectedColorKey, packed) != sizeof(packed)) {
    return false;
  }
  savedSelectedColor_ = packed;
  return true;
}
