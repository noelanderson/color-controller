#include "UiRenderer.h"

#include <Arduino.h>

#include "AudioFeedback.h"
#include "UiDrawing.h"

UiRenderer::UiRenderer(TFT_eSprite& canvas, ST77922& display, ControllerModel& model,
                       ColorWheelControl& colorWheel, BrightnessSliderControl& brightnessSlider,
                       PowerButtonControl& powerButton, ColorPreviewControl& colorPreview)
    : canvas_(canvas),
      display_(display),
      model_(model),
      colorWheel_(colorWheel),
      brightnessSlider_(brightnessSlider),
      powerButton_(powerButton),
      colorPreview_(colorPreview) {}

void UiRenderer::flushDisplay() {
  // Rotation 1 requires a full-frame transfer beginning at the panel origin.
  display_.Fill_Colors(0, 0, Ui::kWidth, Ui::kHeight, static_cast<uint16_t*>(canvas_.getPointer()));
}

void UiRenderer::drawColorStrip(const RgbColor& effectPreviewColor) {
  const RgbColor& color = model_.mode() == OutputMode::kSolid ? model_.selected() : effectPreviewColor;
  colorPreview_.draw(color);
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
      canvas_.fillCircle(dotX, dotY, Ui::kRainbowDotRadius, Ui::toRgb565(canvas_, color));
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
  const uint16_t fill = Ui::toRgb565(canvas_, color);
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
  Ui::drawCenteredText(canvas_, label, x + Ui::kPresetWidth / 2, y + Ui::kPresetHeight / 2, text, fill,
                       Ui::kDefaultTextSize);
}

void UiRenderer::drawControls() {
  for (uint8_t index = 0; index < ControllerModel::kControlCount; ++index) {
    drawControl(index);
  }
}

void UiRenderer::drawPowerControl() { powerButton_.draw(model_.powerOn()); }

void UiRenderer::drawBrightnessControl(const RgbColor& effectPreviewColor) {
  const RgbColor& color = model_.mode() == OutputMode::kSolid ? model_.selected() : effectPreviewColor;
  brightnessSlider_.draw(model_.brightness(), color);
}

void UiRenderer::drawDynamicUi(const RgbColor& effectPreviewColor) {
  drawColorStrip(effectPreviewColor);
  colorWheel_.drawMarker(model_.selected());
  drawControls();
  drawPowerControl();
  drawBrightnessControl(effectPreviewColor);
  flushDisplay();
}

void UiRenderer::drawInitialUi(const RgbColor& effectPreviewColor) {
  canvas_.fillSprite(Ui::kBackground);
  canvas_.fillRoundRect(Ui::kPanelX, Ui::kPanelY, Ui::kPanelWidth, Ui::kPanelHeight, Ui::kPanelCornerRadius,
                        Ui::kPanel);
  colorWheel_.draw();
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
