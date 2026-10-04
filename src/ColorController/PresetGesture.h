#pragma once

#include <stdint.h>

/** Result emitted by the preset button gesture state machine. */
enum class PresetGestureEvent : uint8_t {
  kNone,
  kRecall,
  kStore,
};

/**
 * Distinguishes preset taps from long presses without blocking the main loop.
 *
 * A release inside the original preset emits kRecall unless update() has
 * already emitted kStore. Unsigned elapsed-time arithmetic keeps hold timing
 * correct across millis() rollover.
 */
class PresetGesture {
 public:
  /** Starts tracking a new preset press at the supplied millis() value. */
  void begin(uint32_t now) {
    startedAt_ = now;
    state_ = State::kPressed;
  }

  /** Emits kStore once when an in-bounds press reaches the hold threshold. */
  PresetGestureEvent update(uint32_t now, bool inside, uint32_t holdMs) {
    if (state_ == State::kPressed && inside && now - startedAt_ >= holdMs) {
      state_ = State::kStored;
      return PresetGestureEvent::kStore;
    }
    return PresetGestureEvent::kNone;
  }

  /** Finishes the gesture and emits kRecall only for an unconsumed in-bounds tap. */
  PresetGestureEvent release(bool inside) {
    const PresetGestureEvent event =
        state_ == State::kPressed && inside ? PresetGestureEvent::kRecall : PresetGestureEvent::kNone;
    reset();
    return event;
  }

  /** Cancels all state without emitting an event. */
  void reset() { state_ = State::kIdle; }

 private:
  enum class State : uint8_t {
    kIdle,
    kPressed,
    kStored,
  };

  uint32_t startedAt_ = 0;
  State state_ = State::kIdle;
};
