#include "UiRenderer.h"

#include <Arduino.h>
#include <math.h>

#include "AudioFeedback.h"

UiRenderer::UiRenderer(TFT_eSprite& canvas, ST77922& display, ControllerModel& model)
    : canvas_(canvas), display_(display), model_(model) {}

uint16_t UiRenderer::toRgb565(const RgbColor& color) {
  return canvas_.color565(color.red, color.green, color.blue);
}

void UiRenderer::flushDisplay() {
  // Rotation 1 requires a full-frame transfer beginning at the panel origin.
  display_.Fill_Colors(0, 0, Ui::kWidth, Ui::kHeight, static_cast<uint16_t*>(canvas_.getPointer()));
}

void UiRenderer::drawCenteredText(const char* text, int16_t centerX, int16_t centerY, uint16_t foreground,
                                  uint16_t background, uint8_t size) {
  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(foreground, background);
  canvas_.setTextSize(size);
  canvas_.drawString(text, centerX, centerY);
}

void UiRenderer::drawColorStrip(const RgbColor& effectPreviewColor) {
  const RgbColor& color = model_.mode() == OutputMode::kSolid ? model_.selected() : effectPreviewColor;
  canvas_.fillRect(0, 0, Ui::kWidth, Ui::kColorStripHeight, toRgb565(color));
  canvas_.drawFastHLine(0, Ui::kColorStripHeight - 1, Ui::kWidth, Ui::kWhite);
}

void UiRenderer::drawColorWheel() {
  for (int16_t y = Ui::kWheelCenterY - Ui::kWheelRadius; y <= Ui::kWheelCenterY + Ui::kWheelRadius; ++y) {
    for (int16_t x = Ui::kWheelCenterX - Ui::kWheelRadius; x <= Ui::kWheelCenterX + Ui::kWheelRadius; ++x) {
      RgbColor color;
      if (colorFromWheel(x, y, Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius, color)) {
        canvas_.drawPixel(x, y, toRgb565(color));
      }
    }
  }
  canvas_.drawCircle(Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius, Ui::kWhite);
}

void UiRenderer::restoreMarkerBackground() {
  if (markerX_ < 0 || markerY_ < 0) {
    return;
  }
  // Recompute only the previous marker footprint; redrawing the wheel during a
  // drag is too expensive for responsive touch handling.
  for (int16_t y = markerY_ - Ui::kWheelMarkerRestoreRadius; y <= markerY_ + Ui::kWheelMarkerRestoreRadius;
       ++y) {
    for (int16_t x = markerX_ - Ui::kWheelMarkerRestoreRadius; x <= markerX_ + Ui::kWheelMarkerRestoreRadius;
         ++x) {
      RgbColor color;
      if (colorFromWheel(x, y, Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius, color)) {
        canvas_.drawPixel(x, y, toRgb565(color));
      } else if (x >= 0 && x < Ui::kWidth && y >= Ui::kColorStripHeight && y < Ui::kHeight) {
        canvas_.drawPixel(x, y, Ui::kBackground);
      }
    }
  }
  canvas_.drawCircle(Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius, Ui::kWhite);
}

void UiRenderer::drawWheelMarker() {
  restoreMarkerBackground();
  const HsvColor hsv = rgbToHsv(model_.selected());
  const float distance = static_cast<float>(hsv.saturation) * Ui::kWheelRadius / kColorChannelMax;
  const float angle = static_cast<float>(hsv.hue) * kPi / (kHueCircleDegrees / 2.0f);
  markerX_ = Ui::kWheelCenterX + lroundf(cosf(angle) * distance);
  markerY_ = Ui::kWheelCenterY + lroundf(sinf(angle) * distance);
  const uint16_t outline = (model_.selected().red + model_.selected().green + model_.selected().blue) >
                                   Ui::kMarkerLightColorThreshold
                               ? Ui::kBlack
                               : Ui::kWhite;
  canvas_.drawCircle(markerX_, markerY_, Ui::kWheelMarkerRadius, outline);
  canvas_.drawCircle(markerX_, markerY_, Ui::kWheelMarkerOutlineRadius, outline);
}

