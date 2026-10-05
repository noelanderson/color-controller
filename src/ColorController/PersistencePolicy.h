#pragma once

#include <stdint.h>

#include "ColorMath.h"

/** Packs RGB channels as stable 0x00RRGGBB persistence data. */
constexpr uint32_t packColor(const RgbColor& color) {
  // Preferences stores a stable 0x00RRGGBB value independent of struct layout.
  return (static_cast<uint32_t>(color.red) << 16) | (static_cast<uint32_t>(color.green) << 8) | color.blue;
}

/** Restores RGB channels from the low 24 bits of packed persistence data. */
constexpr RgbColor unpackColor(uint32_t packed) {
  return {
      static_cast<uint8_t>(packed >> 16),
      static_cast<uint8_t>(packed >> 8),
      static_cast<uint8_t>(packed),
  };
}

/** Tracks one manually selected color until it remains stable for the save delay. */
class ManualColorSaveTracker {
 public:
  /** Starts or restarts the stability interval for @p color. */
  void noteChange(const RgbColor& color, uint32_t now) {
    color_ = color;
    changedAt_ = now;
    pending_ = true;
  }

  /** Clears pending state without altering the remembered candidate color. */
  void cancel() { pending_ = false; }

  /** @return true once the pending candidate has remained unchanged for @p delayMs. */
  bool ready(uint32_t now, uint32_t delayMs) const { return pending_ && now - changedAt_ >= delayMs; }

  /** Marks the current candidate persisted without changing its value. */
  void markSaved() { pending_ = false; }

  /** @return Whether a candidate is awaiting its stability interval. */
  bool pending() const { return pending_; }

  /** @return The most recently noted candidate color. */
  const RgbColor& color() const { return color_; }

 private:
  RgbColor color_ = {0, 0, 0};
  uint32_t changedAt_ = 0;
  bool pending_ = false;
};
