#ifdef NDEBUG
#error Color controller tests require assertions to be enabled
#endif

#include <cassert>
#include <cstdlib>
#include <iostream>

#include "../src/ColorController/ButtonControl.h"
#include "../src/ColorController/ColorMath.h"
#include "../src/ColorController/ControllerModel.h"
#include "../src/ColorController/InteractiveControls.h"
#include "../src/ColorController/PersistencePolicy.h"
#include "../src/ColorController/PresetGesture.h"
#include "../src/ColorController/ReactiveLighting.h"
#include "../src/ColorController/TouchDispatcher.h"
#include "../src/ColorController/UiGeometry.h"

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

void testWheelMarkerRepairBounds() {
  constexpr Ui::Circle wheel{120, 132, 105};
  constexpr Ui::Rect repair = Ui::expandedBounds(wheel, 8);

  static_assert(repair.x == 7);
  static_assert(repair.y == 19);
  static_assert(repair.width == 227);
  static_assert(repair.height == 227);

  assert(Ui::contains(repair, 120, 19));
  assert(Ui::contains(repair, 120, 245));
  assert(Ui::contains(repair, 7, 132));
  assert(Ui::contains(repair, 233, 132));
  assert(!Ui::contains(repair, 120, 18));
  assert(!Ui::contains(repair, 234, 132));
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

class TestButton : public ButtonControl {
 public:
  TestButton() : ButtonControl({10, 20, 30, 40}) {}

  uint8_t presses = 0;
  uint8_t moves = 0;
  uint8_t releases = 0;
  bool lastReleaseInside = false;

 protected:
  UiAction onPress(const TouchEvent&) override {
    ++presses;
    return {};
  }

  UiAction onMove(const TouchEvent&, bool) override {
    ++moves;
    return {};
  }

  UiAction onRelease(const TouchEvent&, bool releasedInside) override {
    ++releases;
    lastReleaseInside = releasedInside;
    return releasedInside ? UiAction::togglePower() : UiAction{};
  }
};

void testButtonAndTouchDispatch() {
  TestButton button;
  InteractiveControls controls;
  assert(controls.add(button));
  TouchDispatcher dispatcher(controls);

  assert(dispatcher.update(true, {15, 25}, 100).type == UiActionType::kNone);
  assert(button.presses == 1);

  assert(dispatcher.update(false, {}, 105).type == UiActionType::kNone);
  assert(dispatcher.update(true, {50, 25}, 110).type == UiActionType::kNone);
  assert(button.presses == 1);
  assert(button.moves == 1);

  for (uint8_t miss = 0; miss + 1 < Config::kReleaseDebouncePolls; ++miss) {
    assert(dispatcher.update(false, {}, 115 + miss).type == UiActionType::kNone);
  }
  const UiAction outsideRelease = dispatcher.update(false, {}, 120);
  assert(outsideRelease.type == UiActionType::kNone);
  assert(button.releases == 1);
  assert(!button.lastReleaseInside);

  dispatcher.update(true, {15, 25}, 200);
  UiAction release;
  for (uint8_t miss = 0; miss < Config::kReleaseDebouncePolls; ++miss) {
    release = dispatcher.update(false, {}, 205 + miss);
  }
  assert(release.type == UiActionType::kTogglePower);
  assert(button.releases == 2);
  assert(button.lastReleaseInside);

  InteractiveControls fullCollection;
  TestButton buttons[InteractiveControls::kCapacity];
  for (uint8_t index = 0; index < InteractiveControls::kCapacity; ++index) {
    assert(fullCollection.add(buttons[index]));
  }
  assert(!fullCollection.add(button));

  InteractiveControls duplicateCollection;
  assert(duplicateCollection.add(button));
  assert(!duplicateCollection.add(button));
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
  testWheelMarkerRepairBounds();
  testBrightnessMapping();
  testPresetGesture();
  testButtonAndTouchDispatch();
  testControllerModes();
  testReactiveLightingMath();
  testMusicEnvelope();
  testPersistencePolicy();
  std::cout << "Color math tests passed\n";
  return 0;
}
