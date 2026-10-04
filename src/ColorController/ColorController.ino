#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ST77922.h>
#include <ST77922_Touch.h>
#include <TFT_eSPI.h>

#include "AudioFeedback.h"
#include "AwaitConfig.h"
#include "ColorMath.h"
#include "Config.h"
#include "ControllerModel.h"
#include "PersistencePolicy.h"
#include "PersistentState.h"
#include "PresetGesture.h"
#include "ReactiveLighting.h"

// Screen-space dimensions, hit regions, and RGB565 theme colors.
namespace Ui {

constexpr int16_t kWidth = 480;
constexpr int16_t kHeight = 320;
constexpr int16_t kColorStripHeight = 14;
constexpr int16_t kWheelCenterX = 120;
constexpr int16_t kWheelCenterY = 132;
constexpr int16_t kWheelRadius = 105;
constexpr int16_t kPresetWidth = 96;
constexpr int16_t kPresetHeight = 56;
constexpr int16_t kPresetX[2] = {260, 370};
constexpr int16_t kPresetY[3] = {24, 91, 158};
constexpr int16_t kPowerX = 12;
constexpr int16_t kPowerY = 262;
constexpr int16_t kPowerWidth = 88;
constexpr int16_t kPowerHeight = 46;
constexpr int16_t kSliderStartX = 128;
constexpr int16_t kSliderEndX = 462;
constexpr int16_t kSliderY = 286;
constexpr uint16_t kBackground = 0x1082;
constexpr uint16_t kPanel = 0x2104;
constexpr uint16_t kMuted = 0x7BEF;
constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kBlack = 0x0000;
constexpr uint16_t kGreen = 0x0600;
constexpr uint16_t kRed = 0xB800;
constexpr uint16_t kActive = 0x07E0;
constexpr uint16_t kMusic = 0x4010;

}  // namespace Ui

/** Control that captured the current single-touch gesture. */
enum class TouchTarget : uint8_t {
  kNone,
  kWheel,
  kPreset,
  kPower,
  kBrightness,
};

/** Per-contact routing information retained until a debounced release. */
struct TouchState {
  TouchTarget target = TouchTarget::kNone;
  uint8_t presetIndex = 0;
  int16_t lastX = 0;
  int16_t lastY = 0;
};

