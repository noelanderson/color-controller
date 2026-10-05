#include "ColorPersistenceService.h"

/**
 * @file ColorPersistenceService.cpp
 * @brief Applies delayed-save and retry policy around the Preferences adapter.
 *
 * The persistence coroutine is the sole runtime caller. A returned preset bit
 * means the desired value is durable (or already identical in NVS); callers
 * must not display SAVED before receiving that bit.
 */

#include <Arduino.h>

#include "Config.h"

bool ColorPersistenceService::begin(ControllerModel& model) { return state_.begin(model); }

void ColorPersistenceService::noteManualColor(const RgbColor& color, uint32_t now) {
  manualColorSave_.noteChange(color, now);
}

void ColorPersistenceService::cancelManualColor() { manualColorSave_.cancel(); }

void ColorPersistenceService::queuePresetSave(uint8_t index, const RgbColor& color, uint32_t now) {
  if (index >= ControllerModel::kPresetCount) {
    return;
  }
  pendingPresetColors_[index] = color;
  pendingPresetSaves_[index] = true;
  presetSaveRetryAt_[index] = now;
}

uint8_t ColorPersistenceService::process(uint32_t now, const ControllerModel& model) {
  uint8_t savedPresetMask = 0;
  // Each preset retries independently. One unavailable key cannot delay writes
  // for the other slots, and signed deadline comparison is rollover-safe for
  // the bounded retry interval.
  for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
    if (!pendingPresetSaves_[index] || static_cast<int32_t>(now - presetSaveRetryAt_[index]) < 0) {
      continue;
    }
    if (state_.savePreset(index, pendingPresetColors_[index])) {
      pendingPresetSaves_[index] = false;
      savedPresetMask |= static_cast<uint8_t>(1U << index);
    } else {
      Serial.printf("WARNING: unable to persist preset P%u\n", index + 1);
      presetSaveRetryAt_[index] = now + Config::kPersistenceRetryMs;
    }
  }

  // Manual color persistence is intentionally delayed to protect flash during
  // wheel drags. Effect entry cancels the candidate before this point.
  if (!manualColorSave_.ready(now, Config::kManualColorSaveDelayMs)) {
    return savedPresetMask;
  }
  if (model.mode() != OutputMode::kSolid || model.selected() != manualColorSave_.color()) {
    manualColorSave_.cancel();
    return savedPresetMask;
  }

  const RgbColor color = manualColorSave_.color();
  if (!state_.saveSelectedColor(color)) {
    Serial.println("WARNING: unable to persist selected color");
    manualColorSave_.noteChange(color, now);
  } else {
    manualColorSave_.markSaved();
  }
  return savedPresetMask;
}
