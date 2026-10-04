#include "InteractionController.h"

#include <Arduino.h>

#include "AudioFeedback.h"
#include "Config.h"
#include "UiLayout.h"

InteractionController::InteractionController(ST77922_TOUCH& touch, ControllerModel& model,
                                             LightingOutput& lighting, UiRenderer& renderer,
                                             ColorPersistenceService& persistence,
                                             const ColorWheelControl& colorWheel,
                                             const BrightnessSliderControl& brightnessSlider,
                                             const PowerButtonControl& powerButton)
    : touch_(touch),
      model_(model),
      lighting_(lighting),
      renderer_(renderer),
      persistence_(persistence),
      colorWheel_(colorWheel),
      brightnessSlider_(brightnessSlider),
      powerButton_(powerButton) {}

void InteractionController::selectWheelColor(int16_t x, int16_t y) {
  RgbColor color;
  if (!colorWheel_.colorAt(x, y, color) ||
      (color == model_.selected() && model_.mode() == OutputMode::kSolid)) {
    return;
  }
  model_.select(color);
  const uint32_t now = millis();
  persistence_.noteManualColor(color, now);
  lighting_.apply(now);
  renderer_.drawDynamicUi(lighting_.previewColor());
}

void InteractionController::setBrightnessFromTouch(int16_t x) {
  const uint8_t brightness = brightnessSlider_.brightnessAt(x);
  if (brightness == model_.brightness()) {
    return;
  }
  model_.setBrightness(brightness);
  lighting_.apply();
  renderer_.drawBrightnessControl(lighting_.previewColor());
  renderer_.flushDisplay();
}

InteractionController::TouchTarget InteractionController::identifyTarget(int16_t x, int16_t y,
                                                                         uint8_t& presetIndex) {
  if (colorWheel_.contains(x, y)) {
    return TouchTarget::kWheel;
  }
  for (uint8_t index = 0; index < ControllerModel::kControlCount; ++index) {
    if (Ui::presetContains(index, x, y)) {
      presetIndex = index;
      return TouchTarget::kPreset;
    }
  }
  if (powerButton_.contains(x, y)) {
    return TouchTarget::kPower;
  }
  if (brightnessSlider_.contains(x, y)) {
    return TouchTarget::kBrightness;
  }
  return TouchTarget::kNone;
}

void InteractionController::handleTouchDown(int16_t x, int16_t y, uint32_t now) {
  touchState_ = {};
  touchState_.target = identifyTarget(x, y, touchState_.presetIndex);
  touchState_.lastX = x;
  touchState_.lastY = y;

  switch (touchState_.target) {
    case TouchTarget::kPreset:
      if (touchState_.presetIndex < ControllerModel::kPresetCount) {
        presetGesture_.begin(now);
      }
      break;
    case TouchTarget::kWheel:
      selectWheelColor(x, y);
      break;
    case TouchTarget::kBrightness:
      setBrightnessFromTouch(x);
      break;
    default:
      break;
  }
}

void InteractionController::handleTouchMove(int16_t x, int16_t y) {
  touchState_.lastX = x;
  touchState_.lastY = y;
  switch (touchState_.target) {
    case TouchTarget::kWheel:
      selectWheelColor(x, y);
      break;
    case TouchTarget::kBrightness:
      setBrightnessFromTouch(x);
      break;
    default:
      break;
  }
}

void InteractionController::updatePresetHold(uint32_t now) {
  if (touchState_.target != TouchTarget::kPreset ||
      touchState_.presetIndex >= ControllerModel::kPresetCount ||
      presetGesture_.update(now,
                            Ui::presetContains(touchState_.presetIndex, touchState_.lastX, touchState_.lastY),
                            Config::kPresetHoldMs) != PresetGestureEvent::kStore) {
    return;
  }

  model_.storePreset(touchState_.presetIndex);
  model_.setMode(OutputMode::kSolid);
  persistence_.noteManualColor(model_.selected(), now);
  persistence_.queuePresetSave(touchState_.presetIndex, model_.selected(), now);
  renderer_.showPresetSaved(touchState_.presetIndex, now + Config::kSavedFeedbackMs);
  lighting_.apply(now);
  renderer_.drawDynamicUi(lighting_.previewColor());
  AudioFeedback::beepLong();
}

