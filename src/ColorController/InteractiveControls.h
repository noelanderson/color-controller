#pragma once

#include "InteractiveControl.h"

/**
 * Fixed-memory registry for independently owned interactive elements.
 *
 * Registration order controls which element first sees a touch-down event, but
 * layouts must keep touch regions disjoint rather than relying on that order.
 * Registered objects must outlive the collection and any TouchDispatcher that
 * references it.
 */
class InteractiveControls {
 public:
  static constexpr uint8_t kCapacity = 9;

  /** Removes all registrations without destroying sketch-owned controls. */
  void clear() {
    count_ = 0;
    for (InteractiveControl*& control : controls_) {
      control = nullptr;
    }
  }

  /**
   * Registers one non-owning control pointer.
   *
   * @return false on capacity overflow or duplicate pointer registration.
   */
  bool add(InteractiveControl& control) {
    if (count_ >= kCapacity) {
      return false;
    }
    for (uint8_t index = 0; index < count_; ++index) {
      if (controls_[index] == &control) {
        return false;
      }
    }
    controls_[count_++] = &control;
    return true;
  }

  /** @return Number of currently registered controls. */
  uint8_t count() const { return count_; }

  /**
   * Returns a registered element by index.
   *
   * Callers must pass an index below count(). Bounds checks are intentionally
   * kept at the collection-iteration boundary to avoid per-poll duplication.
   */
  InteractiveControl& at(uint8_t index) { return *controls_[index]; }

  /**
   * Advances timed state on every registered element.
   *
   * At most one action is returned per scheduler poll. Later simultaneous
   * actions are discarded, so timed controls must be designed such that their
   * actionable expirations cannot coincide.
   */
  UiAction tick(uint32_t now) {
    for (uint8_t index = 0; index < count_; ++index) {
      const UiAction action = controls_[index]->tick(now);
      if (action.type != UiActionType::kNone) {
        return action;
      }
    }
    return {};
  }

 private:
  InteractiveControl* controls_[kCapacity] = {};
  uint8_t count_ = 0;
};
