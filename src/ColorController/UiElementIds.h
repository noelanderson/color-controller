#pragma once

#include <stdint.h>

#include "ControllerModel.h"
#include "UiElement.h"

/**
 * Application-owned scene identifiers.
 *
 * UiElement and UiScene intentionally treat IDs as opaque values. Keeping the
 * concrete inventory here lets another composition replace these identifiers
 * without changing the reusable scene contract.
 */
namespace UiElementIds {

constexpr UiElementId kPreview = 0;
constexpr UiElementId kWheel = 1;
constexpr UiElementId kPreset1 = 2;
constexpr UiElementId kPreset2 = 3;
constexpr UiElementId kPreset3 = 4;
constexpr UiElementId kPreset4 = 5;
constexpr UiElementId kRainbow = 6;
constexpr UiElementId kMusic = 7;
constexpr UiElementId kPower = 8;
constexpr UiElementId kBrightness = 9;

/** Maps the model's zero-based preset index to its scene element identifier. */
constexpr UiElementId preset(uint8_t index) { return static_cast<UiElementId>(kPreset1 + index); }

static_assert(kPreset4 - kPreset1 + 1 == ControllerModel::kPresetCount);

}  // namespace UiElementIds