// Hardware adapters and the PSRAM-backed software framebuffer.
TFT_eSPI frameBufferHost;
TFT_eSprite canvas(&frameBufferHost);
ST77922 display;
ST77922_TOUCH touch;
Adafruit_NeoPixel onboardPixel(Config::kOnboardPixelCount, Config::kOnboardPixelPin,
                              NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel externalPixels(Config::kExternalPixelCount, Config::kExternalPixelPin,
                                NEO_GRB + NEO_KHZ800);
ControllerModel model;
TouchState touchState;
PresetGesture presetGesture;
MusicEnvelope musicEnvelope;
PersistentState persistentState;
ManualColorSaveTracker manualColorSave;
RgbColor pendingPresetColors[ControllerModel::kPresetCount] = {};
bool pendingPresetSaves[ControllerModel::kPresetCount] = {};
uint32_t presetSaveRetryAt[ControllerModel::kPresetCount] = {};
bool wasTouched = false;
uint8_t noTouchPolls = 0;
int8_t savedPreset = -1;
uint32_t savedFeedbackUntil = 0;
int16_t markerX = -1;
int16_t markerY = -1;
RgbColor effectPreviewColor = {255, 0, 0};

// Converts model RGB values to the display framebuffer's RGB565 format.
uint16_t toRgb565(const RgbColor& color) {
  return canvas.color565(color.red, color.green, color.blue);
}

bool contains(int16_t x, int16_t y, int16_t left, int16_t top, int16_t width,
              int16_t height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

bool presetContains(uint8_t index, int16_t x, int16_t y) {
  const uint8_t column = index % 2;
  const uint8_t row = index / 2;
  return contains(x, y, Ui::kPresetX[column], Ui::kPresetY[row], Ui::kPresetWidth,
                  Ui::kPresetHeight);
}

void flushDisplay() {
  // Rotation 1 in the vendor driver requires a full-frame transfer from (0, 0).
  display.Fill_Colors(0, 0, Ui::kWidth, Ui::kHeight,
                      static_cast<uint16_t*>(canvas.getPointer()));
}

void writeSolidPixels(Adafruit_NeoPixel& pixels, const RgbColor& color,
                      uint8_t brightness) {
  pixels.setBrightness(brightness);
  for (uint16_t index = 0; index < pixels.numPixels(); ++index) {
    pixels.setPixelColor(index, color.red, color.green, color.blue);
  }
  pixels.show();
}

void writeRainbowPixels(Adafruit_NeoPixel& pixels, uint32_t now,
                        uint8_t brightness) {
  pixels.setBrightness(brightness);
  for (uint16_t index = 0; index < pixels.numPixels(); ++index) {
    const RgbColor color =
        rainbowColor(now, Config::kRainbowCycleMs, index, pixels.numPixels());
    pixels.setPixelColor(index, color.red, color.green, color.blue);
  }
  pixels.show();
}

void applyPixelOutput(uint32_t now = millis()) {
  if (!model.powerOn()) {
    writeSolidPixels(onboardPixel, {0, 0, 0}, 0);
    if (Config::kExternalPixelCount > 0) {
      writeSolidPixels(externalPixels, {0, 0, 0}, 0);
    }
    return;
  }

  if (model.mode() == OutputMode::kRainbow) {
    const uint8_t brightness = scaleBrightness(
        model.brightness(), breathingIntensity(now, Config::kRainbowBreathMs));
    effectPreviewColor = rainbowColor(now, Config::kRainbowCycleMs, 0, 1);
    writeRainbowPixels(onboardPixel, now, brightness);
    if (Config::kExternalPixelCount > 0) {
      writeRainbowPixels(externalPixels, now, brightness);
    }
    return;
  }

  if (model.mode() == OutputMode::kMusic) {
    uint16_t magnitude = 0;
    if (AudioFeedback::readMicrophoneLevel(magnitude)) {
      musicEnvelope.update(magnitude);
    }
    const uint8_t level = musicEnvelope.level();
    const uint8_t brightness =
        scaleBrightness(model.brightness(), musicIntensity(level));
    const RgbColor color = musicColor(now, level);
    effectPreviewColor = color;
    writeSolidPixels(onboardPixel, color, brightness);
    if (Config::kExternalPixelCount > 0) {
      writeSolidPixels(externalPixels, color, brightness);
    }
    return;
  }

  effectPreviewColor = model.selected();
  writeSolidPixels(onboardPixel, model.selected(), model.brightness());
  if (Config::kExternalPixelCount > 0) {
    writeSolidPixels(externalPixels, model.selected(), model.brightness());
  }
}

void drawCenteredText(const char* text, int16_t centerX, int16_t centerY,
                      uint16_t foreground, uint16_t background, uint8_t size = 2) {
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(foreground, background);
  canvas.setTextSize(size);
  canvas.drawString(text, centerX, centerY);
}

void drawColorStrip() {
  const RgbColor& color =
      model.mode() == OutputMode::kSolid ? model.selected() : effectPreviewColor;
  canvas.fillRect(0, 0, Ui::kWidth, Ui::kColorStripHeight, toRgb565(color));
  canvas.drawFastHLine(0, Ui::kColorStripHeight - 1, Ui::kWidth, Ui::kWhite);
}

void drawColorWheel() {
  for (int16_t y = Ui::kWheelCenterY - Ui::kWheelRadius;
       y <= Ui::kWheelCenterY + Ui::kWheelRadius; ++y) {
    for (int16_t x = Ui::kWheelCenterX - Ui::kWheelRadius;
         x <= Ui::kWheelCenterX + Ui::kWheelRadius; ++x) {
      RgbColor color;
      if (colorFromWheel(x, y, Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius,
                         color)) {
        canvas.drawPixel(x, y, toRgb565(color));
      }
    }
  }
  canvas.drawCircle(Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius,
                    Ui::kWhite);
}

void restoreMarkerBackground() {
  if (markerX < 0 || markerY < 0) {
    return;
  }
  // Recompute only the old marker's small footprint; rebuilding the complete
  // wheel during a drag is too expensive on the ESP32-S3.
  for (int16_t y = markerY - 8; y <= markerY + 8; ++y) {
    for (int16_t x = markerX - 8; x <= markerX + 8; ++x) {
      RgbColor color;
      if (colorFromWheel(x, y, Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius,
                         color)) {
        canvas.drawPixel(x, y, toRgb565(color));
      } else if (x >= 0 && x < Ui::kWidth && y >= Ui::kColorStripHeight &&
                 y < Ui::kHeight) {
        canvas.drawPixel(x, y, Ui::kBackground);
      }
    }
  }
  canvas.drawCircle(Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius,
                    Ui::kWhite);
}

void drawWheelMarker() {
  restoreMarkerBackground();
  const HsvColor hsv = rgbToHsv(model.selected());
  const float distance = static_cast<float>(hsv.saturation) * Ui::kWheelRadius / 255.0f;
  const float angle = static_cast<float>(hsv.hue) * kPi / 180.0f;
  markerX = Ui::kWheelCenterX + lroundf(cosf(angle) * distance);
  markerY = Ui::kWheelCenterY + lroundf(sinf(angle) * distance);
  const uint16_t outline =
      (model.selected().red + model.selected().green + model.selected().blue) > 420
          ? Ui::kBlack
          : Ui::kWhite;
  canvas.drawCircle(markerX, markerY, 6, outline);
  canvas.drawCircle(markerX, markerY, 7, outline);
}

void drawControl(uint8_t index) {
  const uint8_t column = index % 2;
  const uint8_t row = index / 2;
  const int16_t x = Ui::kPresetX[column];
  const int16_t y = Ui::kPresetY[row];

  if (index == ControllerModel::kRainbowControlIndex) {
    canvas.fillRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, 8, Ui::kBlack);
    constexpr uint8_t kDots = 6;
    for (uint8_t dot = 0; dot < kDots; ++dot) {
      const RgbColor color =
          hsvToRgb({static_cast<uint16_t>(dot * 300 / (kDots - 1)), 255, 255});
      const int16_t dotX = x + 18 + dot * 12;
      const int16_t distanceFromCenter = abs(static_cast<int16_t>(dot) * 2 -
                                             (kDots - 1));
      const int16_t dotY = y + 25 + distanceFromCenter;
      canvas.fillCircle(dotX, dotY, 7, toRgb565(color));
    }
    const uint16_t outline =
        model.mode() == OutputMode::kRainbow ? Ui::kActive : Ui::kWhite;
    canvas.drawRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, 8, outline);
    if (model.mode() == OutputMode::kRainbow) {
      canvas.drawRoundRect(x + 2, y + 2, Ui::kPresetWidth - 4,
                           Ui::kPresetHeight - 4, 6, outline);
    }
    return;
  }

  if (index == ControllerModel::kMusicControlIndex) {
    const uint16_t outline =
        model.mode() == OutputMode::kMusic ? Ui::kActive : Ui::kWhite;
    canvas.fillRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, 8, Ui::kMusic);
    canvas.drawRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, 8, outline);
    if (model.mode() == OutputMode::kMusic) {
      canvas.drawRoundRect(x + 2, y + 2, Ui::kPresetWidth - 4,
                           Ui::kPresetHeight - 4, 6, outline);
    }
    const int16_t headX = x + Ui::kPresetWidth / 2 - 7;
    const int16_t headY = y + 37;
    const int16_t stemX = headX + 6;
    const int16_t stemTop = y + 12;
    canvas.fillCircle(headX, headY, 8, Ui::kWhite);
    canvas.fillRect(stemX, stemTop, 4, headY - stemTop, Ui::kWhite);
    for (uint8_t offset = 0; offset < 4; ++offset) {
      canvas.drawLine(stemX + 3, stemTop + offset, stemX + 20,
                      stemTop + 8 + offset, Ui::kWhite);
    }
    if (!AudioFeedback::microphoneAvailable()) {
      canvas.fillCircle(x + Ui::kPresetWidth - 10, y + 10, 4, Ui::kRed);
    }
    return;
  }

  const RgbColor color = model.preset(index);
  const uint16_t fill = toRgb565(color);
  const uint16_t text =
      (static_cast<uint16_t>(color.red) * 299 + static_cast<uint16_t>(color.green) * 587 +
       static_cast<uint16_t>(color.blue) * 114) > 140000
          ? Ui::kBlack
          : Ui::kWhite;

  canvas.fillRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, 8, fill);
  canvas.drawRoundRect(x, y, Ui::kPresetWidth, Ui::kPresetHeight, 8, Ui::kWhite);
  char label[8];
  if (savedPreset == index &&
      static_cast<int32_t>(millis() - savedFeedbackUntil) < 0) {
    snprintf(label, sizeof(label), "SAVED");
  } else {
    snprintf(label, sizeof(label), "P%u", index + 1);
  }
  drawCenteredText(label, x + Ui::kPresetWidth / 2, y + Ui::kPresetHeight / 2, text,
                   fill);
}

