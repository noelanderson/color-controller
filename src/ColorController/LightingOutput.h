#pragma once

#include "AddressableLedStrip.h"
#include "ColorMath.h"
#include "ControllerModel.h"
#include "ReactiveLighting.h"

/**
 * Drives onboard and optional external addressable LEDs for every output mode.
 *
 * The constructor is hardware-safe; begin() performs GPIO initialization.
 */
class LightingOutput {
 public:
  explicit LightingOutput(ControllerModel& model);

  void begin();
  void apply();
  void apply(uint32_t now);
  void resetMusicEnvelope();

  /** Returns the live effect color used by the UI strip and slider knob. */
  const RgbColor& previewColor() const;

 private:
  void writeSolidPixels(AddressableLedStrip& pixels, const RgbColor& color, uint8_t brightness);
  void writeRainbowPixels(AddressableLedStrip& pixels, uint32_t now, uint8_t brightness);

  ControllerModel& model_;
  AddressableLedStrip onboardPixels_;
  AddressableLedStrip externalPixels_;
  MusicEnvelope musicEnvelope_;
  RgbColor previewColor_ = {255, 0, 0};
};
