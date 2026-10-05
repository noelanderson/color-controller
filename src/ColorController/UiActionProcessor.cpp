#include "UiActionProcessor.h"

#include "AudioFeedback.h"
#include "Config.h"
#include "UiElementIds.h"
#include "UiRenderer.h"

void UiActionProcessor::selectColor(const RgbColor& color, uint32_t now) {
  if (color == model_.selected() && model_.mode() == OutputMode::kSolid) {
    return;
  }
  model_.select(color);
  persistence_.noteManualColor(color, now);
  lighting_.apply(now);
  renderer_.drawDynamicUi(lighting_.previewColor());
}

void UiActionProcessor::setBrightness(uint8_t brightness) {
  if (brightness == model_.brightness()) {
    return;
  }
  model_.setBrightness(brightness);
  lighting_.apply();
  renderer_.drawElement(UiElementIds::kBrightness, lighting_.previewColor());
  renderer_.flushDisplay();
}

void UiActionProcessor::recallPreset(uint8_t index, uint32_t now) {
  if (index >= ControllerModel::kPresetCount) {
    return;
  }
  const RgbColor preset = model_.preset(index);
  if (preset != model_.selected() || model_.mode() != OutputMode::kSolid) {
    model_.select(preset);
    lighting_.apply(now);
    renderer_.drawDynamicUi(lighting_.previewColor());
    persistence_.noteManualColor(preset, now);
  }
  AudioFeedback::beep();
}

void UiActionProcessor::storePreset(uint8_t index, uint32_t now) {
  if (index >= ControllerModel::kPresetCount) {
    return;
  }
  model_.storePreset(index);
  model_.setMode(OutputMode::kSolid);
  persistence_.noteManualColor(model_.selected(), now);
  persistence_.queuePresetSave(index, model_.selected(), now);
  renderer_.notifyElement(UiElementIds::preset(index),
                          {UiNotificationType::kPresetSaved, now + Config::kSavedFeedbackMs});
  lighting_.apply(now);
  renderer_.drawDynamicUi(lighting_.previewColor());
  AudioFeedback::beepLong();
}

void UiActionProcessor::activateMode(OutputMode mode) {
  if (mode == OutputMode::kSolid) {
    return;
  }
  model_.setMode(mode);
  persistence_.cancelManualColor();
  if (mode == OutputMode::kMusic) {
    lighting_.resetMusicEnvelope();
  }
  lighting_.apply();
  renderer_.drawDynamicUi(lighting_.previewColor());
  AudioFeedback::beep();
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
      lighting_.apply(now);
      renderer_.drawElement(UiElementIds::kPower, lighting_.previewColor());
      renderer_.flushDisplay();
      AudioFeedback::beep();
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
      renderer_.drawElement(UiElementIds::preset(action.value), lighting_.previewColor());
      renderer_.flushDisplay();
      break;
    case UiActionType::kNone:
      break;
  }
}
