#include "BrightnessSliderControl.h"

#include <Arduino.h>

#include "UiDrawing.h"
#include "UiLayout.h"

BrightnessSliderControl::BrightnessSliderControl(TFT_eSprite& canvas) : canvas_(canvas) {}

bool BrightnessSliderControl::contains(int16_t x, int16_t y) const {
  return Ui::contains(x, y, Ui::kSliderTouchX, Ui::kSliderTouchY, Ui::kSliderTouchWidth,
                      Ui::kSliderTouchHeight);
}

uint8_t BrightnessSliderControl::brightnessAt(int16_t x) const {
  return brightnessFromX(x, Ui::kSliderStartX, Ui::kSliderEndX);
}

void BrightnessSliderControl::draw(uint8_t brightness, const RgbColor& knobColor) {
  canvas_.fillRect(Ui::kSliderRedrawX, Ui::kSliderRedrawY, Ui::kSliderRedrawWidth, Ui::kSliderRedrawHeight,
                   Ui::kBackground);
  canvas_.setTextDatum(TL_DATUM);
  canvas_.setTextColor(Ui::kWhite, Ui::kBackground);
  canvas_.setTextSize(Ui::kBrightnessTextSize);
  canvas_.drawString("BRIGHTNESS", Ui::kSliderStartX, Ui::kSliderLabelY);

  char value[Ui::kBrightnessLabelBufferSize];
  snprintf(value, sizeof(value), "%u", brightness);
  canvas_.setTextDatum(TR_DATUM);
  canvas_.drawString(value, Ui::kSliderEndX, Ui::kSliderLabelY);

  canvas_.fillRoundRect(Ui::kSliderStartX, Ui::kSliderY - Ui::kSliderTrackHalfHeight,
                        Ui::kSliderEndX - Ui::kSliderStartX, Ui::kSliderTrackHeight,
                        Ui::kSliderTrackCornerRadius, Ui::kMuted);
  const int16_t knobX = Ui::kSliderStartX +
                        static_cast<int32_t>(brightness) * (Ui::kSliderEndX - Ui::kSliderStartX) / UINT8_MAX;
  canvas_.fillCircle(knobX, Ui::kSliderY, Ui::kSliderKnobRadius, Ui::toRgb565(canvas_, knobColor));
  canvas_.drawCircle(knobX, Ui::kSliderY, Ui::kSliderKnobRadius, Ui::kWhite);
}
