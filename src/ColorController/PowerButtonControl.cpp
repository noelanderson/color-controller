#include "PowerButtonControl.h"

#include "UiDrawing.h"
#include "UiLayout.h"

PowerButtonControl::PowerButtonControl(TFT_eSprite& canvas) : canvas_(canvas) {}

bool PowerButtonControl::contains(int16_t x, int16_t y) const {
  return Ui::contains(x, y, Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight);
}

void PowerButtonControl::draw(bool powerOn) {
  const uint16_t fill = powerOn ? Ui::kRed : Ui::kGreen;
  canvas_.fillRoundRect(Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight, Ui::kPowerCornerRadius, fill);
  canvas_.drawRoundRect(Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight, Ui::kPowerCornerRadius, Ui::kWhite);
  Ui::drawCenteredText(canvas_, powerOn ? "OFF" : "ON", Ui::kPowerX + Ui::kPowerWidth / 2, Ui::kPowerY + Ui::kPowerHeight / 2, Ui::kWhite, fill);
}