void drawControls() {
  for (uint8_t index = 0; index < ControllerModel::kControlCount; ++index) {
    drawControl(index);
  }
}

void drawPowerControl() {
  const uint16_t fill = model.powerOn() ? Ui::kRed : Ui::kGreen;
  canvas.fillRoundRect(Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight, 9,
                       fill);
  canvas.drawRoundRect(Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight, 9,
                       Ui::kWhite);
  drawCenteredText(model.powerOn() ? "OFF" : "ON", Ui::kPowerX + Ui::kPowerWidth / 2,
                   Ui::kPowerY + Ui::kPowerHeight / 2, Ui::kWhite, fill);
}

void drawBrightnessControl() {
  canvas.fillRect(Ui::kSliderStartX - 12, 252,
                  Ui::kSliderEndX - Ui::kSliderStartX + 25, 62, Ui::kBackground);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(Ui::kWhite, Ui::kBackground);
  canvas.setTextSize(1);
  canvas.drawString("BRIGHTNESS", Ui::kSliderStartX, 255);

  char value[5];
  snprintf(value, sizeof(value), "%u", model.brightness());
  canvas.setTextDatum(TR_DATUM);
  canvas.drawString(value, Ui::kSliderEndX, 255);

  canvas.fillRoundRect(Ui::kSliderStartX, Ui::kSliderY - 4,
                       Ui::kSliderEndX - Ui::kSliderStartX, 9, 4, Ui::kMuted);
  const int16_t knobX =
      Ui::kSliderStartX +
      static_cast<int32_t>(model.brightness()) *
          (Ui::kSliderEndX - Ui::kSliderStartX) / 255;
  const RgbColor& color =
      model.mode() == OutputMode::kSolid ? model.selected() : effectPreviewColor;
  canvas.fillCircle(knobX, Ui::kSliderY, 11, toRgb565(color));
  canvas.drawCircle(knobX, Ui::kSliderY, 11, Ui::kWhite);
}

