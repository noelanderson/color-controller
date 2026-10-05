#include "UiRenderer.h"

#include <Arduino.h>

UiRenderer::UiRenderer(TFT_eSprite& canvas, ST77922& display, ControllerModel& model, UiScene& scene)
    : canvas_(canvas), display_(display), model_(model), scene_(scene) {}

UiRenderContext UiRenderer::context(const RgbColor& effectPreviewColor) const {
  return {model_, effectPreviewColor, millis()};
}

void UiRenderer::flushDisplay() {
  // Rotation 1 requires a full-frame transfer beginning at the panel origin.
  display_.Fill_Colors(0, 0, Ui::kWidth, Ui::kHeight, static_cast<uint16_t*>(canvas_.getPointer()));
}

bool UiRenderer::drawElement(UiElementId id, const RgbColor& effectPreviewColor) {
  const bool found = scene_.draw(id, context(effectPreviewColor));
  if (!found && !missingElementReported_) {
    Serial.printf("UI scene element %u is not registered.\n", id);
    missingElementReported_ = true;
  }
  return found;
}

bool UiRenderer::notifyElement(UiElementId id, const UiNotification& notification) {
  const bool found = scene_.notify(id, notification);
  if (!found && !missingElementReported_) {
    Serial.printf("UI scene element %u is not registered.\n", id);
    missingElementReported_ = true;
  }
  return found;
}

void UiRenderer::drawDynamicUi(const RgbColor& effectPreviewColor) {
  scene_.draw(context(effectPreviewColor));
  flushDisplay();
}

void UiRenderer::drawInitialUi(const RgbColor& effectPreviewColor) {
  canvas_.fillSprite(Ui::kBackground);
  scene_.drawInitial(context(effectPreviewColor));
  flushDisplay();
}
