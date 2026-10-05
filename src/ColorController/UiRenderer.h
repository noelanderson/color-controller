#pragma once

#include <ST77922.h>
#include <TFT_eSPI.h>

#include "ColorMath.h"
#include "ControllerModel.h"
#include "UiLayout.h"
#include "UiScene.h"

/**
 * Owns framebuffer transfers and supplies consistent render context to UiScene.
 *
 * Construction only binds references; setup must initialize the display and
 * allocate the canvas before any drawing method is called. Scene composition,
 * element geometry, and draw order remain outside this adapter.
 */
class UiRenderer {
 public:
  UiRenderer(TFT_eSprite& canvas, ST77922& display, ControllerModel& model, UiScene& scene);

  /** Transfers the complete framebuffer; the panel driver does not accept partial sprite pushes. */
  void flushDisplay();
  /**
   * Redraws one registered element without transferring the frame.
   *
   * @return false when the ID is absent; the first absence is reported to
   *         Serial so composition errors cannot fail silently.
   */
  bool drawElement(UiElementId id, const RgbColor& effectPreviewColor);
  /**
   * Delivers element-local state without transferring the frame.
   *
   * @return false when the target ID is not registered.
   */
  bool notifyElement(UiElementId id, const UiNotification& notification);
  /** Redraws all dynamic scene content and transfers the complete frame. */
  void drawDynamicUi(const RgbColor& effectPreviewColor);
  /** Clears the framebuffer, performs initial scene drawing, and transfers the frame. */
  void drawInitialUi(const RgbColor& effectPreviewColor);

 private:
  UiRenderContext context(const RgbColor& effectPreviewColor) const;

  TFT_eSprite& canvas_;
  ST77922& display_;
  ControllerModel& model_;
  UiScene& scene_;
  bool missingElementReported_ = false;
};