void drawDynamicUi() {
  drawColorStrip();
  drawWheelMarker();
  drawControls();
  drawPowerControl();
  drawBrightnessControl();
  flushDisplay();
}

void drawInitialUi() {
  canvas.fillSprite(Ui::kBackground);
  canvas.fillRoundRect(248, 18, 226, 207, 10, Ui::kPanel);
  drawColorWheel();
  drawDynamicUi();
}

void selectWheelColor(int16_t x, int16_t y) {
  RgbColor color;
  if (!colorFromWheel(x, y, Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius,
                      color) ||
      (color == model.selected() && model.mode() == OutputMode::kSolid)) {
    return;
  }
  model.select(color);
  const uint32_t now = millis();
  manualColorSave.noteChange(color, now);
  applyPixelOutput(now);
  drawDynamicUi();
}

void setBrightnessFromTouch(int16_t x) {
  const uint8_t brightness =
      brightnessFromX(x, Ui::kSliderStartX, Ui::kSliderEndX);
  if (brightness == model.brightness()) {
    return;
  }
  model.setBrightness(brightness);
  applyPixelOutput();
  drawBrightnessControl();
  flushDisplay();
}

TouchTarget identifyTarget(int16_t x, int16_t y, uint8_t& presetIndex) {
  RgbColor ignored;
  if (colorFromWheel(x, y, Ui::kWheelCenterX, Ui::kWheelCenterY, Ui::kWheelRadius,
                     ignored)) {
    return TouchTarget::kWheel;
  }
  for (uint8_t index = 0; index < ControllerModel::kControlCount; ++index) {
    if (presetContains(index, x, y)) {
      presetIndex = index;
      return TouchTarget::kPreset;
    }
  }
  if (contains(x, y, Ui::kPowerX, Ui::kPowerY, Ui::kPowerWidth, Ui::kPowerHeight)) {
    return TouchTarget::kPower;
  }
  if (contains(x, y, Ui::kSliderStartX - 12, Ui::kSliderY - 22,
               Ui::kSliderEndX - Ui::kSliderStartX + 24, 44)) {
    return TouchTarget::kBrightness;
  }
  return TouchTarget::kNone;
}

