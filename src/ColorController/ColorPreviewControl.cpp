#include "ColorPreviewControl.h"

#include "UiDrawing.h"
#include "UiLayout.h"

ColorPreviewControl::ColorPreviewControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds)
    : UiElement(id), canvas_(canvas), bounds_(bounds) {}

void ColorPreviewControl::draw(const UiRenderContext& context) {
  canvas_.fillRect(bounds_.x, bounds_.y, bounds_.width, bounds_.height,
                   Ui::toRgb565(canvas_, context.displayedColor()));
  canvas_.drawFastHLine(bounds_.x, bounds_.y + bounds_.height - 1, bounds_.width, Ui::kWhite);
}
