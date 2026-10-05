#include <Arduino.h>
#include <ST77922.h>
#include <ST77922_Touch.h>
#include <TFT_eSPI.h>

#include "AudioFeedback.h"
#include "AwaitConfig.h"
#include "ColorPersistenceService.h"
#include "Config.h"
#include "ControllerModel.h"
#include "BrightnessSliderControl.h"
#include "ColorPreviewControl.h"
#include "ColorWheelControl.h"
#include "InteractionController.h"
#include "InteractiveControls.h"
#include "LightingOutput.h"
#include "ModeButtonControl.h"
#include "PowerButtonControl.h"
#include "PresetButtonControl.h"
#include "UiActionProcessor.h"
#include "UiElementIds.h"
#include "UiLayout.h"
#include "UiRenderer.h"
#include "UiScene.h"

// Hardware adapters and the PSRAM-backed software framebuffer.
TFT_eSPI frameBufferHost;
TFT_eSprite canvas(&frameBufferHost);
ST77922 display;
ST77922_TOUCH touch;
ControllerModel model;
LightingOutput lightingOutput(model);

// UI composition. Geometry lives in UiLayout.h; this block makes the visible
// controls, their types, and their screen arrangement easy to discover.
//
//   [ color wheel ]  [ P1 ][ P2 ]
//                    [ P3 ][ P4 ]
//                    [ rainbow ][ music ]
//   [ power ]        [ brightness slider ]
constexpr Ui::WheelLayout kColorWheelLayout{
    .bounds = {120, 132, 105},
};
constexpr Ui::Rect kPreset1Bounds{260, 37, 96, 56};
constexpr Ui::Rect kPreset2Bounds{370, 37, 96, 56};
constexpr Ui::Rect kPreset3Bounds{260, 104, 96, 56};
constexpr Ui::Rect kPreset4Bounds{370, 104, 96, 56};
constexpr Ui::Rect kRainbowBounds{260, 171, 96, 56};
constexpr Ui::Rect kMusicBounds{370, 171, 96, 56};
constexpr Ui::Rect kPowerBounds{12, 262, 88, 46};
constexpr Ui::SliderLayout kBrightnessLayout{
    .startX = 128,
    .endX = 462,
    .y = 286,
    .labelY = 255,
    .redrawBounds = {116, 252, 359, 62},
    .touchBounds = {116, 264, 358, 44},
};
constexpr Ui::Rect kPreviewBounds{0, 0, Ui::kWidth, 14};
ColorWheelControl colorWheel(canvas, UiElementIds::kWheel, kColorWheelLayout);
PresetButtonControl preset1Button(canvas, UiElementIds::kPreset1, kPreset1Bounds, 0);
PresetButtonControl preset2Button(canvas, UiElementIds::kPreset2, kPreset2Bounds, 1);
PresetButtonControl preset3Button(canvas, UiElementIds::kPreset3, kPreset3Bounds, 2);
PresetButtonControl preset4Button(canvas, UiElementIds::kPreset4, kPreset4Bounds, 3);
ModeButtonControl rainbowButton(canvas, UiElementIds::kRainbow, kRainbowBounds, OutputMode::kRainbow);
ModeButtonControl musicButton(canvas, UiElementIds::kMusic, kMusicBounds, OutputMode::kMusic);
PowerButtonControl powerButton(canvas, UiElementIds::kPower, kPowerBounds);
BrightnessSliderControl brightnessSlider(canvas, UiElementIds::kBrightness, kBrightnessLayout);
ColorPreviewControl colorPreview(canvas, UiElementIds::kPreview, kPreviewBounds);

InteractiveControls interactiveControls;
UiScene uiScene;
UiRenderer uiRenderer(canvas, display, model, uiScene);
ColorPersistenceService persistenceService;
UiActionProcessor uiActionProcessor(model, lightingOutput, uiRenderer, persistenceService);
InteractionController interactionController(touch, interactiveControls, uiActionProcessor);

bool configureUserInterface() {
  interactiveControls.clear();
  uiScene.clear();

  // Controls must not overlap; keep this list aligned with the visual
  // composition above so additions remain easy to audit.
  const bool inputReady =
      interactiveControls.add(colorWheel) &&
      interactiveControls.add(preset1Button) &&
      interactiveControls.add(preset2Button) &&
      interactiveControls.add(preset3Button) &&
      interactiveControls.add(preset4Button) &&
      interactiveControls.add(rainbowButton) &&
      interactiveControls.add(musicButton) &&
      interactiveControls.add(powerButton) &&
      interactiveControls.add(brightnessSlider);

  const bool sceneReady =
      uiScene.add(colorPreview) &&
      uiScene.add(colorWheel) &&
      uiScene.add(preset1Button) &&
      uiScene.add(preset2Button) &&
      uiScene.add(preset3Button) &&
      uiScene.add(preset4Button) &&
      uiScene.add(rainbowButton) &&
      uiScene.add(musicButton) &&
      uiScene.add(powerButton) &&
      uiScene.add(brightnessSlider);

  return inputReady && sceneReady;
}

