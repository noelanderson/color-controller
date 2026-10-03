#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ST77922.h>
#include <ST77922_Touch.h>
#include <TFT_eSPI.h>

#include "ColorMath.h"
#include "Config.h"
#include "ControllerModel.h"
#include "PresetGesture.h"

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
Adafruit_NeoPixel pixels(Config::kPixelCount, Config::kPixelPin, NEO_GRB + NEO_KHZ800);
ControllerModel model;
TouchState touchState;
PresetGesture presetGesture;
bool wasTouched = false;
uint8_t noTouchPolls = 0;
int8_t savedPreset = -1;
uint32_t savedFeedbackUntil = 0;
int16_t markerX = -1;
int16_t markerY = -1;

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

void applyPixelOutput() {
  // Set brightness before rewriting pixels because Adafruit_NeoPixel rescales
  // its existing buffer when brightness changes.
  pixels.setBrightness(model.brightness());
  const RgbColor output = model.powerOn() ? model.selected() : RgbColor{0, 0, 0};
  for (uint16_t index = 0; index < pixels.numPixels(); ++index) {
    pixels.setPixelColor(index, output.red, output.green, output.blue);
  }
  pixels.show();
}

void drawCenteredText(const char* text, int16_t centerX, int16_t centerY,
                      uint16_t foreground, uint16_t background, uint8_t size = 2) {
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(foreground, background);
  canvas.setTextSize(size);
  canvas.drawString(text, centerX, centerY);
}

void drawColorStrip() {
  canvas.fillRect(0, 0, Ui::kWidth, Ui::kColorStripHeight, toRgb565(model.selected()));
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

void drawPreset(uint8_t index) {
  const uint8_t column = index % 2;
  const uint8_t row = index / 2;
  const int16_t x = Ui::kPresetX[column];
  const int16_t y = Ui::kPresetY[row];
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

void drawPresets() {
  for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
    drawPreset(index);
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
  canvas.fillCircle(knobX, Ui::kSliderY, 11, toRgb565(model.selected()));
  canvas.drawCircle(knobX, Ui::kSliderY, 11, Ui::kWhite);
}

void drawDynamicUi() {
  drawColorStrip();
  drawWheelMarker();
  drawPresets();
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
      color == model.selected()) {
    return;
  }
  model.select(color);
  applyPixelOutput();
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
  for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
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

  if (touchState.target == TouchTarget::kPreset) {
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
      presetGesture.update(now,
                           presetContains(touchState.presetIndex, touchState.lastX,
                                          touchState.lastY),
                           Config::kPresetHoldMs) == PresetGestureEvent::kStore) {
    model.storePreset(touchState.presetIndex);
    const int8_t previousSavedPreset = savedPreset;
    savedPreset = touchState.presetIndex;
    savedFeedbackUntil = now + Config::kSavedFeedbackMs;
    if (previousSavedPreset >= 0 && previousSavedPreset != savedPreset) {
      drawPreset(previousSavedPreset);
    }
    drawPreset(touchState.presetIndex);
    flushDisplay();
  }
}

void handleTouchUp() {
  if (touchState.target == TouchTarget::kPreset &&
      presetGesture.release(presetContains(touchState.presetIndex, touchState.lastX,
                                           touchState.lastY)) ==
          PresetGestureEvent::kRecall) {
    const RgbColor preset = model.preset(touchState.presetIndex);
    if (preset != model.selected()) {
      model.select(preset);
      applyPixelOutput();
      drawDynamicUi();
    }
  } else if (touchState.target == TouchTarget::kPower &&
             contains(touchState.lastX, touchState.lastY, Ui::kPowerX, Ui::kPowerY,
                      Ui::kPowerWidth, Ui::kPowerHeight)) {
    model.togglePower();
    applyPixelOutput();
    drawPowerControl();
    flushDisplay();
  } else {
    presetGesture.reset();
  }
  touchState = {};
}

void setup() {
  Serial.begin(115200);

  display.Set_Rotation(Config::kDisplayRotation);
  touch.init();
  touch.Set_Rotation(Config::kDisplayRotation);

  canvas.setColorDepth(16);
  if (canvas.createSprite(Ui::kWidth, Ui::kHeight) == nullptr) {
    Serial.println("FATAL: unable to allocate the display framebuffer");
    while (true) {
      delay(1000);
    }
  }
  canvas.setSwapBytes(true);

  pixels.begin();
  pixels.clear();
  applyPixelOutput();
  drawInitialUi();
}

void loop() {
  const uint32_t now = millis();
  const bool isTouched = touch.Get_Touch();
  if (isTouched) {
    const int16_t x = constrain(static_cast<int16_t>(touch.touch.x[0]), 0, Ui::kWidth - 1);
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
    drawPreset(expiredPreset);
    flushDisplay();
  }
  delay(5);
}
