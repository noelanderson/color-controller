#include "UiActionProcessor.h"

/**
 * @file UiActionProcessor.cpp
 * @brief Applies semantic UI intent and publishes side effects without hardware I/O.
 *
 * The order within each handler is deliberate: mutate authoritative model
 * state first, then publish persistence/output/UI/audio work for independent
 * owners. Consumers therefore never observe a request whose model state is old.
 */

void UiActionProcessor::selectColor(const RgbColor& color, uint32_t now) {
  if (color == model_.selected() && model_.mode() == OutputMode::kSolid) {
    return;
  }
  model_.select(color);
  messages_.noteManualColor(color, now);
  messages_.requestLighting(false, kUiRefreshDynamic);
}

void UiActionProcessor::setBrightness(uint8_t brightness) {
  if (brightness == model_.brightness()) {
    return;
  }
  model_.setBrightness(brightness);
  messages_.requestLighting(false, kUiRefreshBrightness);
}

void UiActionProcessor::recallPreset(uint8_t index, uint32_t now) {
  if (index >= ControllerModel::kPresetCount) {
    return;
  }
  const RgbColor preset = model_.preset(index);
  if (preset != model_.selected() || model_.mode() != OutputMode::kSolid) {
    model_.select(preset);
    messages_.noteManualColor(preset, now);
    messages_.requestLighting(false, kUiRefreshDynamic);
  }
  messages_.requestAudio(AudioCue::kSingle);
}

void UiActionProcessor::storePreset(uint8_t index, uint32_t now) {
  if (index >= ControllerModel::kPresetCount) {
    return;
  }
  // Holding a preset means "store the currently visible color." Returning to
  // solid mode makes that value authoritative before lighting and persistence
  // consumers are notified. SAVED is not emitted here; NVS confirms it later.
  model_.storePreset(index);
  model_.setMode(OutputMode::kSolid);
  messages_.noteManualColor(model_.selected(), now);
  messages_.queuePresetSave(index, model_.selected(), now);
  messages_.requestLighting(false, kUiRefreshDynamic);
  messages_.requestAudio(AudioCue::kDouble);
}

void UiActionProcessor::activateMode(OutputMode mode) {
  if (mode == OutputMode::kSolid) {
    return;
  }
  model_.setMode(mode);
  // An effect owns the selected output while active, so a manual-color
  // candidate must not become durable after this transition.
  messages_.cancelManualColor();
  messages_.requestLighting(mode == OutputMode::kMusic, kUiRefreshDynamic);
  messages_.requestAudio(AudioCue::kSingle);
}

void UiActionProcessor::process(const UiAction& action, uint32_t now) {
  switch (action.type) {
    case UiActionType::kSelectColor:
      selectColor(action.color, now);
      break;
    case UiActionType::kSetBrightness:
      setBrightness(action.value);
      break;
    case UiActionType::kTogglePower:
      model_.togglePower();
      messages_.requestLighting(false, kUiRefreshPower);
      messages_.requestAudio(AudioCue::kSingle);
      break;
    case UiActionType::kRecallPreset:
      recallPreset(action.value, now);
      break;
    case UiActionType::kStorePreset:
      storePreset(action.value, now);
      break;
    case UiActionType::kActivateMode:
      activateMode(action.mode);
      break;
    case UiActionType::kPresetFeedbackExpired:
      messages_.requestPresetRedraw(action.value);
      break;
    case UiActionType::kNone:
      break;
  }
}