void handleTouchDown(int16_t x, int16_t y, uint32_t now) {
  touchState = {};
  touchState.target = identifyTarget(x, y, touchState.presetIndex);
  touchState.lastX = x;
  touchState.lastY = y;

  if (touchState.target == TouchTarget::kPreset &&
      touchState.presetIndex < ControllerModel::kPresetCount) {
    presetGesture.begin(now);
  } else if (touchState.target == TouchTarget::kWheel) {
    selectWheelColor(x, y);
  } else if (touchState.target == TouchTarget::kBrightness) {
    setBrightnessFromTouch(x);
  }
}

void handleTouchMove(int16_t x, int16_t y) {
  touchState.lastX = x;
  touchState.lastY = y;
  if (touchState.target == TouchTarget::kWheel) {
    selectWheelColor(x, y);
  } else if (touchState.target == TouchTarget::kBrightness) {
    setBrightnessFromTouch(x);
  }
}

void updatePresetHold(uint32_t now) {
  if (touchState.target == TouchTarget::kPreset &&
      touchState.presetIndex < ControllerModel::kPresetCount &&
      presetGesture.update(now,
                           presetContains(touchState.presetIndex, touchState.lastX,
                                          touchState.lastY),
                           Config::kPresetHoldMs) == PresetGestureEvent::kStore) {
    model.storePreset(touchState.presetIndex);
    model.setMode(OutputMode::kSolid);
    manualColorSave.noteChange(model.selected(), now);
    pendingPresetColors[touchState.presetIndex] = model.selected();
    pendingPresetSaves[touchState.presetIndex] = true;
    presetSaveRetryAt[touchState.presetIndex] = now;
    savedPreset = touchState.presetIndex;
    savedFeedbackUntil = now + Config::kSavedFeedbackMs;
    applyPixelOutput(now);
    drawDynamicUi();
    AudioFeedback::beepLong();
  }
}

void handleTouchUp() {
  if (touchState.target == TouchTarget::kPreset &&
      touchState.presetIndex < ControllerModel::kPresetCount &&
      presetGesture.release(presetContains(touchState.presetIndex, touchState.lastX,
                                           touchState.lastY)) ==
          PresetGestureEvent::kRecall) {
    const RgbColor preset = model.preset(touchState.presetIndex);
    if (preset != model.selected() || model.mode() != OutputMode::kSolid) {
      model.select(preset);
      applyPixelOutput();
      drawDynamicUi();
      manualColorSave.noteChange(preset, millis());
    }
    AudioFeedback::beep();
  } else if (touchState.target == TouchTarget::kPreset &&
             presetContains(touchState.presetIndex, touchState.lastX,
                            touchState.lastY) &&
             (touchState.presetIndex == ControllerModel::kRainbowControlIndex ||
              touchState.presetIndex == ControllerModel::kMusicControlIndex)) {
    const OutputMode mode =
        touchState.presetIndex == ControllerModel::kRainbowControlIndex
            ? OutputMode::kRainbow
            : OutputMode::kMusic;
    model.setMode(mode);
    manualColorSave.cancel();
    if (mode == OutputMode::kMusic) {
      musicEnvelope.reset();
    }
    applyPixelOutput();
    drawDynamicUi();
    AudioFeedback::beep();
  } else if (touchState.target == TouchTarget::kPower &&
             contains(touchState.lastX, touchState.lastY, Ui::kPowerX, Ui::kPowerY,
                      Ui::kPowerWidth, Ui::kPowerHeight)) {
    model.togglePower();
    applyPixelOutput();
    drawPowerControl();
    flushDisplay();
    AudioFeedback::beep();
  } else {
    presetGesture.reset();
  }
  touchState = {};
}

simpleawait::Task<void> monitorTouchInput() {
  while (true) {
    const uint32_t now = millis();
    const bool isTouched = touch.Get_Touch();
    if (isTouched) {
      const int16_t x =
          constrain(static_cast<int16_t>(touch.touch.x[0]), 0, Ui::kWidth - 1);
      const int16_t y =
          constrain(static_cast<int16_t>(touch.touch.y[0]), 0, Ui::kHeight - 1);
      if (!wasTouched) {
        handleTouchDown(x, y, now);
      } else {
        handleTouchMove(x, y);
      }
      noTouchPolls = 0;
      wasTouched = true;
    } else if (wasTouched && ++noTouchPolls >= Config::kReleaseDebouncePolls) {
      handleTouchUp();
      wasTouched = false;
      noTouchPolls = 0;
    }
    updatePresetHold(now);

    if (savedPreset >= 0 &&
        static_cast<int32_t>(now - savedFeedbackUntil) >= 0) {
      const uint8_t expiredPreset = savedPreset;
      savedPreset = -1;
      drawControl(expiredPreset);
      flushDisplay();
    }

    uint8_t error = 0;
    if (AwaitStatus::take(error)) {
      Serial.printf("WARNING: coroutine scheduler error %u\n", error);
    }
    co_await simpleawait::delay_ms(Config::kTouchPollMs);
  }
}

