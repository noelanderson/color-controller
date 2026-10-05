#pragma once

#include <ST77922_Touch.h>

#include "InteractiveControls.h"
#include "TouchDispatcher.h"
#include "UiActionProcessor.h"

/**
 * Reads the touch hardware and feeds normalized samples into the dispatcher.
 *
 * This is the sole owner allowed to sample ST77922_TOUCH. poll() is called by
 * the single touch coroutine at a fixed cadence, preventing competing readers
 * from observing inconsistent controller state.
 */
class InteractionController {
 public:
  /** Binds non-owning references; all collaborators must outlive the controller. */
  InteractionController(ST77922_TOUCH& touch, InteractiveControls& controls,
                        UiActionProcessor& actionProcessor);

  /** Reads one hardware sample, dispatches it, and advances timed controls. */
  void poll(uint32_t now);

 private:
  ST77922_TOUCH& touch_;
  UiActionProcessor& actionProcessor_;
  TouchDispatcher dispatcher_;
};