simpleawait::Task<void> monitorTouchInput() {
  while (true) {
    const uint32_t now = millis();
    interactionController.poll(now);

    uint8_t error = 0;
    if (AwaitStatus::take(error)) {
      Serial.printf("WARNING: coroutine scheduler error %u\n", error);
    }
    co_await simpleawait::delay_ms(Config::kTouchPollMs);
  }
}

simpleawait::Task<void> updateEffects() {
  while (true) {
    co_await simpleawait::delay_ms(Config::kEffectFrameMs);
    if (model.powerOn() && model.mode() != OutputMode::kSolid) {
      lightingOutput.apply();
    }
  }
}

simpleawait::Task<void> updateEffectUi() {
  AudioFeedback::MicrophoneStatus displayedMicrophoneStatus =
      AudioFeedback::MicrophoneStatus::kInitializing;
  while (true) {
    co_await simpleawait::delay_ms(Config::kEffectUiFrameMs);
    bool displayChanged = false;

    const AudioFeedback::MicrophoneStatus microphoneStatus = AudioFeedback::microphoneStatus();
    if (microphoneStatus != displayedMicrophoneStatus) {
      displayedMicrophoneStatus = microphoneStatus;
      uiRenderer.drawElement(UiElementIds::kMusic, lightingOutput.previewColor());
      displayChanged = true;
    }

    if (model.mode() != OutputMode::kSolid) {
      uiRenderer.drawElement(UiElementIds::kPreview, lightingOutput.previewColor());
      uiRenderer.drawElement(UiElementIds::kBrightness, lightingOutput.previewColor());
      displayChanged = true;
    }
    if (displayChanged) {
      uiRenderer.flushDisplay();
    }
  }
}

simpleawait::Task<void> persistStableManualColor() {
  while (true) {
    co_await simpleawait::delay_ms(Config::kPersistencePollMs);
    persistenceService.process(millis(), model);
  }
}

simpleawait::Task<void> reportFramebufferFailure() {
  while (true) {
    Serial.println("FATAL: unable to allocate the display framebuffer");
    co_await simpleawait::delay_ms(Config::kFatalReportMs);
  }
}

simpleawait::Task<void> reportDisplayFailure() {
  while (true) {
    Serial.println("FATAL: display initialization failed");
    co_await simpleawait::delay_ms(Config::kFatalReportMs);
  }
}

simpleawait::Task<void> reportUiConfigurationFailure() {
  while (true) {
    Serial.println("FATAL: UI input or scene registration failed");
    co_await simpleawait::delay_ms(Config::kFatalReportMs);
  }
}

bool startControllerTask(simpleawait::Task<void>&& task, const char* name) {
  const simpleawait::TaskHandle handle =
      simpleawait::create_task(static_cast<simpleawait::Task<void>&&>(task));
  if (handle.valid()) {
    return true;
  }
  Serial.printf("FATAL: unable to start %s coroutine\n", name);
  return false;
}

void setup() {
  Serial.begin(Config::kSerialBaud);

  if (!display.begin()) {
    startControllerTask(reportDisplayFailure(), "display failure reporter");
    return;
  }
  display.Set_Rotation(Config::kDisplayRotation);

  if (!persistenceService.begin(model)) {
    Serial.println("WARNING: persistent color storage unavailable");
  }

  canvas.setColorDepth(Ui::kFramebufferColorDepth);
  if (canvas.createSprite(Ui::kWidth, Ui::kHeight) == nullptr) {
    startControllerTask(reportFramebufferFailure(), "framebuffer failure reporter");
    return;
  }
  canvas.setSwapBytes(true);

  if (!configureUserInterface()) {
    startControllerTask(reportUiConfigurationFailure(), "UI configuration failure reporter");
    return;
  }

  lightingOutput.begin();
  lightingOutput.apply();
  uiRenderer.drawInitialUi(lightingOutput.previewColor());
  display.Set_Backlight(true);

  if (!touch.init()) {
    Serial.println("WARNING: touch initialization did not reach an idle state");
  }
  touch.Set_Rotation(Config::kDisplayRotation);

  startControllerTask(monitorTouchInput(), "touch input");
  startControllerTask(updateEffects(), "LED effects");
  startControllerTask(updateEffectUi(), "effect UI");
  startControllerTask(persistStableManualColor(), "persistence");
  if (!AudioFeedback::start(g_touchI2CBus)) {
    Serial.println("FATAL: unable to start audio coroutine");
  }
}

void loop() {
  simpleawait::poll_and_wait();
}
