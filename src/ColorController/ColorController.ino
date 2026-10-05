/**
 * @file ColorController.ino
 * @brief Hardware composition root and fixed-memory coroutine wiring.
 *
 * setup() initializes every hardware service before spawning runtime tasks.
 * loop() delegates exclusively to SimpleAwait. Steady-state model mutation,
 * LED output, framebuffer transfers, persistence, and audio each have one
 * coroutine owner connected by a bounded action queue and coalesced mailboxes.
 */

#include <Arduino.h>
#include <ST77922.h>
#include <ST77922_Touch.h>
#include <TFT_eSPI.h>

#include "AudioFeedback.h"
#include "AwaitConfig.h"
#include "ColorPersistenceService.h"
#include "Config.h"
#include "ControllerMessages.h"
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
ControllerMessages controllerMessages;
simpleawait::Queue<UiAction, Config::kUiActionQueueCapacity> uiActionQueue;

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
UiActionProcessor uiActionProcessor(model, controllerMessages);
InteractionController interactionController(touch, interactiveControls);

/**
 * Registers the independently composed controls with input and drawing collections.
 *
 * Registration is transactional at startup: a false result prevents runtime
 * tasks from starting and leaves the diagnostic framebuffer visible.
 *
 * @return true when every interactive and drawable element fits its fixed collection.
 */
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

/**
 * Sole touch-hardware reader and producer for the semantic action FIFO.
 *
 * Up to two actions may result from one sample: a contact transition and a
 * time-driven control event. Queue send is intentionally awaited so a full
 * queue applies backpressure instead of silently dropping a user command.
 */
simpleawait::Task<void> monitorTouchInput() {
  while (true) {
    const uint32_t now = millis();
    UiAction actions[2];
    const uint8_t actionCount = interactionController.poll(now, actions);
    for (uint8_t index = 0; index < actionCount; ++index) {
      co_await uiActionQueue.send(actions[index]);
    }

    uint8_t error = 0;
    if (AwaitStatus::take(error)) {
      Serial.printf("WARNING: coroutine scheduler error %u\n", error);
    }
    co_await simpleawait::delay_ms(Config::kTouchPollMs);
  }
}

/**
 * Sole steady-state writer of ControllerModel.
 *
 * Startup restoration completes before this task is created. Each received
 * action is applied atomically within one scheduler resume, after which typed
 * side-effect messages are available to their owning service tasks.
 */
simpleawait::Task<void> processUiActions() {
  while (true) {
    const UiAction action = co_await uiActionQueue.receive();
    uiActionProcessor.process(action, millis());
  }
}

/**
 * Sole runtime owner of physical LED writes and reactive-lighting state.
 *
 * Solid-state changes are coalesced until this 25 ms task runs. Effect modes
 * render every frame. UI invalidations attached to a lighting request are
 * published only after previewColor() and the strips reflect the new model.
 */
simpleawait::Task<void> updateLighting() {
  while (true) {
    co_await simpleawait::delay_ms(Config::kEffectFrameMs);
    const LightingMessage message = controllerMessages.takeLighting();
    if (message.resetMusicEnvelope) {
      lightingOutput.resetMusicEnvelope();
    }
    if (message.apply || (model.powerOn() && model.mode() != OutputMode::kSolid)) {
      lightingOutput.apply();
    }
    if (message.uiAfterApply != kUiRefreshNone) {
      controllerMessages.requestUi(message.uiAfterApply);
    }
  }
}

/**
 * Sole runtime owner of UiRenderer and framebuffer transfers.
 *
 * Mailbox work is serviced at the responsive UI cadence, while effect preview
 * updates are rate-limited independently. Dirty bits coalesce multiple changes
 * into one framebuffer transfer. Confirmed preset-save notifications are
 * applied before drawing so SAVED and its expiry remain element-local state.
 */
