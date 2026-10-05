#include "ColorWheelControl.h"

#include <math.h>

#include "UiDrawing.h"
#include "UiLayout.h"

ColorWheelControl::ColorWheelControl(TFT_eSprite& canvas, UiElementId id, const Ui::WheelLayout& layout)
    : UiElement(id), canvas_(canvas), layout_(layout) {}

bool ColorWheelControl::contains(int16_t x, int16_t y) const {
  RgbColor ignored;
  return colorAt(x, y, ignored);
}

bool ColorWheelControl::colorAt(int16_t x, int16_t y, RgbColor& color) const {
  return colorFromWheel(x, y, layout_.bounds.centerX, layout_.bounds.centerY, layout_.bounds.radius, color);
}

TouchClaim ColorWheelControl::tryTouchDown(const TouchEvent& event) {
  if (!contains(event.point.x, event.point.y)) {
    return {};
  }
  return {true, touchMove(event)};
}

UiAction ColorWheelControl::touchMove(const TouchEvent& event) {
  RgbColor color;
  return colorAt(event.point.x, event.point.y, color) ? UiAction::selectColor(color) : UiAction{};
}

void ColorWheelControl::drawInitial(const UiRenderContext& context) {
  drawWheel();
  draw(context);
}

void ColorWheelControl::draw(const UiRenderContext& context) { drawMarker(context.model.selected()); }

void ColorWheelControl::drawWheel() {
  for (int16_t y = layout_.bounds.centerY - layout_.bounds.radius;
       y <= layout_.bounds.centerY + layout_.bounds.radius; ++y) {
    for (int16_t x = layout_.bounds.centerX - layout_.bounds.radius;
         x <= layout_.bounds.centerX + layout_.bounds.radius; ++x) {
      RgbColor color;
      if (colorAt(x, y, color)) {
        canvas_.drawPixel(x, y, Ui::toRgb565(canvas_, color));
      }
    }
  }
  canvas_.drawCircle(layout_.bounds.centerX, layout_.bounds.centerY, layout_.bounds.radius, Ui::kWhite);
}

void ColorWheelControl::drawMarker(const RgbColor& selectedColor) {
  restoreMarkerBackground();

  const HsvColor hsv = rgbToHsv(selectedColor);
  const float distance = static_cast<float>(hsv.saturation) * layout_.bounds.radius / kColorChannelMax;
  const float angle = static_cast<float>(hsv.hue) * kPi / (kHueCircleDegrees / 2.0f);
  markerX_ = layout_.bounds.centerX + lroundf(cosf(angle) * distance);
  markerY_ = layout_.bounds.centerY + lroundf(sinf(angle) * distance);

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
  // during a drag is too expensive for responsive touch handling. Deriving the
  // repair bounds from the marker margin is important at full saturation,
  // where the marker is centered on the wheel edge and extends beyond the disc.
  const Ui::Rect repairBounds = Ui::expandedBounds(layout_.bounds, Ui::kWheelMarkerRestoreRadius);
  const Ui::Rect framebufferBounds{0, 0, Ui::kWidth, Ui::kHeight};
  for (int16_t y = markerY_ - Ui::kWheelMarkerRestoreRadius; y <= markerY_ + Ui::kWheelMarkerRestoreRadius;
       ++y) {
    for (int16_t x = markerX_ - Ui::kWheelMarkerRestoreRadius; x <= markerX_ + Ui::kWheelMarkerRestoreRadius;
         ++x) {
      RgbColor color;
      if (colorAt(x, y, color)) {
        canvas_.drawPixel(x, y, Ui::toRgb565(canvas_, color));
      } else if (Ui::contains(repairBounds, x, y) && Ui::contains(framebufferBounds, x, y)) {
        canvas_.drawPixel(x, y, Ui::kBackground);
      }
    }
  }
  canvas_.drawCircle(layout_.bounds.centerX, layout_.bounds.centerY, layout_.bounds.radius, Ui::kWhite);
}
