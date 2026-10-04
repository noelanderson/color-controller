#pragma once

#include <stdint.h>

#include "ColorMath.h"

constexpr uint32_t packColor(const RgbColor& color) {
  return (static_cast<uint32_t>(color.red) << 16) |
         (static_cast<uint32_t>(color.green) << 8) | color.blue;
}

constexpr RgbColor unpackColor(uint32_t packed) {
  return {
      static_cast<uint8_t>(packed >> 16),
      static_cast<uint8_t>(packed >> 8),
      static_cast<uint8_t>(packed),
  };
}

class ManualColorSaveTracker {
 public:
  void noteChange(const RgbColor& color, uint32_t now) {
    color_ = color;
    changedAt_ = now;
    pending_ = true;
  }

  void cancel() {
    pending_ = false;
  }

  bool ready(uint32_t now, uint32_t delayMs) const {
    return pending_ && now - changedAt_ >= delayMs;
  }

  void markSaved() {
    pending_ = false;
  }

  bool pending() const {
    return pending_;
  }

  const RgbColor& color() const {
    return color_;
  }

 private:
  RgbColor color_ = {0, 0, 0};
  uint32_t changedAt_ = 0;
  bool pending_ = false;
};
