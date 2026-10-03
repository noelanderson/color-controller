#pragma once

#include "ColorMath.h"
#include "Config.h"

enum class OutputMode : uint8_t {
  kSolid,
  kRainbow,
  kMusic,
};

/**
 * Owns user-visible controller state independently of display and LED hardware.
 *
 * The selected color and brightness are preserved while power is off. Presets
 * are intentionally RAM-only in v1 and return to defaults after a reset.
 */
class ControllerModel {
 public:
  static constexpr uint8_t kPresetCount = 4;
  static constexpr uint8_t kControlCount = 6;
  static constexpr uint8_t kRainbowControlIndex = 4;
  static constexpr uint8_t kMusicControlIndex = 5;

  /** Creates the model with a red selection and four useful default presets. */
  ControllerModel()
      : selected_({255, 0, 0}), brightness_(Config::kInitialBrightness), powerOn_(true) {
    presets_[0] = {255, 0, 0};
    presets_[1] = {255, 128, 0};
    presets_[2] = {255, 255, 0};
    presets_[3] = {0, 255, 0};
  }

  const RgbColor& selected() const {
    return selected_;
  }

  /** Replaces the current color without changing power or brightness. */
  void select(const RgbColor& color) {
    selected_ = color;
    mode_ = OutputMode::kSolid;
  }

  uint8_t brightness() const {
    return brightness_;
  }

  void setBrightness(uint8_t brightness) {
    brightness_ = brightness;
  }

  bool powerOn() const {
    return powerOn_;
  }

  void togglePower() {
    powerOn_ = !powerOn_;
  }

  OutputMode mode() const {
    return mode_;
  }

  void setMode(OutputMode mode) {
    mode_ = mode;
  }

  /** Returns a preset by index; callers must pass an index below kPresetCount. */
  const RgbColor& preset(uint8_t index) const {
    return presets_[index];
  }

  /** Copies the current selection into a valid preset slot. */
  void storePreset(uint8_t index) {
    if (index < kPresetCount) {
      presets_[index] = selected_;
    }
  }

 private:
  RgbColor selected_;
  RgbColor presets_[kPresetCount];
  uint8_t brightness_;
  bool powerOn_;
  OutputMode mode_ = OutputMode::kSolid;
};
