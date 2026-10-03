# Feature: Reactive Lighting Modes

> **Created with:** feature-spec skill
>
> **Location:** `specs/feature-reactive-lighting-modes/specs-reactive-lighting-modes-funct-and-tech.md`

---

## Functional Specification

### Problem Statement

The controller currently uses all six right-side buttons as static, editable
color presets. It cannot produce ambient animation or react to music, which
limits its usefulness as a wall controller for decorative LED strips.

### Proposed Solution

Keep P1-P4 as editable solid-color presets and replace P5 and P6 with dedicated
lighting modes:

- **Rainbow mode (P5):** continuously moves through the full hue spectrum while
  applying a slow breathing brightness envelope. External arrays show a
  distributed rainbow that moves along the strip; the onboard pixel cycles
  through the same spectrum.
- **Music mode (P6):** reads the onboard analog microphone through the ES8311
  codec and pulses a slowly changing saturated color and its brightness from
  the detected audio envelope.

Touching the wheel or P1-P4 returns to solid-color mode. The brightness slider
sets the maximum output brightness for every mode. The power control suppresses
all output without discarding the active mode.

### Functional Requirements

- [x] P1-P4 continue to recall colors on tap and store colors on a 700 ms hold.
- [x] P5 activates a full-spectrum rainbow animation.
- [x] Rainbow brightness rises and falls smoothly on a slow, repeating cycle.
- [x] P6 activates music-reactive color and brightness using the onboard mic.
- [x] P6 displays a recognizable musical-note icon.
- [x] The active effect button has a visible selected state.
- [x] Touching the wheel or P1-P4 exits an effect and restores solid output.
- [x] The brightness slider acts as a maximum brightness in every mode.
- [x] Power off forces all pixels dark and power on resumes the selected mode.
- [x] Effects remain responsive to touch and do not block the main loop.
- [x] A microphone or codec initialization failure leaves the rest of the
  controller operational and reports the failure over Serial.

### User Scenarios

1. **Ambient rainbow:** The user taps P5. The onboard pixel cycles through color
   while breathing; an attached strip displays a moving spatial rainbow.
2. **Music response:** The user taps the musical-note P6 button. Quiet audio
   produces a dim glow and music transients produce brighter pulses.
3. **Control an effect:** The user adjusts brightness while an effect is active;
   the effect continues with its output capped by the new value.
4. **Return to a static color:** The user taps P1 or touches the wheel and the
   animation stops immediately in favor of that solid color.
5. **Unavailable microphone:** Codec input initialization fails. P6 remains
   selectable but produces a quiet fallback glow, while Serial reports the
   hardware error and all other controls continue to work.

### Out of Scope

- FFT-based frequency bands, beat classification, or tempo estimation.
- User-configurable animation speed, microphone gain, or sensitivity controls.
- Persistent mode selection across resets.
- Replacing the existing touch loop with a coroutine scheduler.
- Audio playback while music mode is active.

### Open Questions (Functional)

- [ ] Tune the perceived breathing speed and microphone sensitivity on the
  physical controller after the first firmware build.

---

## Technical Specification

### Architecture Impact

`ControllerModel` gains an explicit output mode while retaining the selected
solid color and brightness. A hardware-independent reactive-lighting helper
produces rainbow colors, breathing values, and a smoothed music envelope. The
main sketch renders the two dedicated mode controls and updates active effects
at a bounded frame rate.

`AudioFeedback` becomes a small duplex audio service: TX continues to provide
button beeps, while RX samples the ES8311 microphone without blocking. The
minimal ES8311 driver gains the two vendor-proven analog microphone register
writes. Existing touch and display drivers remain unchanged.

### Dependencies

- **Internal:** `ColorMath`, `ControllerModel`, `AudioFeedback`, and the existing
  timer-driven Arduino loop.
- **External:** Existing ESP32 Arduino I2S driver and included NeoPixel/display
  libraries. No new library is required.
- **Reference:** Freenove `Sketch_07.2_Echo` confirms GPIO 16 as I2S input and
  the ES8311 analog microphone configuration (`ADC_REG17=0xC8`,
  `SYSTEM_REG14=0x1A`).

### Technical Requirements

- [x] Use `millis()`-based effect timing and unsigned rollover-safe arithmetic.
- [x] Limit effect output updates to a fixed interval to avoid needless
  NeoPixel writes and preserve touch responsiveness.
- [x] Read microphone samples with a zero-timeout/non-blocking I2S call.
- [x] Adapt to ambient noise with a slowly moving noise floor and a faster
  attack/slower release envelope.
- [x] Keep animation math host-testable and independent of Arduino headers.
- [x] Preserve button beeps outside music mode without competing blocking work.
- [x] Avoid per-frame full-screen redraws; only interaction-driven UI changes
  should flush the display.

### Implementation Considerations

- **Approach:** Use the existing loop as a cooperative scheduler. A 25 ms
  effect tick updates LEDs, while touch and saved-preset timers continue on
  every loop. Rainbow hue and breathing are derived from absolute time, so
  missed frames do not accumulate drift.
- **Music response:** Convert short microphone blocks to mean absolute sample
  amplitude, remove an adaptive noise floor, normalize against a tracked peak,
  and apply attack/release smoothing. Use the result to scale the user's
  brightness and advance a saturated color.
- **UI:** P5 uses a compact rainbow gradient and text; P6 uses drawing
  primitives for a note icon so no Unicode font dependency is introduced.
