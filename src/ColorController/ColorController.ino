#include <Arduino.h>
#include <ST77922.h>
#include <ST77922_Touch.h>
#include <TFT_eSPI.h>

#include "AudioFeedback.h"
#include "AwaitConfig.h"
#include "BrightnessSliderControl.h"
#include "ColorPersistenceService.h"
#include "ColorPreviewControl.h"
#include "ColorWheelControl.h"
#include "Config.h"
#include "ControllerModel.h"
#include "InteractionController.h"
#include "LightingOutput.h"
#include "PowerButtonControl.h"
#include "UiLayout.h"
#include "UiRenderer.h"

// Hardware adapters and the PSRAM-backed software framebuffer.
TFT_eSPI frameBufferHost;
TFT_eSprite canvas(&frameBufferHost);
ST77922 display;
ST77922_TOUCH touch;
ControllerModel model;
LightingOutput lightingOutput(model);
ColorWheelControl colorWheel(canvas);
BrightnessSliderControl brightnessSlider(canvas);
PowerButtonControl powerButton(canvas);
ColorPreviewControl colorPreview(canvas);
UiRenderer uiRenderer(canvas, display, model, colorWheel, brightnessSlider, powerButton, colorPreview);
ColorPersistenceService persistenceService;
InteractionController interactionController(touch, model, lightingOutput, uiRenderer, persistenceService,
                                            colorWheel, brightnessSlider, powerButton);

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
      uiRenderer.drawControl(ControllerModel::kMusicControlIndex);
      displayChanged = true;
    }

    if (model.mode() != OutputMode::kSolid) {
      uiRenderer.drawColorStrip(lightingOutput.previewColor());
      uiRenderer.drawBrightnessControl(lightingOutput.previewColor());
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
  simpleawait::poll();
}
