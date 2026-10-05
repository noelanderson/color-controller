#include "LightingOutput.h"

#include <Arduino.h>

#include "AudioFeedback.h"
#include "Config.h"

LightingOutput::LightingOutput(ControllerModel& model)
    : model_(model),
      onboardPixels_(Config::kOnboardPixelPin, Config::kOnboardPixelCount),
      externalPixels_(Config::kExternalPixelPin, Config::kExternalPixelCount) {}

void LightingOutput::begin() {
  if (!onboardPixels_.begin()) {
    Serial.println("Addressable LED initialization failed for onboard pixels.");
  }
  if (!externalPixels_.begin()) {
    Serial.println("Addressable LED initialization failed for external pixels.");
  }
}

void LightingOutput::writeSolidPixels(AddressableLedStrip& pixels, const RgbColor& color,
                                      uint8_t brightness) {
  for (uint16_t index = 0; index < pixels.size(); ++index) {
    pixels.setPixel(index, color.red, color.green, color.blue, brightness);
  }
  pixels.show();
}

void LightingOutput::writeRainbowPixels(AddressableLedStrip& pixels, uint32_t now, uint8_t brightness) {
  for (uint16_t index = 0; index < pixels.size(); ++index) {
    const RgbColor color = rainbowColor(now, Config::kRainbowCycleMs, index, pixels.size());
    pixels.setPixel(index, color.red, color.green, color.blue, brightness);
  }
  pixels.show();
}

void LightingOutput::apply() { apply(millis()); }

void LightingOutput::apply(uint32_t now) {
  if (!model_.powerOn()) {
    writeSolidPixels(onboardPixels_, {0, 0, 0}, 0);
    if (Config::kExternalPixelCount > 0) {
      writeSolidPixels(externalPixels_, {0, 0, 0}, 0);
    }
    return;
  }

  if (model_.mode() == OutputMode::kRainbow) {
    const uint8_t brightness =
        scaleBrightness(model_.brightness(), breathingIntensity(now, Config::kRainbowBreathMs));
    previewColor_ = rainbowColor(now, Config::kRainbowCycleMs, 0, 1);
    writeRainbowPixels(onboardPixels_, now, brightness);
    if (Config::kExternalPixelCount > 0) {
      writeRainbowPixels(externalPixels_, now, brightness);
    }
    return;
  }

  if (model_.mode() == OutputMode::kMusic) {
    uint16_t magnitude = 0;
    if (AudioFeedback::readMicrophoneLevel(magnitude)) {
      musicEnvelope_.update(magnitude);
    }
    const uint8_t level = musicEnvelope_.level();
    const uint8_t brightness = scaleBrightness(model_.brightness(), musicIntensity(level));
    const RgbColor color = musicColor(now, level);
    previewColor_ = color;
    writeSolidPixels(onboardPixels_, color, brightness);
    if (Config::kExternalPixelCount > 0) {
      writeSolidPixels(externalPixels_, color, brightness);
    }
    return;
  }

  previewColor_ = model_.selected();
  writeSolidPixels(onboardPixels_, model_.selected(), model_.brightness());
  if (Config::kExternalPixelCount > 0) {
    writeSolidPixels(externalPixels_, model_.selected(), model_.brightness());
  }
}

void LightingOutput::resetMusicEnvelope() { musicEnvelope_.reset(); }

const RgbColor& LightingOutput::previewColor() const { return previewColor_; }
