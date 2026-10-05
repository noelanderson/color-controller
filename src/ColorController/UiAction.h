#pragma once

#include <stdint.h>

#include "ColorMath.h"
#include "ControllerModel.h"

enum class UiActionType : uint8_t {
  kNone,
  kSelectColor,
  kSetBrightness,
  kTogglePower,
  kRecallPreset,
  kStorePreset,
  kActivateMode,
  kPresetFeedbackExpired,
};

/**
 * Semantic user intent emitted by a control.
 *
 * Controls create actions but never mutate ControllerModel or perform device
 * I/O. UiActionProcessor is the single application boundary that interprets
 * these fields. Only the field associated with type is meaningful.
 */
struct UiAction {
  UiActionType type = UiActionType::kNone;
  RgbColor color = {};
  OutputMode mode = OutputMode::kSolid;
  uint8_t value = 0;

  /** Creates a solid-color selection action. */
  static UiAction selectColor(const RgbColor& color) {
    UiAction action;
    action.type = UiActionType::kSelectColor;
    action.color = color;
    return action;
  }

  /** Creates an absolute 0-255 brightness action. */
  static UiAction setBrightness(uint8_t brightness) {
    UiAction action;
    action.type = UiActionType::kSetBrightness;
    action.value = brightness;
    return action;
  }

  /** Creates a request to toggle output without discarding model state. */
  static UiAction togglePower() {
    UiAction action;
    action.type = UiActionType::kTogglePower;
    return action;
  }

  /** Creates a preset-indexed action such as recall, store, or feedback expiry. */
  static UiAction preset(UiActionType type, uint8_t index) {
    UiAction action;
    action.type = type;
    action.value = index;
    return action;
  }

  /** Creates a request to enter a non-solid output mode. */
  static UiAction activateMode(OutputMode mode) {
    UiAction action;
    action.type = UiActionType::kActivateMode;
    action.mode = mode;
    return action;
  }
};