- **Risks:** Microphone levels vary between boards and enclosures; codec
  full-duplex behavior and visual sensitivity require on-device tuning.
- **Alternatives considered:** SimpleAwait would add a dependency without
  simplifying these independent periodic state machines. FFT processing is
  heavier than needed for amplitude-reactive pulsing.

### Open Questions (Technical)

- [ ] Confirm the ES8311 left-channel sample alignment and final sensitivity on
  physical hardware.

---

## Feature-Specific Context

### Requirements & Constraints

- P5 is no longer an editable preset and becomes rainbow mode.
- P6 is no longer an editable preset and becomes music mode.
- Music mode must use the onboard microphone.
- The feature must work for the onboard NeoPixel and optional external array.
- Documentation and the existing feature specification must be updated.
- An alternative model must review the finished change.

### Implementation Guidance

- Reuse the Freenove board samples for compatible pin and codec configuration.
- Prefer the existing simple event loop; use SimpleAwait only if a genuinely
  blocking sequence cannot be represented clearly as timer-driven state.
- Keep hardware-independent response math covered by host-side tests.

---

## User Story Map

### MVP Release Slice

| Activity | Select a mode | Experience the effect | Adjust output | Recover and switch |
|---|---|---|---|---|
| User steps | Tap rainbow or music | Watch hue/breathing or music pulses | Move brightness slider | Toggle power or choose a static color |
| Tasks | Replace P5/P6 preset actions and draw mode controls | Schedule rainbow frames and microphone envelope updates | Scale effect output by model brightness | Preserve active mode across power and exit it on wheel/P1-P4 |

---

## PROPOSED IMPLEMENTATION STEPS

> **Status tags**: `[COMPLETED]` | `[IN PROGRESS]` | `[]` (pending)

### Stage 1: Research and Design

- [COMPLETED] Step 1.1 - Trace preset input, model state, LED writes, audio
  initialization, rendering, and test coverage.
- [COMPLETED] Step 1.2 - Inspect the Freenove music and echo examples for the
  compatible microphone pin, I2S format, and ES8311 ADC configuration.
- [COMPLETED] Step 1.3 - Define mode transitions, brightness/power semantics,
  failure behavior, and a non-blocking effect architecture.

### Stage 2: Testable Mode Foundation

- [COMPLETED] Step 2.1 - Add explicit solid, rainbow, and music mode state,
  reducing editable presets to P1-P4.
- [COMPLETED] Step 2.2 - Add pure rainbow, breathing, brightness-scaling, and adaptive
  audio-envelope helpers.
- [COMPLETED] Step 2.3 - Extend host tests for mode transitions and deterministic effect
  math, and correct the stale source include paths in the test harness.

### Stage 3: Rainbow Mode and UI

- [COMPLETED] Step 3.1 - Refactor NeoPixel output so solid and animated frames share a
  safe brightness/power write path.
- [COMPLETED] Step 3.2 - Schedule rainbow frames with per-pixel hue offsets and a slow
  breathing envelope.
- [COMPLETED] Step 3.3 - Render P5 as a rainbow control and visibly mark it active.

### Stage 4: Microphone-Reactive Music Mode

- [COMPLETED] Step 4.1 - Configure ES8311 analog microphone input and an I2S RX channel
  on GPIO 16 while preserving beep output.
- [COMPLETED] Step 4.2 - Read microphone blocks without blocking and feed the adaptive
  envelope into music-mode color and brightness.
- [COMPLETED] Step 4.3 - Render P6 with a musical-note icon, expose unavailable-input
  diagnostics, and visibly mark the mode active.

### Stage 5: Documentation, Validation, and Review

- [COMPLETED] Step 5.1 - Update the original controller specification and README with
  mode behavior, architecture, build guidance, and physical test steps.
- [COMPLETED] Step 5.2 - Run host-side tests and compile the complete ESP32-S3 firmware.
- [COMPLETED] Step 5.3 - Have an alternative model review the implementation, address
  substantive findings, and rerun affected validation.
- [COMPLETED] Step 5.4 - Record hardware-only follow-up checks and final learnings.

---

## Learnings & Notes

### Patterns Discovered

- The existing main loop already provides cooperative scheduling for touch,
  timers, and a bounded animation tick; coroutines would not reduce complexity.
- Freenove's matching 3.5-inch ST77922 configuration uses GPIO 16 for ES8311
  serial audio input and GPIO 15 for output.
- The existing codec initialization powers the ADC but omits the microphone
  routing and gain writes used by Freenove's echo example.

### Issues Encountered

- The current host test included references to the old `firmware/` source folder;
  the production source now lives under `src/`.
- Local Application Control blocks the ESP32 package's bundled Python
  executables. The firmware was validated by overriding those recipes with the
  installed `esptool` Python module and the packaged `gen_esp32part.py` source.
- Alternative-model review found that the initially enabled RX channel would
  not own BCLK/WS when TX was initialized first. Initializing RX first keeps the
  microphone clocked between beeps. The same review led to draining all queued
  microphone DMA blocks, preventing noise-floor drift during sustained music,
  and making preset holds exit effects consistently.

### Notes for Future Work

- Hardware testing should tune microphone sensitivity, noise-floor adaptation,
  and breathing period rather than adding user-facing controls prematurely.
- Final host tests and the complete ESP32-S3 build pass. Physical microphone,
  LED-strip, and perceived-effect checks remain hardware-only.
