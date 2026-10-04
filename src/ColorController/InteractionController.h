#pragma once

#include <ST77922_Touch.h>

#include "ColorPersistenceService.h"
#include "ControllerModel.h"
#include "LightingOutput.h"
#include "PresetGesture.h"
#include "UiRenderer.h"

/**
 * Translates raw touch samples into model updates and coordinated output/UI
 * changes. poll() is called by the touch coroutine at a fixed cadence.
 */
class InteractionController {
 public:
  InteractionController(ST77922_TOUCH& touch, ControllerModel& model, LightingOutput& lighting,
                        UiRenderer& renderer, ColorPersistenceService& persistence);

  void poll(uint32_t now);

 private:
  enum class TouchTarget : uint8_t {
    kNone,
    kWheel,
    kPreset,
    kPower,
    kBrightness,
  };

  enum class ContactState : uint8_t {
    kIdle,
    kPressed,
    kReleaseDebouncing,
  };

  struct TouchState {
    TouchTarget target = TouchTarget::kNone;
    uint8_t presetIndex = 0;
    int16_t lastX = 0;
    int16_t lastY = 0;
  };

  TouchTarget identifyTarget(int16_t x, int16_t y, uint8_t& presetIndex);
  void selectWheelColor(int16_t x, int16_t y);
  void setBrightnessFromTouch(int16_t x);
  void handleTouchDown(int16_t x, int16_t y, uint32_t now);
  void handleTouchMove(int16_t x, int16_t y);
  void updatePresetHold(uint32_t now);
  void handleTouchUp();
  void handlePresetRelease();
  void recallPreset(uint8_t index);
  void activateMode(OutputMode mode);
  void handlePowerRelease();
  void handleTouchSample(uint32_t now);
  void handleNoTouch();

  ST77922_TOUCH& touch_;
  ControllerModel& model_;
  LightingOutput& lighting_;
  UiRenderer& renderer_;
  ColorPersistenceService& persistence_;
  TouchState touchState_;
  PresetGesture presetGesture_;
  ContactState contactState_ = ContactState::kIdle;
  uint8_t noTouchPolls_ = 0;
};
