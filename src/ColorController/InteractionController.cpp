#include "InteractionController.h"

#include <Arduino.h>

#include "UiLayout.h"

InteractionController::InteractionController(ST77922_TOUCH& touch, InteractiveControls& controls)
    : touch_(touch), dispatcher_(controls) {}

uint8_t InteractionController::poll(uint32_t now, UiAction (&actions)[2]) {
  TouchPoint point;
  const bool touching = touch_.Get_Touch();
  if (touching) {
    point.x = constrain(static_cast<int16_t>(touch_.touch.x[0]), 0, Ui::kWidth - 1);
    point.y = constrain(static_cast<int16_t>(touch_.touch.y[0]), 0, Ui::kHeight - 1);
  }

  uint8_t count = 0;
  const UiAction contactAction = dispatcher_.update(touching, point, now);
  if (contactAction.type != UiActionType::kNone) {
    actions[count++] = contactAction;
  }
  const UiAction timedAction = dispatcher_.tick(now);
  if (timedAction.type != UiActionType::kNone) {
    actions[count++] = timedAction;
  }
  return count;
}
