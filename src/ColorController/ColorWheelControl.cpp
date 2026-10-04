#include "ColorWheelControl.h"

#include <math.h>

#include "UiDrawing.h"
#include "UiLayout.h"

ColorWheelControl::ColorWheelControl(TFT_eSprite& canvas) : canvas_(canvas) {}

bool ColorWheelControl::contains(int16_t x, int16_t y) const {
  RgbColor ignored;
  return colorAt(x, y, ignored);
}

bool ColorWheelControl::colorAt(int16_t x, int16_t y, RgbColor& color) const {
  return colorFromWheel(x, y, Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius, color);
}

void ColorWheelControl::draw() {
  for (int16_t y = Ui::kWheelCenterY - Ui::kWheelRadius; y <= Ui::kWheelCenterY + Ui::kWheelRadius; ++y) {
    for (int16_t x = Ui::kWheelCenterX - Ui::kWheelRadius; x <= Ui::kWheelCenterX + Ui::kWheelRadius; ++x) {
      RgbColor color;
      if (colorAt(x, y, color)) {
        canvas_.drawPixel(x, y, Ui::toRgb565(canvas_, color));
      }
    }
  }
  canvas_.drawCircle(Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius, Ui::kWhite);
}

void ColorWheelControl::drawMarker(const RgbColor& selectedColor) {
  restoreMarkerBackground();

  const HsvColor hsv = rgbToHsv(selectedColor);
  const float distance = static_cast<float>(hsv.saturation) * Ui::kWheelRadius / kColorChannelMax;
  const float angle = static_cast<float>(hsv.hue) * kPi / (kHueCircleDegrees / 2.0f);
  markerX_ = Ui::kWheelCenterX + lroundf(cosf(angle) * distance);
  markerY_ = Ui::kWheelCenterY + lroundf(sinf(angle) * distance);

  const uint16_t outline =
      (selectedColor.red + selectedColor.green + selectedColor.blue) > Ui::kMarkerLightColorThreshold
          ? Ui::kBlack
          : Ui::kWhite;
  canvas_.drawCircle(markerX_, markerY_, Ui::kWheelMarkerRadius, outline);
  canvas_.drawCircle(markerX_, markerY_, Ui::kWheelMarkerOutlineRadius, outline);
}

void ColorWheelControl::restoreMarkerBackground() {
  if (markerX_ < 0 || markerY_ < 0) {
    return;
  }

  // Recompute only the previous marker footprint; redrawing the complete wheel
  // during a drag is too expensive for responsive touch handling.
  for (int16_t y = markerY_ - Ui::kWheelMarkerRestoreRadius; y <= markerY_ + Ui::kWheelMarkerRestoreRadius;
       ++y) {
    for (int16_t x = markerX_ - Ui::kWheelMarkerRestoreRadius; x <= markerX_ + Ui::kWheelMarkerRestoreRadius;
         ++x) {
      RgbColor color;
      if (colorAt(x, y, color)) {
        canvas_.drawPixel(x, y, Ui::toRgb565(canvas_, color));
      } else if (x >= 0 && x < Ui::kWidth && y >= Ui::kColorStripHeight && y < Ui::kHeight) {
        canvas_.drawPixel(x, y, Ui::kBackground);
      }
    }
  }
  canvas_.drawCircle(Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius, Ui::kWhite);
}
