#include "ModeButtonControl.h"

#include <Arduino.h>

#include "ColorMath.h"
#include "UiDrawing.h"
#include "UiLayout.h"

ModeButtonControl::ModeButtonControl(TFT_eSprite& canvas, UiElementId id, const Ui::Rect& bounds,
                                     OutputMode mode)
    : ButtonControl(bounds), UiElement(id), canvas_(canvas), mode_(mode) {}

UiAction ModeButtonControl::onRelease(const TouchEvent&, bool releasedInside) {
  return releasedInside ? UiAction::activateMode(mode_) : UiAction{};
}

void ModeButtonControl::draw(const UiRenderContext& context) {
  const bool active = context.model.mode() == mode_;
  if (mode_ == OutputMode::kRainbow) {
    drawRainbow(active);
  } else {
    drawMusic(active, AudioFeedback::microphoneStatus());
  }
}

void ModeButtonControl::drawActiveOutline(bool active) {
  const uint16_t outline = active ? Ui::kActive : Ui::kWhite;
  canvas_.drawRoundRect(x(), y(), width(), height(), Ui::kControlCornerRadius, outline);
  if (active) {
    canvas_.drawRoundRect(x() + Ui::kActiveControlInset, y() + Ui::kActiveControlInset,
                          width() - 2 * Ui::kActiveControlInset,
                          height() - 2 * Ui::kActiveControlInset, Ui::kActiveControlCornerRadius,
                          outline);
  }
}

void ModeButtonControl::drawRainbow(bool active) {
  canvas_.fillRoundRect(x(), y(), width(), height(), Ui::kControlCornerRadius, Ui::kBlack);
  for (uint8_t dot = 0; dot < Ui::kRainbowDotCount; ++dot) {
    const RgbColor color =
        hsvToRgb({static_cast<uint16_t>(dot * Ui::kRainbowGlyphHueSpan / (Ui::kRainbowDotCount - 1)),
                  UINT8_MAX, UINT8_MAX});
    const int16_t dotX = x() + Ui::kRainbowDotStartX + dot * Ui::kRainbowDotSpacing;
    const int16_t distanceFromCenter =
        abs(static_cast<int16_t>(dot) * Ui::kRainbowArcVerticalScale - (Ui::kRainbowDotCount - 1));
    canvas_.fillCircle(dotX, y() + Ui::kRainbowDotBaseY + distanceFromCenter, Ui::kRainbowDotRadius,
                       Ui::toRgb565(canvas_, color));
  }
  drawActiveOutline(active);
}

void ModeButtonControl::drawMusic(bool active, AudioFeedback::MicrophoneStatus microphoneStatus) {
  canvas_.fillRoundRect(x(), y(), width(), height(), Ui::kControlCornerRadius, Ui::kMusic);
  drawActiveOutline(active);

  const int16_t headX = x() + width() / 2 + Ui::kMusicHeadCenterOffsetX;
  const int16_t headY = y() + Ui::kMusicHeadCenterY;
  const int16_t stemX = headX + Ui::kMusicStemOffsetX;
  const int16_t stemTop = y() + Ui::kMusicStemTopY;
  canvas_.fillCircle(headX, headY, Ui::kMusicHeadRadius, Ui::kWhite);
  canvas_.fillRect(stemX, stemTop, Ui::kMusicStemWidth, headY - stemTop, Ui::kWhite);
  for (uint8_t offset = 0; offset < Ui::kMusicStemWidth; ++offset) {
    canvas_.drawLine(stemX + Ui::kMusicStemWidth - 1, stemTop + offset, stemX + Ui::kMusicFlagLength,
                     stemTop + Ui::kMusicFlagRise + offset, Ui::kWhite);
  }

  // The red dot means initialization failed; it is never a recording indicator.
  if (microphoneStatus == AudioFeedback::MicrophoneStatus::kUnavailable) {
    canvas_.fillCircle(x() + width() - Ui::kMicStatusInset, y() + Ui::kMicStatusInset,
                       Ui::kMicStatusRadius, Ui::kRed);
  }
}
