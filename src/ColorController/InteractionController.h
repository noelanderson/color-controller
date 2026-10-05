#pragma once

#include <ST77922_Touch.h>

#include "InteractiveControls.h"
#include "TouchDispatcher.h"
#include "UiAction.h"

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
  InteractionController(ST77922_TOUCH& touch, InteractiveControls& controls);

  /** Reads one hardware sample and returns up to two ordered semantic actions. */
  uint8_t poll(uint32_t now, UiAction (&actions)[2]);

 private:
  ST77922_TOUCH& touch_;
  TouchDispatcher dispatcher_;
};