simpleawait::Task<void> updateEffectUi() {
  AudioFeedback::MicrophoneStatus displayedMicrophoneStatus =
      AudioFeedback::MicrophoneStatus::kInitializing;
  uint32_t lastEffectRefreshAt = millis();
  while (true) {
    co_await simpleawait::delay_ms(Config::kUiServicePollMs);
    const uint32_t now = millis();
    UiMessage message = controllerMessages.takeUi();
    bool displayChanged = false;

    const AudioFeedback::MicrophoneStatus microphoneStatus = AudioFeedback::microphoneStatus();
    if (microphoneStatus != displayedMicrophoneStatus) {
      displayedMicrophoneStatus = microphoneStatus;
      message.refresh |= kUiRefreshMusic;
    }

    if (model.mode() != OutputMode::kSolid &&
        static_cast<uint32_t>(now - lastEffectRefreshAt) >= Config::kEffectUiFrameMs) {
      lastEffectRefreshAt = now;
      message.refresh |= kUiRefreshPreview | kUiRefreshBrightness;
    }

    for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
      const uint8_t bit = static_cast<uint8_t>(1U << index);
      if ((message.savedPresetMask & bit) != 0) {
        uiRenderer.notifyElement(
            UiElementIds::preset(index),
            {UiNotificationType::kPresetSaved, now + Config::kSavedFeedbackMs});
        message.redrawPresetMask |= bit;
      }
    }

    if ((message.refresh & kUiRefreshDynamic) != 0) {
      uiRenderer.drawDynamicUi(lightingOutput.previewColor());
      continue;
    }
    if ((message.refresh & kUiRefreshPreview) != 0) {
      uiRenderer.drawElement(UiElementIds::kPreview, lightingOutput.previewColor());
      displayChanged = true;
    }
    if ((message.refresh & kUiRefreshBrightness) != 0) {
      uiRenderer.drawElement(UiElementIds::kBrightness, lightingOutput.previewColor());
      displayChanged = true;
    }
    if ((message.refresh & kUiRefreshPower) != 0) {
      uiRenderer.drawElement(UiElementIds::kPower, lightingOutput.previewColor());
      displayChanged = true;
    }
    if ((message.refresh & kUiRefreshMusic) != 0) {
      uiRenderer.drawElement(UiElementIds::kMusic, lightingOutput.previewColor());
      displayChanged = true;
    }
    for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
      if ((message.redrawPresetMask & static_cast<uint8_t>(1U << index)) != 0) {
        uiRenderer.drawElement(UiElementIds::preset(index), lightingOutput.previewColor());
        displayChanged = true;
      }
    }
    if (displayChanged) {
      uiRenderer.flushDisplay();
    }
  }
}

/**
 * Sole runtime owner of persistence policy and NVS writes.
 *
 * The task drains latest-value mailbox state, advances delayed/manual and retry
 * policies, and publishes SAVED only for preset bits reported durable by NVS.
 */
simpleawait::Task<void> persistStableManualColor() {
  while (true) {
    co_await simpleawait::delay_ms(Config::kPersistencePollMs);
    const PersistenceMessage message = controllerMessages.takePersistence();
    if (message.manualColor == ManualColorMessage::kChanged) {
      persistenceService.noteManualColor(message.color, message.now);
    } else if (message.manualColor == ManualColorMessage::kCancelled) {
      persistenceService.cancelManualColor();
    }
    for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
      if ((message.presetMask & static_cast<uint8_t>(1U << index)) != 0) {
        persistenceService.queuePresetSave(
            index, message.presetColors[index], message.presetTimes[index]);
      }
    }
    const uint8_t savedPresetMask = persistenceService.process(millis(), model);
    for (uint8_t index = 0; index < ControllerModel::kPresetCount; ++index) {
      if ((savedPresetMask & static_cast<uint8_t>(1U << index)) != 0) {
        controllerMessages.notifyPresetSaved(index);
      }
    }
  }
}

simpleawait::Task<void> reportFramebufferFailure() {
  // Fatal reporters keep loop() scheduler-only and suspend positively between
  // messages, preserving idle time even when normal runtime cannot start.
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
  // Task creation can fail because the fixed slot table or frame pool is
  // exhausted. Report it synchronously while Serial is known to be available.
  const simpleawait::TaskHandle handle =
      simpleawait::create_task(static_cast<simpleawait::Task<void>&&>(task));
  if (handle.valid()) {
    return true;
  }
  Serial.printf("FATAL: unable to start %s coroutine\n", name);
  return false;
}

void setup() {
  // Initialization order is a dependency graph:
  // display -> restored model -> framebuffer/UI -> first output/frame ->
  // touch/shared I2C -> runtime tasks -> audio device on the shared bus.
  // No global constructor accesses hardware.
  Serial.begin(Config::kSerialBaud);

  if (!display.begin()) {
    startControllerTask(reportDisplayFailure(), "display failure reporter");
    return;
  }
  display.Set_Rotation(Config::kDisplayRotation);

  // Storage is optional at boot. Defaults remain valid and later persistence
  // attempts surface failures without preventing the controller from running.
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

  // Render and illuminate a complete initial state before exposing touch input
  // or turning on the panel backlight, avoiding a partially drawn startup UI.
  lightingOutput.begin();
  lightingOutput.apply();
  uiRenderer.drawInitialUi(lightingOutput.previewColor());
  display.Set_Backlight(true);

  if (!touch.init()) {
    Serial.println("WARNING: touch initialization did not reach an idle state");
  }
  touch.Set_Rotation(Config::kDisplayRotation);

  // All state and hardware referenced by these tasks are now ready. Each call
  // reports its own fixed-resource failure; successfully created tasks remain
  // independently schedulable if another optional service cannot start.
  startControllerTask(monitorTouchInput(), "touch input");
  startControllerTask(processUiActions(), "UI actions");
  startControllerTask(updateLighting(), "LED effects");
  startControllerTask(updateEffectUi(), "effect UI");
  startControllerTask(persistStableManualColor(), "persistence");
  if (!AudioFeedback::start(g_touchI2CBus, controllerMessages)) {
    Serial.println("FATAL: unable to start audio coroutine");
  }
}

void loop() {
  // poll_and_wait() runs ready coroutines, then blocks the Arduino loop task
  // until a timer or cross-coroutine event is due. Application work must not be
  // added here because a permanently-ready loop would defeat scheduler idle.
  simpleawait::poll_and_wait();
}
