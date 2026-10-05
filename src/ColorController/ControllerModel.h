#pragma once

#include "ColorMath.h"
#include "Config.h"

/** Selects the algorithm used to produce the current LED frame. */
enum class OutputMode : uint8_t {
  kSolid,
  kRainbow,
  kMusic,
};

/**
 * Owns user-visible controller state independently of display and LED hardware.
 *
 * The selected color and brightness are preserved while power is off. Storage
 * adapters may restore selected colors and presets during startup.
 */
class ControllerModel {
 public:
  static constexpr uint8_t kPresetCount = 4;

  /** Creates the model with a red selection and four useful default presets. */
  ControllerModel() : selected_({255, 0, 0}), brightness_(Config::kInitialBrightness), powerOn_(true) {
    presets_[0] = {255, 0, 0};
    presets_[1] = {255, 128, 0};
    presets_[2] = {255, 255, 0};
    presets_[3] = {0, 255, 0};
  }

  /** @return The stable solid color selection, which remains valid in effect modes. */
  const RgbColor& selected() const { return selected_; }

  /**
   * Replaces the current color and returns the model to solid mode.
   *
   * @param color New stable color selection.
   */
  void select(const RgbColor& color) {
    selected_ = color;
    mode_ = OutputMode::kSolid;
  }

  /** @return User-selected maximum output brightness in the range 0-255. */
  uint8_t brightness() const { return brightness_; }

  /** @param brightness New absolute brightness in the range 0-255. */
  void setBrightness(uint8_t brightness) { brightness_ = brightness; }

  /** @return Whether LED output is enabled. */
  bool powerOn() const { return powerOn_; }

  /** Toggles output without discarding the selected color, brightness, or mode. */
  void togglePower() { powerOn_ = !powerOn_; }

  /** @return The active solid or effect output mode. */
  OutputMode mode() const { return mode_; }

  /**
   * Selects an output algorithm without altering the stable solid color.
   *
   * @param mode Mode to activate.
   */
  void setMode(OutputMode mode) { mode_ = mode; }

  /** Returns a preset by index; callers must pass an index below kPresetCount. */
  const RgbColor& preset(uint8_t index) const { return presets_[index]; }

  /**
   * Replaces one preset when the index is valid.
   *
   * Invalid indices are ignored because restored persistence data is optional
   * and must not corrupt adjacent model storage.
   *
   * @param index Zero-based preset index.
   * @param color New preset color.
   */
  void setPreset(uint8_t index, const RgbColor& color) {
    if (index < kPresetCount) {
      presets_[index] = color;
    }
  }

  /**
   * Copies the current stable selection into a valid preset slot.
   *
   * @param index Zero-based preset index; invalid values are ignored.
   */
  void storePreset(uint8_t index) { setPreset(index, selected_); }

 private:
  RgbColor selected_;
  RgbColor presets_[kPresetCount];
  uint8_t brightness_;
  bool powerOn_;
  OutputMode mode_ = OutputMode::kSolid;
};
