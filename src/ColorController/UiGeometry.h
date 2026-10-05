#pragma once

#include <stdint.h>

namespace Ui {

/** Axis-aligned rectangle shared by control construction, drawing, and hit testing. */
struct Rect {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
};

/** Circular control geometry, expressed in framebuffer coordinates. */
struct Circle {
  int16_t centerX;
  int16_t centerY;
  int16_t radius;
};

/**
 * Color-wheel geometry.
 */
struct WheelLayout {
  Circle bounds;
};

/**
 * Complete brightness-slider geometry.
 *
 * redrawBounds covers every pixel changed by draw(); touchBounds may be larger
 * than the visible track to provide a finger-friendly target.
 */
struct SliderLayout {
  int16_t startX;
  int16_t endX;
  int16_t y;
  int16_t labelY;
  Rect redrawBounds;
  Rect touchBounds;
};

/** Returns true when the point lies inside the half-open rectangle. */
inline bool contains(const Rect& bounds, int16_t x, int16_t y) {
  return x >= bounds.x && x < bounds.x + bounds.width && y >= bounds.y && y < bounds.y + bounds.height;
}

/**
 * Returns the smallest half-open rectangle containing a circle plus a margin.
 *
 * The extra pixel converts the inclusive coordinates at both extrema into the
 * width and height expected by contains().
 */
constexpr Rect expandedBounds(const Circle& circle, int16_t margin) {
  return {
      static_cast<int16_t>(circle.centerX - circle.radius - margin),
      static_cast<int16_t>(circle.centerY - circle.radius - margin),
      static_cast<int16_t>(2 * (circle.radius + margin) + 1),
      static_cast<int16_t>(2 * (circle.radius + margin) + 1),
  };
}

}  // namespace Ui
