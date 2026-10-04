#include "ColorPreviewControl.h"

#include "UiDrawing.h"
#include "UiLayout.h"

ColorPreviewControl::ColorPreviewControl(TFT_eSprite& canvas) : canvas_(canvas) {}

void ColorPreviewControl::draw(const RgbColor& color) {
  canvas_.fillRect(0, 0, Ui::kWidth, Ui::kColorStripHeight, Ui::toRgb565(canvas_, color));
  canvas_.drawFastHLine(0, Ui::kColorStripHeight - 1, Ui::kWidth, Ui::kWhite);
}
