#pragma once

#include <math.h>
#include <stdint.h>

/** An 8-bit-per-channel RGB color used by the UI and NeoPixel output. */
struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  constexpr bool operator==(const RgbColor& other) const {
    return red == other.red && green == other.green && blue == other.blue;
  }

  constexpr bool operator!=(const RgbColor& other) const {
    return !(*this == other);
  }
};

/** An HSV color with hue in degrees and saturation/value in the range 0-255. */
struct HsvColor {
  uint16_t hue;
  uint8_t saturation;
  uint8_t value;
};

constexpr float kPi = 3.14159265358979323846f;

/** Converts HSV to RGB using integer channel arithmetic. */
inline RgbColor hsvToRgb(const HsvColor& hsv) {
  if (hsv.saturation == 0) {
    return {hsv.value, hsv.value, hsv.value};
  }

  const uint16_t hue = hsv.hue % 360;
  const uint8_t region = static_cast<uint8_t>(hue / 60);
  const uint16_t remainder = (hue - region * 60) * 255 / 60;
  const uint8_t p = static_cast<uint8_t>(
      static_cast<uint16_t>(hsv.value) * (255 - hsv.saturation) / 255);
  const uint8_t q = static_cast<uint8_t>(
      static_cast<uint16_t>(hsv.value) *
      (255 - static_cast<uint16_t>(hsv.saturation) * remainder / 255) / 255);
  const uint8_t t = static_cast<uint8_t>(
      static_cast<uint16_t>(hsv.value) *
      (255 - static_cast<uint16_t>(hsv.saturation) * (255 - remainder) / 255) / 255);

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

  hsv.saturation = static_cast<uint16_t>(delta) * 255 / maximum;
  int16_t hue;
  if (maximum == rgb.red) {
    hue = 60 * static_cast<int16_t>(rgb.green - rgb.blue) / delta;
  } else if (maximum == rgb.green) {
    hue = 120 + 60 * static_cast<int16_t>(rgb.blue - rgb.red) / delta;
  } else {
    hue = 240 + 60 * static_cast<int16_t>(rgb.red - rgb.green) / delta;
  }
  if (hue < 0) {
    hue += 360;
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
inline bool colorFromWheel(int16_t x, int16_t y, int16_t centerX, int16_t centerY,
                           int16_t radius, RgbColor& color) {
  const int32_t deltaX = x - centerX;
  const int32_t deltaY = y - centerY;
  const int32_t distanceSquared = deltaX * deltaX + deltaY * deltaY;
  if (distanceSquared > static_cast<int32_t>(radius) * radius) {
    return false;
  }

  float angle = atan2f(static_cast<float>(deltaY), static_cast<float>(deltaX)) *
                180.0f / kPi;
  if (angle < 0.0f) {
    angle += 360.0f;
  }
  const float distance = sqrtf(static_cast<float>(distanceSquared));
  const uint8_t saturation =
      static_cast<uint8_t>(fminf(255.0f, distance * 255.0f / radius));
  color = hsvToRgb({static_cast<uint16_t>(angle), saturation, 255});
  return true;
}

/** Maps and clamps a horizontal slider coordinate to the range 0-255. */
inline uint8_t brightnessFromX(int16_t x, int16_t minimumX, int16_t maximumX) {
  if (x <= minimumX) {
    return 0;
  }
  if (x >= maximumX) {
    return 255;
  }
  return static_cast<uint8_t>(
      static_cast<uint32_t>(x - minimumX) * 255 / (maximumX - minimumX));
}
