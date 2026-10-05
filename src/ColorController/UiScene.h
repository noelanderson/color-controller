#pragma once

#include "UiElement.h"

/**
 * Fixed-memory ordered collection of sketch-owned drawable elements.
 *
 * Registration order is back-to-front draw order. IDs must be unique because
 * targeted redraws and notifications resolve one element by ID. Registered
 * elements must outlive the scene.
 */
class UiScene {
 public:
  static constexpr uint8_t kCapacity = 12;

  /** Removes all registrations without destroying sketch-owned elements. */
  void clear() {
    count_ = 0;
    for (UiElement*& element : elements_) {
      element = nullptr;
    }
  }

  /**
   * Registers one non-owning element pointer at the front-to-back tail.
   *
   * @return false on capacity overflow or duplicate scene identity.
   */
  bool add(UiElement& element) {
    if (count_ >= kCapacity || find(element.id()) != nullptr) {
      return false;
    }
    elements_[count_++] = &element;
    return true;
  }

  /** Performs the initial back-to-front draw, including static element content. */
  void drawInitial(const UiRenderContext& context) {
    for (uint8_t index = 0; index < count_; ++index) {
      elements_[index]->drawInitial(context);
    }
  }

  /** Redraws the dynamic portion of every element in back-to-front order. */
  void draw(const UiRenderContext& context) {
    for (uint8_t index = 0; index < count_; ++index) {
      elements_[index]->draw(context);
    }
  }

  /** Redraws one element and reports whether the ID was registered. */
  bool draw(UiElementId id, const UiRenderContext& context) {
    UiElement* element = find(id);
    if (element == nullptr) {
      return false;
    }
    element->draw(context);
    return true;
  }

  /** Delivers transient element-local state and reports whether the ID exists. */
  bool notify(UiElementId id, const UiNotification& notification) {
    UiElement* element = find(id);
    if (element == nullptr) {
      return false;
    }
    element->notify(notification);
    return true;
  }

 private:
  UiElement* find(UiElementId id) {
    for (uint8_t index = 0; index < count_; ++index) {
      if (elements_[index]->id() == id) {
        return elements_[index];
      }
    }
    return nullptr;
  }

  UiElement* elements_[kCapacity] = {};
  uint8_t count_ = 0;
};