void UiRenderer::drawControl(uint8_t index) {
  const uint8_t column = index % Ui::kControlColumns;
  const uint8_t row = index / Ui::kControlColumns;
  const int16_t x = Ui::kPresetX[column];
  const int16_t y = Ui::kPresetY[row];

  if (index == ControllerModel::kRainbowControlIndex) {
    canvas_.fillRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, Ui::kControlCornerRadius, Ui::kBlack);
    for (uint8_t dot = 0; dot < Ui::kRainbowDotCount; ++dot) {
      const RgbColor color =
          hsvToRgb({static_cast<uint16_t>(dot * Ui::kRainbowGlyphHueSpan / (Ui::kRainbowDotCount - 1)),
                    UINT8_MAX, UINT8_MAX});
      const int16_t dotX = x + Ui::kRainbowDotStartX + dot * Ui::kRainbowDotSpacing;
      const int16_t distanceFromCenter =
          abs(static_cast<int16_t>(dot) * Ui::kRainbowArcVerticalScale - (Ui::kRainbowDotCount - 1));
      const int16_t dotY = y + Ui::kRainbowDotBaseY + distanceFromCenter;
      canvas_.fillCircle(dotX, dotY, Ui::kRainbowDotRadius, toRgb565(color));
    }
    const uint16_t outline = model_.mode() == OutputMode::kRainbow ? Ui::kActive : Ui::kWhite;
    canvas_.drawRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, Ui::kControlCornerRadius, outline);
    if (model_.mode() == OutputMode::kRainbow) {
      canvas_.drawRoundRect(x + Ui::kActiveControlInset, y + Ui::kActiveControlInset,
                            Ui::kPresetWidth - 2 * Ui::kActiveControlInset,
                            Ui::kPresetHeight - 2 * Ui::kActiveControlInset, Ui::kActiveControlCornerRadius,
                            outline);
    }
    return;
  }

  if (index == ControllerModel::kMusicControlIndex) {
    const uint16_t outline = model_.mode() == OutputMode::kMusic ? Ui::kActive : Ui::kWhite;
    canvas_.fillRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, Ui::kControlCornerRadius, Ui::kMusic);
    canvas_.drawRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, Ui::kControlCornerRadius, outline);
    if (model_.mode() == OutputMode::kMusic) {
      canvas_.drawRoundRect(x + Ui::kActiveControlInset, y + Ui::kActiveControlInset,
                            Ui::kPresetWidth - 2 * Ui::kActiveControlInset,
                            Ui::kPresetHeight - 2 * Ui::kActiveControlInset, Ui::kActiveControlCornerRadius,
                            outline);
    }
    const int16_t headX = x + Ui::kPresetWidth / 2 + Ui::kMusicHeadCenterOffsetX;
    const int16_t headY = y + Ui::kMusicHeadCenterY;
    const int16_t stemX = headX + Ui::kMusicStemOffsetX;
    const int16_t stemTop = y + Ui::kMusicStemTopY;
    canvas_.fillCircle(headX, headY, Ui::kMusicHeadRadius, Ui::kWhite);
    canvas_.fillRect(stemX, stemTop, Ui::kMusicStemWidth, headY - stemTop, Ui::kWhite);
    for (uint8_t offset = 0; offset < Ui::kMusicStemWidth; ++offset) {
      canvas_.drawLine(stemX + Ui::kMusicStemWidth - 1, stemTop + offset, stemX + Ui::kMusicFlagLength,
                       stemTop + Ui::kMusicFlagRise + offset, Ui::kWhite);
    }
    // The status dot indicates a confirmed microphone failure, not recording.
    // Hide it while asynchronous codec initialization is still in progress.
    if (AudioFeedback::microphoneStatus() == AudioFeedback::MicrophoneStatus::kUnavailable) {
      canvas_.fillCircle(x + Ui::kPresetWidth - Ui::kMicStatusInset, y + Ui::kMicStatusInset,
                         Ui::kMicStatusRadius, Ui::kRed);
    }
    return;
  }

  const RgbColor color = model_.preset(index);
  const uint16_t fill = toRgb565(color);
  const uint16_t text =
      (static_cast<uint16_t>(color.red) * Ui::kRedLuminanceWeight +
       static_cast<uint16_t>(color.green) * Ui::kGreenLuminanceWeight +
       static_cast<uint16_t>(color.blue) * Ui::kBlueLuminanceWeight) > Ui::kLightBackgroundThreshold
          ? Ui::kBlack
          : Ui::kWhite;

  canvas_.fillRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, Ui::kControlCornerRadius, fill);
  canvas_.drawRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, Ui::kControlCornerRadius, Ui::kWhite);
  char label[Ui::kPresetLabelBufferSize];
  if (savedPreset_ == index && static_cast<int32_t>(millis() - savedFeedbackUntil_) < 0) {
    snprintf(label, sizeof(label), "SAVED");
  } else {
    snprintf(label, sizeof(label), "P%u", index + 1);
  }
  drawCenteredText(label, x + Ui::kPresetWidth / 2, y + Ui::kPresetHeight / 2, text, fill);
}

