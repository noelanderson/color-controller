#pragma once

#include <math.h>
#include <stdint.h>

/** An 8-bit-per-channel RGB color shared by the UI and addressable LED output. */
struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  constexpr bool operator==(const RgbColor& other) const {
    return red == other.red && green == other.green && blue == other.blue;
  }

  constexpr bool operator!=(const RgbColor& other) const { return !(*this == other); }
};

/** An HSV color with hue in degrees and saturation/value in the range 0-255. */
struct HsvColor {
  uint16_t hue;
  uint8_t saturation;
  uint8_t value;
};

constexpr float kPi = 3.14159265358979323846f;
constexpr uint16_t kHueCircleDegrees = 360;
constexpr uint8_t kHueRegionDegrees = 60;
constexpr uint8_t kColorChannelMax = UINT8_MAX;

/** Converts HSV to RGB using integer channel arithmetic. */
inline RgbColor hsvToRgb(const HsvColor& hsv) {
  if (hsv.saturation == 0) {
    return {hsv.value, hsv.value, hsv.value};
  }

  const uint16_t hue = hsv.hue % kHueCircleDegrees;
  const uint8_t region = static_cast<uint8_t>(hue / kHueRegionDegrees);
  const uint16_t remainder = (hue - region * kHueRegionDegrees) * kColorChannelMax / kHueRegionDegrees;
  const uint8_t p = static_cast<uint8_t>(static_cast<uint16_t>(hsv.value) *
                                         (kColorChannelMax - hsv.saturation) / kColorChannelMax);
  const uint8_t q = static_cast<uint8_t>(
      static_cast<uint16_t>(hsv.value) *
      (kColorChannelMax - static_cast<uint16_t>(hsv.saturation) * remainder / kColorChannelMax) /
      kColorChannelMax);
  const uint8_t t =
      static_cast<uint8_t>(static_cast<uint16_t>(hsv.value) *
                           (kColorChannelMax - static_cast<uint16_t>(hsv.saturation) *
                                                   (kColorChannelMax - remainder) / kColorChannelMax) /
                           kColorChannelMax);

  switch (region) {
    case 0:
      return {hsv.value, t, p};
    case 1:
      return {q, hsv.value, p};
    case 2:
      return {p, hsv.value, t};
    case 3:
      return {p, q, hsv.value};
    case 4:
      return {t, p, hsv.value};
    default:
      return {hsv.value, p, q};
  }
}

/** Converts RGB to HSV for positioning the marker on the color wheel. */
inline HsvColor rgbToHsv(const RgbColor& rgb) {
  const uint8_t redGreenMaximum = rgb.red > rgb.green ? rgb.red : rgb.green;
  const uint8_t redGreenMinimum = rgb.red < rgb.green ? rgb.red : rgb.green;
  const uint8_t maximum = redGreenMaximum > rgb.blue ? redGreenMaximum : rgb.blue;
  const uint8_t minimum = redGreenMinimum < rgb.blue ? redGreenMinimum : rgb.blue;
  const uint8_t delta = maximum - minimum;

  HsvColor hsv = {0, 0, maximum};
  if (maximum == 0 || delta == 0) {
    return hsv;
  }

  hsv.saturation = static_cast<uint16_t>(delta) * kColorChannelMax / maximum;
  int16_t hue;
  if (maximum == rgb.red) {
    hue = kHueRegionDegrees * static_cast<int16_t>(rgb.green - rgb.blue) / delta;
  } else if (maximum == rgb.green) {
    hue = 2 * kHueRegionDegrees + kHueRegionDegrees * static_cast<int16_t>(rgb.blue - rgb.red) / delta;
  } else {
    hue = 4 * kHueRegionDegrees + kHueRegionDegrees * static_cast<int16_t>(rgb.red - rgb.green) / delta;
  }
  if (hue < 0) {
    hue += kHueCircleDegrees;
  }
  hsv.hue = hue;
  return hsv;
}

/**
 * Converts a screen coordinate inside a circular hue/saturation wheel to RGB.
 *
 * Hue is determined by angle, saturation by distance from the center, and
 * value is fixed at full brightness. Returns false when the point is outside
 * the wheel and leaves color unchanged.
 */
inline bool colorFromWheel(int16_t x, int16_t y, int16_t centerX, int16_t centerY, int16_t radius,
                           RgbColor& color) {
  const int32_t deltaX = x - centerX;
  const int32_t deltaY = y - centerY;
  const int32_t distanceSquared = deltaX * deltaX + deltaY * deltaY;
  if (distanceSquared > static_cast<int32_t>(radius) * radius) {
    return false;
  }

  float angle =
      atan2f(static_cast<float>(deltaY), static_cast<float>(deltaX)) * (kHueCircleDegrees / 2.0f) / kPi;
  if (angle < 0.0f) {
    angle += kHueCircleDegrees;
  }
  const float distance = sqrtf(static_cast<float>(distanceSquared));
  const uint8_t saturation =
      static_cast<uint8_t>(fminf(kColorChannelMax, distance * kColorChannelMax / radius));
  color = hsvToRgb({static_cast<uint16_t>(angle), saturation, kColorChannelMax});
  return true;
}

/** Maps and clamps a horizontal slider coordinate to the range 0-255. */
inline uint8_t brightnessFromX(int16_t x, int16_t minimumX, int16_t maximumX) {
  if (x <= minimumX) {
    return 0;
  }
  if (x >= maximumX) {
    return kColorChannelMax;
  }
  return static_cast<uint8_t>(static_cast<uint32_t>(x - minimumX) * kColorChannelMax / (maximumX - minimumX));
}
