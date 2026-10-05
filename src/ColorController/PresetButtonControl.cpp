#include "PresetButtonControl.h"

#include <Arduino.h>

#include "Config.h"
#include "UiDrawing.h"
#include "UiLayout.h"

PresetButtonControl::PresetButtonControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds,
                                         uint8_t index)
    : ButtonControl(bounds), UiElement(id), canvas_(canvas), index_(index) {}

UiAction PresetButtonControl::onPress(const TouchEvent& event) {
  touchInside_ = true;
  gesture_.begin(event.now);
  return {};
}

UiAction PresetButtonControl::onMove(const TouchEvent& event, bool inside) {
  touchInside_ = inside;
  if (gesture_.update(event.now, inside, Config::kPresetHoldMs) == PresetGestureEvent::kStore) {
    return UiAction::preset(UiActionType::kStorePreset, index_);
  }
  return {};
}

UiAction PresetButtonControl::onRelease(const TouchEvent&, bool releasedInside) {
  touchInside_ = false;
  if (gesture_.release(releasedInside) == PresetGestureEvent::kRecall) {
    return UiAction::preset(UiActionType::kRecallPreset, index_);
  }
  return {};
}

UiAction PresetButtonControl::tick(uint32_t now) {
  if (gesture_.update(now, touchInside_, Config::kPresetHoldMs) == PresetGestureEvent::kStore) {
    return UiAction::preset(UiActionType::kStorePreset, index_);
  }
  if (savedVisible_ && static_cast<int32_t>(now - savedUntil_) >= 0) {
    savedVisible_ = false;
    return UiAction::preset(UiActionType::kPresetFeedbackExpired, index_);
  }
  return {};
}

void PresetButtonControl::notify(const UiNotification& notification) {
  if (notification.type == UiNotificationType::kPresetSaved) {
    savedUntil_ = notification.until;
    savedVisible_ = true;
  }
}

void PresetButtonControl::draw(const UiRenderContext& context) {
  const RgbColor color = context.model.preset(index_);
  const uint16_t fill = Ui::toRgb565(canvas_, color);
  const uint16_t text =
      (static_cast<uint16_t>(color.red) * Ui::kRedLuminanceWeight +
       static_cast<uint16_t>(color.green) * Ui::kGreenLuminanceWeight +
       static_cast<uint16_t>(color.blue) * Ui::kBlueLuminanceWeight) > Ui::kLightBackgroundThreshold
          ? Ui::kBlack
          : Ui::kWhite;

  canvas_.fillRoundRect(x(), y(), width(), height(), Ui::kControlCornerRadius, fill);
  canvas_.drawRoundRect(x(), y(), width(), height(), Ui::kControlCornerRadius, Ui::kWhite);

  char label[Ui::kPresetLabelBufferSize];
  const bool saved = savedVisible_ && static_cast<int32_t>(context.now - savedUntil_) < 0;
  if (saved) {
    snprintf(label, sizeof(label), "SAVED");
  } else {
    snprintf(label, sizeof(label), "P%u", index_ + 1);
  }
  Ui::drawCenteredText(canvas_, label, x() + width() / 2, y() + height() / 2, text, fill);
}