void UiRenderer::drawControls() {
  for (uint8_t index = 0; index < ControllerModel::kControlCount; ++index) {
    drawControl(index);
  }
}

void UiRenderer::drawPowerControl() {
  const uint16_t fill = model_.powerOn() ? Ui::kRed : Ui::kGreen;
  canvas_.fillRoundRect(Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight, Ui::kPowerCornerRadius,
                        fill);
  canvas_.drawRoundRect(Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight, Ui::kPowerCornerRadius,
                        Ui::kWhite);
  drawCenteredText(model_.powerOn() ? "OFF" : "ON", Ui::kPowerX + Ui::kPowerWidth / 2,
                   Ui::kPowerY + Ui::kPowerHeight / 2, Ui::kWhite, fill);
}

void UiRenderer::drawBrightnessControl(const RgbColor& effectPreviewColor) {
  canvas_.fillRect(Ui::kSliderRedrawX, Ui::kSliderRedrawY, Ui::kSliderRedrawWidth, Ui::kSliderRedrawHeight,
                   Ui::kBackground);
  canvas_.setTextDatum(TL_DATUM);
  canvas_.setTextColor(Ui::kWhite, Ui::kBackground);
  canvas_.setTextSize(Ui::kBrightnessTextSize);
  canvas_.drawString("BRIGHTNESS", Ui::kSliderStartX, Ui::kSliderLabelY);

  char value[Ui::kBrightnessLabelBufferSize];
  snprintf(value, sizeof(value), "%u", model_.brightness());
  canvas_.setTextDatum(TR_DATUM);
  canvas_.drawString(value, Ui::kSliderEndX, Ui::kSliderLabelY);

  canvas_.fillRoundRect(Ui::kSliderStartX, Ui::kSliderY - Ui::kSliderTrackHalfHeight,
                        Ui::kSliderEndX - Ui::kSliderStartX, Ui::kSliderTrackHeight,
                        Ui::kSliderTrackCornerRadius, Ui::kMuted);
  const int16_t knobX = Ui::kSliderStartX + static_cast<int32_t>(model_.brightness()) *
                                                (Ui::kSliderEndX - Ui::kSliderStartX) / UINT8_MAX;
  const RgbColor& color = model_.mode() == OutputMode::kSolid ? model_.selected() : effectPreviewColor;
  canvas_.fillCircle(knobX, Ui::kSliderY, Ui::kSliderKnobRadius, toRgb565(color));
  canvas_.drawCircle(knobX, Ui::kSliderY, Ui::kSliderKnobRadius, Ui::kWhite);
}

void UiRenderer::drawDynamicUi(const RgbColor& effectPreviewColor) {
  drawColorStrip(effectPreviewColor);
  drawWheelMarker();
  drawControls();
  drawPowerControl();
  drawBrightnessControl(effectPreviewColor);
  flushDisplay();
}

void UiRenderer::drawInitialUi(const RgbColor& effectPreviewColor) {
  canvas_.fillSprite(Ui::kBackground);
  canvas_.fillRoundRect(Ui::kPanelX, Ui::kPanelY, Ui::kPanelWidth, Ui::kPanelHeight, Ui::kPanelCornerRadius,
                        Ui::kPanel);
  drawColorWheel();
  drawDynamicUi(effectPreviewColor);
}

void UiRenderer::showPresetSaved(uint8_t index, uint32_t until) {
  savedPreset_ = index;
  savedFeedbackUntil_ = until;
}

void UiRenderer::expireSavedPreset(uint32_t now) {
  if (savedPreset_ < 0 || static_cast<int32_t>(now - savedFeedbackUntil_) < 0) {
    return;
  }
  const uint8_t expiredPreset = savedPreset_;
  savedPreset_ = -1;
  drawControl(expiredPreset);
  flushDisplay();
}
