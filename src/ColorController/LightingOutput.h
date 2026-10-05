#pragma once

#include <AddressableLedStrip.h>
#include "ColorMath.h"
#include "ControllerModel.h"
#include "ReactiveLighting.h"

/**
 * Drives onboard and optional external addressable LEDs for every output mode.
 *
 * The constructor is hardware-safe; begin() performs GPIO initialization.
 * One lighting coroutine owns all runtime calls, including the adaptive music
 * envelope and the preview color consumed by the UI coroutine.
 */
class LightingOutput {
 public:
  /**
   * Binds the model and constructs both strip transports without touching GPIO.
   *
   * @param model State source that must outlive this output adapter.
   */
  explicit LightingOutput(ControllerModel& model);

  /** Initializes both configured strips; failures are reported and later writes become no-ops. */
  void begin();
  /**
   * Applies the current model using millis() as the effect timestamp.
   *
   * Solid mode writes the selected color. Rainbow and music modes derive a
   * frame and update previewColor(). Power-off writes black while preserving
   * all model state.
   */
  void apply();
  /**
   * Applies the current model using an explicit rollover-safe timestamp.
   *
   * @param now Current millis() timestamp used by effect calculations.
   */
  void apply(uint32_t now);
  /** Clears adaptive microphone history when entering music mode. */
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
