#include "InteractionController.h"

#include <Arduino.h>

#include "UiLayout.h"

InteractionController::InteractionController(ST77922_TOUCH& touch, InteractiveControls& controls,
                                             UiActionProcessor& actionProcessor)
    : touch_(touch), actionProcessor_(actionProcessor), dispatcher_(controls) {}

void InteractionController::poll(uint32_t now) {
  TouchPoint point;
  const bool touching = touch_.Get_Touch();
  if (touching) {
    point.x = constrain(static_cast<int16_t>(touch_.touch.x[0]), 0, Ui::kWidth - 1);
    point.y = constrain(static_cast<int16_t>(touch_.touch.y[0]), 0, Ui::kHeight - 1);
  }

  actionProcessor_.process(dispatcher_.update(touching, point, now), now);
  actionProcessor_.process(dispatcher_.tick(now), now);
}
