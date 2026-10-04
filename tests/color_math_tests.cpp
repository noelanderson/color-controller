#ifdef NDEBUG
#error Color controller tests require assertions to be enabled
#endif

#include <cassert>
#include <cstdlib>
#include <iostream>

#include "../src/ColorController/ColorMath.h"
#include "../src/ColorController/ControllerModel.h"
#include "../src/ColorController/PersistencePolicy.h"
#include "../src/ColorController/PresetGesture.h"
#include "../src/ColorController/ReactiveLighting.h"

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

void testControllerModes() {
  ControllerModel model;
  assert(model.mode() == OutputMode::kSolid);
  assert(ControllerModel::kPresetCount == 4);

  model.setMode(OutputMode::kRainbow);
  assert(model.mode() == OutputMode::kRainbow);
  model.select({12, 34, 56});
  assert(model.mode() == OutputMode::kSolid);
  assert((model.selected() == RgbColor{12, 34, 56}));

  model.setMode(OutputMode::kMusic);
  model.togglePower();
  assert(model.mode() == OutputMode::kMusic);
  assert(!model.powerOn());
}

void testReactiveLightingMath() {
  assert(scaleBrightness(200, 0) == 0);
  assert(scaleBrightness(200, 255) == 200);
  assert(scaleBrightness(200, 128) == 100);

  assert(breathingIntensity(0, 5000) == 64);
  assert(breathingIntensity(2500, 5000) == 255);
  assert(breathingIntensity(5000, 5000) == 64);

  assert((rainbowColor(0, 12000, 0, 1) == RgbColor{255, 0, 0}));
  assert((rainbowColor(4000, 12000, 0, 1) == RgbColor{0, 255, 0}));
  assert((rainbowColor(0, 12000, 1, 3) == RgbColor{0, 255, 0}));
  assert(musicIntensity(0) == 32);
  assert(musicIntensity(255) == 255);
}

void testMusicEnvelope() {
  MusicEnvelope envelope;
  assert(envelope.update(100) == 0);
  assert(envelope.update(100) == 0);
  const uint8_t attack = envelope.update(1000);
  assert(attack > 150);
  const uint8_t release = envelope.update(100);
  assert(release < attack);
  assert(release > 0);
  uint8_t sustained = 0;
  for (uint16_t index = 0; index < 300; ++index) {
    sustained = envelope.update(1000);
  }
  assert(sustained > 200);
  envelope.reset();
  assert(envelope.level() == 0);
}

void testPersistencePolicy() {
  constexpr RgbColor color = {12, 34, 56};
  static_assert(packColor(color) == 0x0C2238);
  static_assert(unpackColor(0x0C2238) == color);

  ManualColorSaveTracker tracker;
  assert(!tracker.pending());
  tracker.noteChange(color, 1000);
  assert(tracker.pending());
  assert(!tracker.ready(120999, 120000));
  assert(tracker.ready(121000, 120000));

  const RgbColor changed = {70, 80, 90};
  tracker.noteChange(changed, 60000);
  assert(!tracker.ready(121000, 120000));
  assert(tracker.ready(180000, 120000));
  assert(tracker.color() == changed);

  tracker.cancel();
  assert(!tracker.ready(300000, 120000));

  tracker.noteChange(color, UINT32_MAX - 1000);
  assert(tracker.ready(119000, 120000));
  tracker.markSaved();
  assert(!tracker.pending());
}

}  // namespace

int main() {
  testPrimaryColors();
  testRgbRoundTrip();
  testWheelGeometry();
  testBrightnessMapping();
  testPresetGesture();
  testControllerModes();
  testReactiveLightingMath();
  testMusicEnvelope();
  testPersistencePolicy();
  std::cout << "Color math tests passed\n";
  return 0;
}
