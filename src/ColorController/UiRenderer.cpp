#include "UiRenderer.h"

/**
 * @file UiRenderer.cpp
 * @brief Scene drawing facade and sole full-frame display-transfer implementation.
 */

#include <Arduino.h>

UiRenderer::UiRenderer(TFT_eSprite& canvas, ST77922& display, ControllerModel& model, UiScene& scene)
    : canvas_(canvas), display_(display), model_(model), scene_(scene) {}

UiRenderContext UiRenderer::context(const RgbColor& effectPreviewColor) const {
  return {model_, effectPreviewColor, millis()};
}

void UiRenderer::flushDisplay() {
  // Rotation 1 requires a full-frame transfer beginning at the panel origin.
  // The sprite lives in PSRAM; Fill_Colors consumes its contiguous RGB565
  // backing store synchronously, so no second frame buffer is required.
  display_.Fill_Colors(0, 0, Ui::kWidth, Ui::kHeight, static_cast<uint16_t*>(canvas_.getPointer()));
}

bool UiRenderer::drawElement(UiElementId id, const RgbColor& effectPreviewColor) {
  const bool found = scene_.draw(id, context(effectPreviewColor));
  // Report a composition bug once rather than flooding Serial at the 25 ms UI
  // cadence. The false result remains observable on every call.
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
  // Scene order is registration order, allowing the composition root to make
  // overlap behavior explicit without coupling elements to one another.
  scene_.draw(context(effectPreviewColor));
  flushDisplay();
}

void UiRenderer::drawInitialUi(const RgbColor& effectPreviewColor) {
  canvas_.fillSprite(Ui::kBackground);
  scene_.drawInitial(context(effectPreviewColor));
  flushDisplay();
}
