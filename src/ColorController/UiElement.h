#pragma once

#include <stdint.h>

#include "ControllerModel.h"

/** Opaque scene identity; each application composition owns its concrete values. */
using UiElementId = uint8_t;

/**
 * Immutable application state supplied to one scene draw pass.
 *
 * A single context keeps every element in a frame consistent while avoiding
 * direct dependencies from controls to lighting or optional peripheral services.
 */
struct UiRenderContext {
  const ControllerModel& model;
  RgbColor effectPreviewColor;
  uint32_t now;

  /** Returns the solid selection or current effect preview, as appropriate. */
  RgbColor displayedColor() const {
    return model.mode() == OutputMode::kSolid ? model.selected() : effectPreviewColor;
  }
};

enum class UiNotificationType : uint8_t {
  kPresetSaved,
};

/** Out-of-band element state change that does not belong in ControllerModel. */
struct UiNotification {
  UiNotificationType type;
  uint32_t until;
};

/**
 * Drawable scene element owned by the sketch.
 *
 * Elements are registered by reference and must outlive UiScene. drawInitial()
 * exists for expensive static content, such as the wheel bitmap; draw() handles
 * the state that may change during normal operation.
 */
class UiElement {
 public:
  /** Creates an element with an application-owned stable scene identity. */
  explicit UiElement(UiElementId id) : id_(id) {}
  virtual ~UiElement() = default;

  /** @return Identity used for targeted redraws and notifications. */
  UiElementId id() const { return id_; }
  /**
   * Draws static and initial dynamic content.
   *
   * The default delegates to draw(); expensive static elements may override.
   */
  virtual void drawInitial(const UiRenderContext& context) { draw(context); }
  /** Draws the element's current dynamic appearance into the framebuffer. */
  virtual void draw(const UiRenderContext& context) = 0;
  /** Receives optional element-local state not represented by ControllerModel. */
  virtual void notify(const UiNotification&) {}

 private:
  UiElementId id_;
};
