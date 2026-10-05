#pragma once

#include <stdint.h>

#include "ColorMath.h"
#include "ControllerModel.h"

/**
 * @file ControllerMessages.h
 * @brief Fixed-memory communication contracts between steady-state coroutines.
 *
 * The controller runs all coroutines on one cooperative scheduler. These
 * mailboxes therefore require no locks, but they still define explicit
 * ownership boundaries: the application task publishes intent and each
 * hardware/service task consumes only its own message domain.
 */

/** Bit mask describing the smallest UI surface that must be redrawn. */
enum UiRefresh : uint16_t {
  /** No redraw is pending. */
  kUiRefreshNone = 0,
  /** Redraw every dynamic element and perform one framebuffer transfer. */
  kUiRefreshDynamic = 1U << 0,
  /** Redraw the brightness label, track, and knob. */
  kUiRefreshBrightness = 1U << 1,
  /** Redraw only the power control. */
  kUiRefreshPower = 1U << 2,
  /** Redraw the music button, including microphone status. */
  kUiRefreshMusic = 1U << 3,
  /** Redraw the live preview strip. */
  kUiRefreshPreview = 1U << 4,
};

/**
 * Audible feedback priority.
 *
 * Values are intentionally ordered by strength. When several actions arrive
 * before the audio task consumes them, a double cue replaces a single cue,
 * while a later single cue cannot downgrade a pending double cue.
 */
enum class AudioCue : uint8_t {
  kNone,
  kSingle,
  kDouble,
};

/** Latest manual-color persistence operation requested by the application. */
enum class ManualColorMessage : uint8_t {
  kNone,
  kChanged,
  kCancelled,
};

/**
 * Coalesced request consumed by the sole lighting task.
 *
 * UI flags travel with the lighting request so the lighting task can publish
 * them only after previewColor() and physical output reflect the new model.
 */
struct LightingMessage {
  bool apply = false;
  bool resetMusicEnvelope = false;
  uint16_t uiAfterApply = kUiRefreshNone;
};

/** Coalesced redraw and transient-notification request for the UI task. */
struct UiMessage {
  uint16_t refresh = kUiRefreshNone;
  uint8_t redrawPresetMask = 0;
  uint8_t savedPresetMask = 0;
};

/**
 * Snapshot transferred from the application task to the persistence task.
 *
 * Manual color updates use latest-value semantics. Preset requests retain one
 * independently addressable value and timestamp per slot, preventing a save
 * for P1 from overwriting a pending save for P2.
 */
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
  /**
   * Requests one lighting application and optional post-apply work.
   *
   * Repeated requests coalesce. Envelope reset is sticky until consumed and UI
   * flags are unioned so no required redraw is lost.
   *
   * @param resetMusicEnvelope Whether the lighting owner must clear adaptive
   *        microphone history before applying the next frame.
   * @param uiAfterApply UI dirty bits to publish after lighting is current.
   */
  void requestLighting(bool resetMusicEnvelope = false, uint16_t uiAfterApply = kUiRefreshNone) {
    lighting_.apply = true;
    lighting_.resetMusicEnvelope = lighting_.resetMusicEnvelope || resetMusicEnvelope;
    lighting_.uiAfterApply |= uiAfterApply;
  }

  /**
   * Atomically takes and clears the pending lighting request.
   *
   * @return A snapshot representing every request since the previous take.
   */
  LightingMessage takeLighting() {
    const LightingMessage message = lighting_;
    lighting_ = {};
    return message;
  }

  /** Unions one or more redraw bits into the pending UI message. */
  void requestUi(uint16_t refresh) { ui_.refresh |= refresh; }

  /**
   * Requests redraw of one preset without asserting persistence success.
   *
   * Used when transient SAVED feedback expires. Invalid indices are ignored at
   * this boundary so they cannot alter unrelated preset bits.
   */
  void requestPresetRedraw(uint8_t index) {
    if (index < ControllerModel::kPresetCount) {
      ui_.redrawPresetMask |= static_cast<uint8_t>(1U << index);
    }
  }

  /**
   * Publishes a confirmed persistence success for one preset.
   *
   * Only the persistence task may call this method, and only after NVS reports
   * a successful write. This invariant prevents success-shaped data loss.
   */
  void notifyPresetSaved(uint8_t index) {
    if (index < ControllerModel::kPresetCount) {
      ui_.savedPresetMask |= static_cast<uint8_t>(1U << index);
    }
  }

  /** Takes and clears all pending UI dirty bits and preset notifications. */
  UiMessage takeUi() {
    const UiMessage message = ui_;
    ui_ = {};
    return message;
  }

  /** Replaces the pending delayed-save candidate with the latest manual color. */
  void noteManualColor(const RgbColor& color, uint32_t now) {
    persistence_.manualColor = ManualColorMessage::kChanged;
    persistence_.color = color;
    persistence_.now = now;
  }

  /** Cancels any pending manual-color candidate before an effect becomes authoritative. */
  void cancelManualColor() { persistence_.manualColor = ManualColorMessage::kCancelled; }

  /**
   * Retains the latest desired persistent value for one preset slot.
   *
   * Multiple saves to the same slot coalesce to the newest value. Saves to
   * different slots remain independently pending.
   */
  void queuePresetSave(uint8_t index, const RgbColor& color, uint32_t now) {
    if (index >= ControllerModel::kPresetCount) {
      return;
    }
    persistence_.presetColors[index] = color;
    persistence_.presetTimes[index] = now;
    persistence_.presetMask |= static_cast<uint8_t>(1U << index);
  }

  /** Takes and clears the complete persistence inbox snapshot. */
  PersistenceMessage takePersistence() {
    const PersistenceMessage message = persistence_;
    persistence_ = {};
    return message;
  }

  /**
   * Coalesces audible feedback without building a stale sound backlog.
   *
   * @param cue Requested cue; stronger pending cues are never downgraded.
   */
  void requestAudio(AudioCue cue) {
    if (static_cast<uint8_t>(cue) > static_cast<uint8_t>(audioCue_)) {
      audioCue_ = cue;
    }
  }

  /** Takes and clears the strongest currently pending audio cue. */
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
