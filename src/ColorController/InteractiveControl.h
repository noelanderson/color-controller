#pragma once

#include <stdint.h>

#include "UiAction.h"

/** Framebuffer-space touch coordinate after rotation and clamping. */
struct TouchPoint {
  int16_t x = 0;
  int16_t y = 0;
};

/** One normalized sample with its rollover-safe millis() timestamp. */
struct TouchEvent {
  TouchPoint point;
  uint32_t now = 0;
};

/** Result of offering a new contact to one control. */
struct TouchClaim {
  bool claimed = false;
  UiAction action;
};

/**
 * Input-facing contract for one capturable UI component.
 *
 * Each control receives touch-down data and decides whether to claim it. The
 * dispatcher then guarantees that only the claimant receives move/up events.
 */
class InteractiveControl {
 public:
  virtual ~InteractiveControl() = default;

  /**
   * Offers the first sample of a new contact.
   *
   * @return claimed=true when this control owns the gesture until release.
   *         The optional action is processed immediately.
   */
  virtual TouchClaim tryTouchDown(const TouchEvent& event) = 0;
  /** Handles movement for a gesture already captured by this control. */
  virtual UiAction touchMove(const TouchEvent& event) = 0;
  /** Handles the debounced release and resets control-local gesture state. */
  virtual UiAction touchUp(const TouchEvent& event) = 0;
  /**
   * Advances time-based state independently of contact samples.
   *
   * Implementations must emit at most one action and avoid blocking.
   */
  virtual UiAction tick(uint32_t now) { return {}; }
};
