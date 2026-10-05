#include "PowerButtonControl.h"

#include "UiDrawing.h"
#include "UiLayout.h"

PowerButtonControl::PowerButtonControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds)
    : ButtonControl(bounds), UiElement(id), canvas_(canvas) {}

UiAction PowerButtonControl::onRelease(const TouchEvent&, bool releasedInside) {
  return releasedInside ? UiAction::togglePower() : UiAction{};
}

void PowerButtonControl::draw(const UiRenderContext& context) {
  const bool powerOn = context.model.powerOn();
  const uint16_t fill = powerOn ? Ui::kRed : Ui::kGreen;
  canvas_.fillRoundRect(x(), y(), width(), height(), Ui::kPowerCornerRadius, fill);
  canvas_.drawRoundRect(x(), y(), width(), height(), Ui::kPowerCornerRadius, Ui::kWhite);
  Ui::drawCenteredText(canvas_, powerOn ? "OFF" : "ON", x() + width() / 2, y() + height() / 2,
                       Ui::kWhite, fill);
}