simpleawait::Task<void> updateEffects() {
  while (true) {
    co_await simpleawait::delay_ms(Config::kEffectFrameMs);
    if (model.powerOn() && model.mode() != OutputMode::kSolid) {
      applyPixelOutput();
    }
  }
}

simpleawait::Task<void> updateEffectUi() {
  while (true) {
    co_await simpleawait::delay_ms(Config::kEffectUiFrameMs);
    if (model.mode() != OutputMode::kSolid) {
      drawColorStrip();
      drawBrightnessControl();
      flushDisplay();
    }
  }
}

simpleawait::Task<void> persistStableManualColor() {
  while (true) {
    co_await simpleawait::delay_ms(Config::kPersistencePollMs);
    const uint32_t now = millis();
    for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
      if (!pendingPresetSaves[index] ||
          static_cast<int32_t>(now - presetSaveRetryAt[index]) < 0) {
        continue;
      }
      if (persistentState.savePreset(index, pendingPresetColors[index])) {
        pendingPresetSaves[index] = false;
      } else {
        Serial.printf("WARNING: unable to persist preset P%u\n", index + 1);
        presetSaveRetryAt[index] = now + Config::kPersistenceRetryMs;
      }
    }

    if (!manualColorSave.ready(now, Config::kManualColorSaveDelayMs)) {
      continue;
    }
    if (model.mode() != OutputMode::kSolid ||
        model.selected() != manualColorSave.color()) {
      manualColorSave.cancel();
      continue;
    }
    const RgbColor color = manualColorSave.color();
    if (!persistentState.saveSelectedColor(color)) {
      Serial.println("WARNING: unable to persist selected color");
      manualColorSave.noteChange(color, now);
    } else {
      manualColorSave.markSaved();
    }
  }
}

simpleawait::Task<void> reportFramebufferFailure() {
  while (true) {
    Serial.println("FATAL: unable to allocate the display framebuffer");
    co_await simpleawait::delay_ms(1000);
  }
}

simpleawait::Task<void> reportDisplayFailure() {
  while (true) {
    Serial.println("FATAL: display initialization failed");
    co_await simpleawait::delay_ms(1000);
  }
}

bool startControllerTask(simpleawait::Task<void>&& task, const char* name) {
  const simpleawait::TaskHandle handle =
      simpleawait::create_task(static_cast<simpleawait::Task<void>&&>(task));
  if (handle.valid()) {
    return true;
  }
  Serial.printf("FATAL: unable to start %s coroutine\n", name);
  return false;
}

void setup() {
  Serial.begin(115200);

  if (!display.begin()) {
    startControllerTask(reportDisplayFailure(), "display failure reporter");
    return;
  }
  display.Set_Rotation(Config::kDisplayRotation);

  if (!persistentState.begin(model)) {
    Serial.println("WARNING: persistent color storage unavailable");
  }

  canvas.setColorDepth(16);
  if (canvas.createSprite(Ui::kWidth, Ui::kHeight) == nullptr) {
    startControllerTask(reportFramebufferFailure(), "framebuffer failure reporter");
    return;
  }
  canvas.setSwapBytes(true);

  onboardPixel.begin();
  onboardPixel.clear();
  if (Config::kExternalPixelCount > 0) {
    externalPixels.begin();
    externalPixels.clear();
  }
  applyPixelOutput();
  drawInitialUi();
  display.Set_Backlight(true);

  if (!touch.init()) {
    Serial.println("WARNING: touch initialization did not reach an idle state");
  }
  touch.Set_Rotation(Config::kDisplayRotation);

  startControllerTask(monitorTouchInput(), "touch input");
  startControllerTask(updateEffects(), "LED effects");
  startControllerTask(updateEffectUi(), "effect UI");
  startControllerTask(persistStableManualColor(), "persistence");
  if (!AudioFeedback::start(g_touchI2CBus)) {
    Serial.println("FATAL: unable to start audio coroutine");
  }
}

void loop() {
  simpleawait::poll();
}
