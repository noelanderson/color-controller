#pragma once

#include "Config.h"
#include "InteractiveControls.h"

/**
 * Normalizes noisy touch contact into one captured gesture.
 *
 * On touch-down, registered elements are offered the event until one claims
 * it. That claimant alone receives every move and the debounced release, even
 * if the finger leaves its bounds.
 */
class TouchDispatcher {
 public:
  /** Binds a non-owning control collection that must outlive the dispatcher. */
  explicit TouchDispatcher(InteractiveControls& controls) : controls_(controls) {}

  /**
   * Advances time-based control state when no hardware sample is required.
   *
   * @return The first semantic action emitted by collection iteration.
   */
  UiAction tick(uint32_t now) { return controls_.tick(now); }

  /**
   * Consumes one normalized touch sample.
   *
   * Missing samples enter release debounce; contact returning before the
   * threshold continues the existing capture. The final release uses the last
   * valid coordinate because the hardware exposes no coordinate after contact.
   *
   * @param touching Whether the hardware currently reports contact.
   * @param point Current coordinate when touching; ignored otherwise.
   * @param now Current millis() timestamp.
   * @return At most one semantic action produced by the captured control.
   */
  UiAction update(bool touching, const TouchPoint& point, uint32_t now) {
    if (touching) {
      lastPoint_ = point;
      noTouchPolls_ = 0;

      if (state_ == ContactState::kIdle) {
        state_ = ContactState::kPressed;
        const TouchEvent event{point, now};
        for (uint8_t index = 0; index < controls_.count(); ++index) {
          InteractiveControl& candidate = controls_.at(index);
          const TouchClaim claim = candidate.tryTouchDown(event);
          if (claim.claimed) {
            captured_ = &candidate;
            return claim.action;
          }
        }
        return {};
      }

      // A sample returning during release debounce continues the captured gesture.
      state_ = ContactState::kPressed;
      return captured_ == nullptr ? UiAction{} : captured_->touchMove({point, now});
    }

    if (state_ == ContactState::kIdle) {
      return {};
    }

    if (state_ == ContactState::kPressed) {
      state_ = ContactState::kReleaseDebouncing;
      noTouchPolls_ = 1;
    } else {
      ++noTouchPolls_;
    }

    if (noTouchPolls_ < Config::kReleaseDebouncePolls) {
      return {};
    }

    UiAction action;
    if (captured_ != nullptr) {
      action = captured_->touchUp({lastPoint_, now});
    }
    captured_ = nullptr;
    state_ = ContactState::kIdle;
    noTouchPolls_ = 0;
    return action;
  }

 private:
  enum class ContactState : uint8_t {
    kIdle,
    kPressed,
    kReleaseDebouncing,
  };

  InteractiveControls& controls_;
  InteractiveControl* captured_ = nullptr;
  TouchPoint lastPoint_;
  ContactState state_ = ContactState::kIdle;
  uint8_t noTouchPolls_ = 0;
};
