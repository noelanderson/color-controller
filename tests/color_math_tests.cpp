#ifdef NDEBUG
#error Color controller tests require assertions to be enabled
#endif

#include <cassert>
#include <cstdlib>
#include <iostream>

#include "../firmware/ColorController/ColorMath.h"
#include "../firmware/ColorController/PresetGesture.h"

namespace {

void expectNear(uint8_t actual, uint8_t expected, uint8_t tolerance = 2) {
  assert(std::abs(static_cast<int>(actual) - expected) <= tolerance);
}

void testPrimaryColors() {
  assert((hsvToRgb({0, 255, 255}) == RgbColor{255, 0, 0}));
  assert((hsvToRgb({120, 255, 255}) == RgbColor{0, 255, 0}));
  assert((hsvToRgb({240, 255, 255}) == RgbColor{0, 0, 255}));
  assert((hsvToRgb({300, 0, 127}) == RgbColor{127, 127, 127}));
}

void testRgbRoundTrip() {
  const RgbColor original = {37, 173, 221};
  const RgbColor roundTrip = hsvToRgb(rgbToHsv(original));
  expectNear(roundTrip.red, original.red);
  expectNear(roundTrip.green, original.green);
  expectNear(roundTrip.blue, original.blue);
}

void testWheelGeometry() {
  RgbColor color;
  assert(colorFromWheel(100, 100, 100, 100, 50, color));
  assert((color == RgbColor{255, 255, 255}));

  assert(colorFromWheel(150, 100, 100, 100, 50, color));
  assert((color == RgbColor{255, 0, 0}));

  assert(colorFromWheel(100, 150, 100, 100, 50, color));
  assert(color.green == 255);
  expectNear(color.red, 127, 2);
  assert(color.blue == 0);

  assert(!colorFromWheel(151, 100, 100, 100, 50, color));
}

void testBrightnessMapping() {
  assert(brightnessFromX(50, 100, 200) == 0);
  assert(brightnessFromX(100, 100, 200) == 0);
  assert(brightnessFromX(150, 100, 200) == 127);
  assert(brightnessFromX(200, 100, 200) == 255);
  assert(brightnessFromX(250, 100, 200) == 255);
}

void testPresetGesture() {
  PresetGesture gesture;

  gesture.begin(100);
  assert(gesture.update(799, true, 700) == PresetGestureEvent::kNone);
  assert(gesture.release(true) == PresetGestureEvent::kRecall);

  gesture.begin(100);
  assert(gesture.update(800, true, 700) == PresetGestureEvent::kStore);
  assert(gesture.release(true) == PresetGestureEvent::kNone);

  gesture.begin(100);
  assert(gesture.release(false) == PresetGestureEvent::kNone);

  gesture.begin(100);
  assert(gesture.update(800, false, 700) == PresetGestureEvent::kNone);
  assert(gesture.update(900, true, 700) == PresetGestureEvent::kStore);

  gesture.begin(UINT32_MAX - 100);
  assert(gesture.update(650, true, 700) == PresetGestureEvent::kStore);
}

}  // namespace

int main() {
  testPrimaryColors();
  testRgbRoundTrip();
  testWheelGeometry();
  testBrightnessMapping();
  testPresetGesture();
  std::cout << "Color math tests passed\n";
  return 0;
}
