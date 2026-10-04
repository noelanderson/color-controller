#pragma once

#include <math.h>
#include <stdint.h>

#include "ColorMath.h"

namespace ReactiveLighting {

constexpr uint8_t kBreathingMinimum = 64;
constexpr uint8_t kBreathingRange = UINT8_MAX - kBreathingMinimum;
constexpr uint32_t kMusicHueStepMs = 30;
constexpr uint8_t kMusicHueDegreesPerLevel = 2;
constexpr uint8_t kMusicMinimumIntensity = 32;
constexpr uint8_t kMusicIntensityRange = UINT8_MAX - kMusicMinimumIntensity;

}  // namespace ReactiveLighting

inline uint8_t scaleBrightness(uint8_t maximum, uint8_t intensity) {
  return static_cast<uint16_t>(maximum) * intensity / UINT8_MAX;
}

inline uint8_t breathingIntensity(uint32_t now, uint32_t periodMs) {
  if (periodMs == 0) {
    return UINT8_MAX;
  }
  const float phase = static_cast<float>(now % periodMs) / periodMs;
  const float wave = (1.0f - cosf(phase * 2.0f * kPi)) * 0.5f;
  return static_cast<uint8_t>(ReactiveLighting::kBreathingMinimum + wave * ReactiveLighting::kBreathingRange);
}

inline RgbColor rainbowColor(uint32_t now, uint32_t cycleMs, uint16_t pixelIndex, uint16_t pixelCount) {
  if (cycleMs == 0) {
    cycleMs = 1;
  }
  const uint16_t baseHue = static_cast<uint64_t>(now % cycleMs) * kHueCircleDegrees / cycleMs;
  const uint16_t offset =
      pixelCount > 1 ? static_cast<uint32_t>(pixelIndex) * kHueCircleDegrees / pixelCount : 0;
  return hsvToRgb({
      static_cast<uint16_t>((baseHue + offset) % kHueCircleDegrees),
      UINT8_MAX,
      UINT8_MAX,
  });
}

inline RgbColor musicColor(uint32_t now, uint8_t level) {
  const uint16_t hue =
      static_cast<uint16_t>((now / ReactiveLighting::kMusicHueStepMs +
                             static_cast<uint16_t>(level) * ReactiveLighting::kMusicHueDegreesPerLevel) %
                            kHueCircleDegrees);
  return hsvToRgb({hue, UINT8_MAX, UINT8_MAX});
}

inline uint8_t musicIntensity(uint8_t level) {
  return static_cast<uint8_t>(ReactiveLighting::kMusicMinimumIntensity +
                              static_cast<uint16_t>(level) * ReactiveLighting::kMusicIntensityRange /
                                  UINT8_MAX);
}

/**
 * Tracks an adaptive noise floor and recent signal peak, then applies a fast
 * attack and slower release so music pulses feel responsive without flicker.
 */
class MusicEnvelope {
 public:
  uint8_t update(uint16_t magnitude) {
    if (!initialized_) {
      noiseFloor_ = magnitude;
      initialized_ = true;
      return 0;
    }

    if (magnitude < noiseFloor_) {
      noiseFloor_ = (noiseFloor_ * kFastFloorHistoryWeight + magnitude) / kFastFloorDivisor;
    } else if (magnitude < noiseFloor_ + noiseFloor_ / 2 + kMinimumPeak) {
      noiseFloor_ = (noiseFloor_ * kSlowFloorHistoryWeight + magnitude) / kSlowFloorDivisor;
    }

    const uint32_t signal = magnitude > noiseFloor_ ? magnitude - noiseFloor_ : 0;
    const uint32_t decayedPeak = peak_ * kPeakDecayNumerator / kPeakDecayDivisor;
    peak_ = signal > decayedPeak ? signal : decayedPeak;
    if (peak_ < kMinimumPeak) {
      peak_ = kMinimumPeak;
    }

    uint32_t target = signal * UINT8_MAX / peak_;
    if (target > UINT8_MAX) {
      target = UINT8_MAX;
    }
    if (target > level_) {
      level_ = static_cast<uint8_t>((level_ + target * kAttackTargetWeight) / kAttackDivisor);
    } else {
      level_ = static_cast<uint8_t>((static_cast<uint16_t>(level_) * kReleaseHistoryWeight + target) /
                                    kReleaseDivisor);
    }
    return level_;
  }

  void reset() {
    initialized_ = false;
    noiseFloor_ = 0;
    peak_ = kMinimumPeak;
    level_ = 0;
  }

  uint8_t level() const { return level_; }

 private:
  static constexpr uint8_t kFastFloorHistoryWeight = 31;
  static constexpr uint8_t kFastFloorDivisor = 32;
  static constexpr uint16_t kSlowFloorHistoryWeight = 255;
  static constexpr uint16_t kSlowFloorDivisor = 256;
  static constexpr uint8_t kMinimumPeak = 64;
  static constexpr uint16_t kPeakDecayNumerator = 250;
  static constexpr uint16_t kPeakDecayDivisor = 256;
  static constexpr uint8_t kAttackTargetWeight = 3;
  static constexpr uint8_t kAttackDivisor = 4;
  static constexpr uint8_t kReleaseHistoryWeight = 7;
  static constexpr uint8_t kReleaseDivisor = 8;

  bool initialized_ = false;
  uint32_t noiseFloor_ = 0;
  uint32_t peak_ = kMinimumPeak;
  uint8_t level_ = 0;
};
