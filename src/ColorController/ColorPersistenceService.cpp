#include "ColorPersistenceService.h"

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

void ColorPersistenceService::process(uint32_t now, const ControllerModel& model) {
  for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
    if (!pendingPresetSaves_[index] || static_cast<int32_t>(now - presetSaveRetryAt_[index]) < 0) {
      continue;
    }
    if (state_.savePreset(index, pendingPresetColors_[index])) {
      pendingPresetSaves_[index] = false;
    } else {
      Serial.printf("WARNING: unable to persist preset P%u\n", index + 1);
      presetSaveRetryAt_[index] = now + Config::kPersistenceRetryMs;
    }
  }

  if (!manualColorSave_.ready(now, Config::kManualColorSaveDelayMs)) {
    return;
  }
  if (model.mode() != OutputMode::kSolid || model.selected() != manualColorSave_.color()) {
    manualColorSave_.cancel();
    return;
  }

  const RgbColor color = manualColorSave_.color();
  if (!state_.saveSelectedColor(color)) {
    Serial.println("WARNING: unable to persist selected color");
    manualColorSave_.noteChange(color, now);
  } else {
    manualColorSave_.markSaved();
  }
}