void InteractionController::recallPreset(uint8_t index) {
  const RgbColor preset = model_.preset(index);
  if (preset != model_.selected() || model_.mode() != OutputMode::kSolid) {
    model_.select(preset);
    lighting_.apply();
    renderer_.drawDynamicUi(lighting_.previewColor());
    persistence_.noteManualColor(preset, millis());
  }
  AudioFeedback::beep();
}

void InteractionController::activateMode(OutputMode mode) {
  model_.setMode(mode);
  persistence_.cancelManualColor();
  if (mode == OutputMode::kMusic) {
    lighting_.resetMusicEnvelope();
  }
  lighting_.apply();
  renderer_.drawDynamicUi(lighting_.previewColor());
  AudioFeedback::beep();
}

void InteractionController::handlePresetRelease() {
  const uint8_t index = touchState_.presetIndex;
  const bool inside = Ui::presetContains(index, touchState_.lastX, touchState_.lastY);

  if (index < ControllerModel::kPresetCount) {
    if (presetGesture_.release(inside) == PresetGestureEvent::kRecall) {
      recallPreset(index);
    }
    return;
  }
  if (!inside) {
    return;
  }

  switch (index) {
    case ControllerModel::kRainbowControlIndex:
      activateMode(OutputMode::kRainbow);
      break;
    case ControllerModel::kMusicControlIndex:
      activateMode(OutputMode::kMusic);
      break;
    default:
      break;
  }
}

void InteractionController::handlePowerRelease() {
  if (!powerButton_.contains(touchState_.lastX, touchState_.lastY)) {
    return;
  }
  model_.togglePower();
  lighting_.apply();
  renderer_.drawPowerControl();
  renderer_.flushDisplay();
  AudioFeedback::beep();
}

void InteractionController::handleTouchUp() {
  switch (touchState_.target) {
    case TouchTarget::kPreset:
      handlePresetRelease();
      break;
    case TouchTarget::kPower:
      handlePowerRelease();
      break;
    default:
      break;
  }

  presetGesture_.reset();
  touchState_ = {};
}

void InteractionController::handleTouchSample(uint32_t now) {
  const int16_t x = constrain(static_cast<int16_t>(touch_.touch.x[0]), 0, Ui::kWidth - 1);
  const int16_t y = constrain(static_cast<int16_t>(touch_.touch.y[0]), 0, Ui::kHeight - 1);

  if (contactState_ == ContactState::kIdle) {
    handleTouchDown(x, y, now);
  } else {
    // A sample that returns during release debounce continues the same gesture.
    handleTouchMove(x, y);
  }
  contactState_ = ContactState::kPressed;
  noTouchPolls_ = 0;
}

void InteractionController::handleNoTouch() {
  if (contactState_ == ContactState::kIdle) {
    return;
  }

  if (contactState_ == ContactState::kPressed) {
    // Count the first missing sample immediately so release occurs after
    // exactly kReleaseDebouncePolls consecutive misses.
    contactState_ = ContactState::kReleaseDebouncing;
    noTouchPolls_ = 1;
  } else {
    ++noTouchPolls_;
  }

  if (noTouchPolls_ >= Config::kReleaseDebouncePolls) {
    handleTouchUp();
    contactState_ = ContactState::kIdle;
    noTouchPolls_ = 0;
  }
}

void InteractionController::poll(uint32_t now) {
  if (touch_.Get_Touch()) {
    handleTouchSample(now);
  } else {
    handleNoTouch();
  }

  updatePresetHold(now);
  renderer_.expireSavedPreset(now);
}
