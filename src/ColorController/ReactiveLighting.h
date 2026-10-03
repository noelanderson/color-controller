#pragma once

#include <math.h>
#include <stdint.h>

#include "ColorMath.h"

inline uint8_t scaleBrightness(uint8_t maximum, uint8_t intensity) {
  return static_cast<uint16_t>(maximum) * intensity / 255;
}

inline uint8_t breathingIntensity(uint32_t now, uint32_t periodMs) {
  if (periodMs == 0) {
    return 255;
  }
  const float phase = static_cast<float>(now % periodMs) / periodMs;
  const float wave = (1.0f - cosf(phase * 2.0f * kPi)) * 0.5f;
  return static_cast<uint8_t>(64.0f + wave * 191.0f);
}

inline RgbColor rainbowColor(uint32_t now, uint32_t cycleMs, uint16_t pixelIndex,
                             uint16_t pixelCount) {
  if (cycleMs == 0) {
    cycleMs = 1;
  }
  const uint16_t baseHue =
      static_cast<uint64_t>(now % cycleMs) * 360 / cycleMs;
  const uint16_t offset =
      pixelCount > 1 ? static_cast<uint32_t>(pixelIndex) * 360 / pixelCount : 0;
  return hsvToRgb({static_cast<uint16_t>((baseHue + offset) % 360), 255, 255});
}

inline RgbColor musicColor(uint32_t now, uint8_t level) {
  const uint16_t hue =
      static_cast<uint16_t>((now / 30 + static_cast<uint16_t>(level) * 2) % 360);
  return hsvToRgb({hue, 255, 255});
}

inline uint8_t musicIntensity(uint8_t level) {
  return static_cast<uint8_t>(32 + static_cast<uint16_t>(level) * 223 / 255);
}

class MusicEnvelope {
 public:
  uint8_t update(uint16_t magnitude) {
    if (!initialized_) {
      noiseFloor_ = magnitude;
      initialized_ = true;
      return 0;
    }

    if (magnitude < noiseFloor_) {
      noiseFloor_ = (noiseFloor_ * 31 + magnitude) / 32;
    } else if (magnitude < noiseFloor_ + noiseFloor_ / 2 + 64) {
      noiseFloor_ = (noiseFloor_ * 255 + magnitude) / 256;
    }

    const uint32_t signal = magnitude > noiseFloor_ ? magnitude - noiseFloor_ : 0;
    const uint32_t decayedPeak = peak_ * 250 / 256;
    peak_ = signal > decayedPeak ? signal : decayedPeak;
    if (peak_ < 64) {
      peak_ = 64;
    }

    uint32_t target = signal * 255 / peak_;
    if (target > 255) {
      target = 255;
    }
    if (target > level_) {
      level_ = static_cast<uint8_t>((level_ + target * 3) / 4);
    } else {
      level_ = static_cast<uint8_t>((static_cast<uint16_t>(level_) * 7 + target) / 8);
    }
    return level_;
  }

  void reset() {
    initialized_ = false;
    noiseFloor_ = 0;
    peak_ = 64;
    level_ = 0;
  }

  uint8_t level() const {
    return level_;
  }

 private:
  bool initialized_ = false;
  uint32_t noiseFloor_ = 0;
  uint32_t peak_ = 64;
  uint8_t level_ = 0;
};
