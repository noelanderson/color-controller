#include "BrightnessSliderControl.h"

#include <Arduino.h>

#include "UiDrawing.h"
#include "UiLayout.h"

BrightnessSliderControl::BrightnessSliderControl(TFT_eSprite& canvas, UiElementId id,
                                                 const Ui::SliderLayout& layout)
    : UiElement(id), canvas_(canvas), layout_(layout) {}

bool BrightnessSliderControl::contains(int16_t x, int16_t y) const {
  return Ui::contains(layout_.touchBounds, x, y);
}

uint8_t BrightnessSliderControl::brightnessAt(int16_t x) const {
  return brightnessFromX(x, layout_.startX, layout_.endX);
}

TouchClaim BrightnessSliderControl::tryTouchDown(const TouchEvent& event) {
  if (!contains(event.point.x, event.point.y)) {
    return {};
  }
  return {true, touchMove(event)};
}

UiAction BrightnessSliderControl::touchMove(const TouchEvent& event) {
  return UiAction::setBrightness(brightnessAt(event.point.x));
}

void BrightnessSliderControl::draw(const UiRenderContext& context) {
  const uint8_t brightness = context.model.brightness();
  canvas_.fillRect(layout_.redrawBounds.x, layout_.redrawBounds.y, layout_.redrawBounds.width,
                   layout_.redrawBounds.height, Ui::kBackground);
  canvas_.setTextDatum(TL_DATUM);
  canvas_.setTextColor(Ui::kWhite, Ui::kBackground);
  canvas_.setTextSize(Ui::kBrightnessTextSize);
  canvas_.drawString("BRIGHTNESS", layout_.startX, layout_.labelY);

  char value[Ui::kBrightnessLabelBufferSize];
  snprintf(value, sizeof(value), "%u", brightness);
  canvas_.setTextDatum(TR_DATUM);
  canvas_.drawString(value, layout_.endX, layout_.labelY);

  canvas_.fillRoundRect(layout_.startX, layout_.y - Ui::kSliderTrackHalfHeight,
                        layout_.endX - layout_.startX, Ui::kSliderTrackHeight,
                        Ui::kSliderTrackCornerRadius, Ui::kMuted);
  const int16_t knobX = layout_.startX +
                        static_cast<int32_t>(brightness) * (layout_.endX - layout_.startX) / UINT8_MAX;
  canvas_.fillCircle(knobX, layout_.y, Ui::kSliderKnobRadius,
                     Ui::toRgb565(canvas_, context.displayedColor()));
  canvas_.drawCircle(knobX, layout_.y, Ui::kSliderKnobRadius, Ui::kWhite);
}
