#include "PersistentState.h"

/**
 * @file PersistentState.cpp
 * @brief Preferences/NVS encoding, startup restoration, and write deduplication.
 *
 * RGB values use a stable 24-bit representation rather than object layout.
 * Cached packed values suppress unchanged flash writes. This adapter owns its
 * Preferences namespace but never owns or closes the surrounding NVS service.
 */

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
  // Startup restoration is the only model mutation outside the application
  // coroutine. setup() completes this call before any runtime task is spawned.
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
      // Cache the usable firmware default so a later request for the same value
      // is treated as already synchronized rather than causing a repair write.
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
