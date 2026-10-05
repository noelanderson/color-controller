#pragma once

#include <TFT_eSPI.h>

#include "ColorMath.h"
#include "UiLayout.h"

namespace Ui {

/** Converts a controller RGB color through the active TFT color implementation. */
inline uint16_t toRgb565(TFT_eSprite& canvas, const RgbColor& color) {
  return canvas.color565(color.red, color.green, color.blue);
}

/**
 * Draws opaque text centered on one framebuffer coordinate.
 *
 * This helper intentionally sets all required TFT text state so controls do
 * not depend on state left by the previously drawn scene element.
 */
inline void drawCenteredText(TFT_eSprite& canvas, const char* text, int16_t centerX, int16_t centerY,
                             uint16_t foreground, uint16_t background, uint8_t size = Ui::kDefaultTextSize) {
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(foreground, background);
  canvas.setTextSize(size);
  canvas.drawString(text, centerX, centerY);
}

}  // namespace Ui
