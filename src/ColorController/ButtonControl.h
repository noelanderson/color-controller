#pragma once

#include "InteractiveControl.h"
#include "UiGeometry.h"

/**
 * Shared rectangular press lifecycle for button-like controls.
 *
 * A button claims only a down event inside its bounds. Once claimed, it keeps
 * receiving movement outside the bounds and decides on release whether the
 * gesture remains valid. Subclasses provide semantics without reimplementing
 * capture bookkeeping.
 */
class ButtonControl : public InteractiveControl {
 public:
  explicit ButtonControl(const Ui::Rect& bounds) : bounds_(bounds) {}

  TouchClaim tryTouchDown(const TouchEvent& event) final {
    if (!contains(event.point)) {
      return {};
    }
    pressed_ = true;
    return {true, onPress(event)};
  }

 protected:
  bool contains(const TouchPoint& point) const {
    return Ui::contains(bounds_, point.x, point.y);
  }

 public:
  UiAction touchMove(const TouchEvent& event) final {
    return pressed_ ? onMove(event, contains(event.point)) : UiAction{};
  }

  UiAction touchUp(const TouchEvent& event) final {
    if (!pressed_) {
      return {};
    }
    const bool releasedInside = contains(event.point);
    pressed_ = false;
    return onRelease(event, releasedInside);
  }

 protected:
  virtual UiAction onPress(const TouchEvent&) { return {}; }
  virtual UiAction onMove(const TouchEvent&, bool) { return {}; }
  virtual UiAction onRelease(const TouchEvent&, bool releasedInside) = 0;

  int16_t x() const { return bounds_.x; }
  int16_t y() const { return bounds_.y; }
  int16_t width() const { return bounds_.width; }
  int16_t height() const { return bounds_.height; }

 private:
  Ui::Rect bounds_;
  bool pressed_ = false;
};
