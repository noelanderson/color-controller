#pragma once

#include <stdint.h>

#include "ColorMath.h"
#include "ControllerModel.h"

enum UiRefresh : uint16_t {
  kUiRefreshNone = 0,
  kUiRefreshDynamic = 1U << 0,
  kUiRefreshBrightness = 1U << 1,
  kUiRefreshPower = 1U << 2,
  kUiRefreshMusic = 1U << 3,
  kUiRefreshPreview = 1U << 4,
};

enum class AudioCue : uint8_t {
  kNone,
  kSingle,
  kDouble,
};

enum class ManualColorMessage : uint8_t {
  kNone,
  kChanged,
  kCancelled,
};

struct LightingMessage {
  bool apply = false;
  bool resetMusicEnvelope = false;
  uint16_t uiAfterApply = kUiRefreshNone;
};

struct UiMessage {
  uint16_t refresh = kUiRefreshNone;
  uint8_t redrawPresetMask = 0;
  uint8_t savedPresetMask = 0;
};

struct PersistenceMessage {
  ManualColorMessage manualColor = ManualColorMessage::kNone;
  RgbColor color = {};
  uint32_t now = 0;
  uint8_t presetMask = 0;
  RgbColor presetColors[ControllerModel::kPresetCount] = {};
  uint32_t presetTimes[ControllerModel::kPresetCount] = {};
};

/**
 * Fixed-memory, scheduler-local mailboxes between controller coroutines.
 *
 * Continuous state uses latest-value/coalescing semantics. Preset requests
 * retain the latest value for every slot, so no persisted user command is lost.
 * The single cooperative scheduler makes each publish/take operation atomic.
 */
class ControllerMessages {
 public:
  void requestLighting(bool resetMusicEnvelope = false, uint16_t uiAfterApply = kUiRefreshNone) {
    lighting_.apply = true;
    lighting_.resetMusicEnvelope = lighting_.resetMusicEnvelope || resetMusicEnvelope;
    lighting_.uiAfterApply |= uiAfterApply;
  }

  LightingMessage takeLighting() {
    const LightingMessage message = lighting_;
    lighting_ = {};
    return message;
  }

  void requestUi(uint16_t refresh) { ui_.refresh |= refresh; }

  void requestPresetRedraw(uint8_t index) {
    if (index < ControllerModel::kPresetCount) {
      ui_.redrawPresetMask |= static_cast<uint8_t>(1U << index);
    }
  }

  void notifyPresetSaved(uint8_t index) {
    if (index < ControllerModel::kPresetCount) {
      ui_.savedPresetMask |= static_cast<uint8_t>(1U << index);
    }
  }

  UiMessage takeUi() {
    const UiMessage message = ui_;
    ui_ = {};
    return message;
  }

  void noteManualColor(const RgbColor& color, uint32_t now) {
    persistence_.manualColor = ManualColorMessage::kChanged;
    persistence_.color = color;
    persistence_.now = now;
  }

  void cancelManualColor() { persistence_.manualColor = ManualColorMessage::kCancelled; }

  void queuePresetSave(uint8_t index, const RgbColor& color, uint32_t now) {
    if (index >= ControllerModel::kPresetCount) {
      return;
    }
    persistence_.presetColors[index] = color;
    persistence_.presetTimes[index] = now;
    persistence_.presetMask |= static_cast<uint8_t>(1U << index);
  }

  PersistenceMessage takePersistence() {
    const PersistenceMessage message = persistence_;
    persistence_ = {};
    return message;
  }

  void requestAudio(AudioCue cue) {
    if (static_cast<uint8_t>(cue) > static_cast<uint8_t>(audioCue_)) {
      audioCue_ = cue;
    }
  }

  AudioCue takeAudio() {
    const AudioCue cue = audioCue_;
    audioCue_ = AudioCue::kNone;
    return cue;
  }

 private:
  LightingMessage lighting_;
  UiMessage ui_;
  PersistenceMessage persistence_;
  AudioCue audioCue_ = AudioCue::kNone;
};
