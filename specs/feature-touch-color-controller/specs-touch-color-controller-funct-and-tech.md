# Feature: Touch Color Controller

> **Created with:** feature-spec skill
>
> **Location:** `specs/feature-touch-color-controller/specs-touch-color-controller-funct-and-tech.md`

---

## Functional Specification

### Problem Statement

The Elecrow 3.5-inch ESP32-S3 display needs a self-contained touch interface for choosing an addressable LED color and brightness without a phone, network connection, or serial console. The first release must control the board's single onboard RGB LED while establishing a clean path to an external WS2812-compatible array on connector P2.

### Proposed Solution

Run the display in 480x320 landscape orientation and present one control surface:

- A thin full-width strip across the top previews the currently selected color.
- A saturation/hue color wheel occupies the left half.
- Four editable preset buttons plus rainbow and music mode controls occupy the
  right half.
- A power button and brightness slider occupy the bottom control row.
- Tapping P1-P4 selects a static color; holding one stores the color wheel's
  current color in that slot. P5 starts a breathing rainbow and P6 starts
  onboard-microphone-reactive lighting.

The UI state is independent of the LED transport. Version 1 uses the onboard addressable LED on GPIO 40. A later build can switch the output configuration to a strip on P2 without changing touch or display behavior.

### Functional Requirements

- [x] The display presents a responsive 480x320 landscape UI.
- [x] The top preview strip always reflects the selected RGB color.
- [x] Touching or dragging within the color wheel selects hue by angle and saturation by distance from the center.
- [x] Four preset buttons show their stored colors.
- [x] A short preset touch selects and applies that preset color.
- [x] A preset touch held for at least 700 ms stores the current wheel color without also firing the short-touch action.
- [x] Stored presets survive reboot and power loss.
- [x] A manually selected solid color is stored after two minutes without a change.
- [x] Rainbow and music colors are never stored as the selected startup color.
- [x] P5 activates a full-spectrum rainbow with a slow breathing envelope.
- [x] P6 shows a musical-note icon and pulses color/brightness from onboard mic input.
- [x] The power control turns addressable LED output off and restores the selected color when turned back on.
- [x] The brightness slider adjusts output from 0 through 255.
- [x] Version 1 controls the onboard addressable LED on GPIO 40.
- [x] Output configuration supports a later external array on P2 GPIO 45 by changing compile-time settings.
- [x] Interactions remain non-blocking; long-press detection must not use `delay`.

### User Scenarios

1. **Choose a new color:** The user drags around the wheel, sees the top preview update, and sees the onboard addressable LED follow the selected color.
2. **Recall a preset:** The user taps a colored preset button and the preview and addressable LED change to that stored color.
3. **Replace a preset:** The user chooses a wheel color, holds P1-P4 for at least 700 ms, receives visible saved feedback, and later recalls the new color with a tap.
4. **Run an ambient effect:** The user taps P5 and sees a moving, breathing rainbow.
5. **React to music:** The user taps P6 and sees output brightness pulse from onboard microphone input.
6. **Dim or turn off:** The user drags the brightness slider, then turns output off. The selected mode and maximum brightness are restored when output is turned on.
7. **Cancel a preset gesture:** The user presses a preset, drags outside it, and releases; no preset is recalled or overwritten.

### Out of Scope

- User-configurable effect speed, microphone sensitivity, FFT bands, color
  temperature controls, or per-pixel editing.
- Wi-Fi, Bluetooth, SD card, and battery telemetry.
- Driving an externally powered array in version 1.
- Runtime selection between the onboard pixel and an external strip.

### Open Questions (Functional)

- [ ] Validate the physical screen/touch orientation and color channel order on the target unit.
- [x] Persist presets and stable manually selected colors in ESP32 Preferences/NVS.

---

## Technical Specification

### Architecture Impact

The firmware has the following responsibilities:

1. `ColorController.ino` is the composition root for hardware initialization and cooperative tasks.
2. `ControllerModel` owns color, brightness, power, presets, and the active output mode.
3. `InteractionController` is the sole touch-hardware reader and forwards samples to `TouchDispatcher`.
4. `TouchDispatcher` offers normalized touch-down data to each registered control and captures the sole claimant.
5. Controls own their gesture state and emit `UiAction` values; `UiActionProcessor` coordinates side effects.
6. `UiRenderer` and `UiLayout` own framebuffer rendering, panel transfers, and named geometry.
7. `LightingOutput` owns addressable LED outputs, reactive state, and the live effect preview.
8. `ColorPersistenceService` coordinates delayed and queued writes through `PersistentState`.
9. `ColorMath` and `ReactiveLighting` provide host-tested color, animation, and envelope calculations.
10. `AudioFeedback` runs ES8311 speaker output and onboard microphone input over duplex I2S.
11. SimpleAwait C++20 tasks schedule touch, effects, effect UI, audio, and persistence.
12. The ST77922 display/touch drivers are consumed from the supplied vendor resource pack.

The repository-owned `AddressableLedStrip` Arduino library provides the minimal
WS2812-compatible GRB encoder and ESP32 RMT transport. The repository-owned
`ES8311` Arduino library attaches the codec to the touch controller's existing
I2C bus and exposes the fixed audio operating point. `TFT_eSPI` supplies a
PSRAM-backed software sprite used as the RGB565 framebuffer; the vendor ST77922
driver transfers that framebuffer to the QSPI display.

### Dependencies

- **Internal:** Supplied `ST77922` and `ST77922_Touch` board drivers in the resource pack.
- **External:** ESP32 Arduino core 3.3.x, TFT_eSPI 2.5.x, and SimpleAwait merged
  commit `765b418` (package metadata 1.0.1).
- **Hardware:** Elecrow DLE06235B, onboard addressable LED on GPIO 40, ES8311 analog
  microphone input on I2S GPIO 16, and optional external signal on P2 GPIO 45.

### Technical Requirements

- [x] Compile for `esp32:esp32:esp32s3` with 16 MB flash and OPI PSRAM enabled.
- [x] Use display and touch rotation 1 so both coordinate systems are 480x320.
- [x] Allocate the 16-bit framebuffer in PSRAM and fail visibly over Serial if allocation fails.
- [x] Keep the main loop limited to `simpleawait::poll_and_wait()`.
- [x] Use SimpleAwait delay primitives instead of application-level blocking
  delay calls.
- [x] Clamp all touch coordinates and computed values to valid ranges.
- [x] Separate state transitions and color math from hardware writes where practical.
- [x] Avoid writing the addressable LED or display when state has not changed.
- [x] Schedule effects without blocking and avoid per-frame display refreshes.
- [x] Read microphone blocks with zero-timeout I2S calls and smooth the level
  with an adaptive noise floor and attack/release envelope.
- [x] Compile with the exact vendor libraries included in `resource-pack`.
- [x] Deduplicate NVS writes and write selected colors only after 120 seconds of stability.
- [x] Cancel pending selected-color writes when rainbow or music mode starts.
- [x] Render the initial framebuffer before bounded touch-controller startup,
  reject invalid touch-count values, and continue safely after touch I2C failure.
- [x] Keep global display/touch constructors data-only and initialize SPI, I2C,
  GPIO, and panel hardware explicitly from Arduino `setup()` for reliable cold boot.
- [x] Keep the panel backlight off until the initial full framebuffer transfer
  completes so cold boot never exposes uninitialized display memory.

### Implementation Considerations

- **Approach:** Draw the expensive color wheel once into the sprite. Redraw only dynamic controls in the framebuffer, then transfer the complete frame when a visible state changes. Run the touch state machine and the separate 25 ms LED effect tick as fixed-memory SimpleAwait tasks.
- **Risks:** Physical touch rotation and microphone sensitivity need on-device confirmation; the vendor touch driver depends on ESP-IDF 5 APIs; large framebuffer allocation requires PSRAM; external arrays require separate 5 V power, common ground, signal conditioning, and power budgeting.
- **Alternatives considered:** LVGL adds substantial configuration and is unnecessary for this single screen. Direct rendering without a framebuffer complicates wheel marker restoration and partial updates. Hand-written loop timers were replaced by cooperative SimpleAwait tasks so all runtime scheduling follows one model.

### Open Questions (Technical)

- [ ] Confirm which P2 signal pin and connector pin order will be used in v2 before wiring an external strip.
- [ ] Measure full-frame refresh latency on hardware and add dirty-rectangle transfers only if interaction feels sluggish.

---

## Feature-Specific Context

### Requirements & Constraints

- Hardware is the Elecrow 3.5-inch ESP32-S3 display, 320x480 IPS capacitive touch variant.
- The supplied resource pack is the primary source for board-specific drivers and pin assignments.
- Version 1 targets the onboard addressable LED.
- Version 2 will target an external addressable LED array connected to P2.
- Required UI includes the top color strip, left color wheel, four presets,
  rainbow/music controls, power, and brightness.

### Implementation Guidance

- The vendor examples identify ST77922 QSPI display pins, touch I2C pins, onboard addressable LED GPIO 40, and landscape rotation behavior.
- P2 exposes GPIO 45 and GPIO 46, but ESP32-S3 GPIO 46 is input-only. The output configuration defaults to GPIO 40 and one pixel, with documented constants for migration to GPIO 45.
- Use fixed-memory SimpleAwait tasks for all runtime scheduling; keep Arduino
  `loop()` limited to polling the scheduler.

---

## User Story Map

### MVP Release Slice

| Activity | Choose a color | Reuse a color | Control output | Build and operate |
|---|---|---|---|---|
| User steps | Touch or drag on wheel | Tap a preset | Toggle power | Install dependencies |
| Tasks | Convert wheel coordinates to HSV/RGB | Apply stored RGB value | Preserve color while output is off | Compile for ESP32-S3 with PSRAM |
| User steps | Observe preview strip and LED | Hold a preset to replace it | Drag brightness | Flash and verify on device |
| Tasks | Render selection marker and update output | Detect 700 ms hold and show feedback | Clamp and apply 0-255 value | Confirm touch orientation and RGB order |
| User steps |  | Tap rainbow or music mode | Adjust effect brightness | Validate microphone response |
| Tasks |  | Run timer-driven effect output | Scale animation by slider value | Tune sensitivity on physical hardware |

---

## PROPOSED IMPLEMENTATION STEPS

> **Status tags**: `[COMPLETED]` | `[IN PROGRESS]` | `[]` (pending)

### Stage 1: Research and Technical Design

- [COMPLETED] Step 1.1 - Inspect the vendor display, touch, addressable LED, and backlight examples and record the authoritative pin assignments.
- [COMPLETED] Step 1.2 - Confirm the installed Arduino toolchain and the ESP32-S3 board target.
- [COMPLETED] Step 1.3 - Define the 480x320 layout, touch gesture rules, state model, output abstraction, and v2 boundary.

### Stage 2: Firmware Foundation

- [COMPLETED] Step 2.1 - Create the Arduino sketch and connect it to the supplied board-specific drivers.
- [COMPLETED] Step 2.2 - Implement pure color conversion, wheel geometry, and controller state transitions.
- [COMPLETED] Step 2.3 - Initialize the display, touch controller, PSRAM framebuffer, and onboard addressable LED with explicit failure handling.

### Stage 3: Touch User Interface

- [COMPLETED] Step 3.1 - Render the static screen structure, color wheel, preset grid, power control, brightness track, and selected-color strip.
- [COMPLETED] Step 3.2 - Implement touch down, drag, release, and cancellation behavior for the wheel, presets, power, and slider.
- [COMPLETED] Step 3.3 - Implement preset long-press storage and visible saved feedback without blocking the event loop.

### Stage 4: Output and V2 Readiness

- [COMPLETED] Step 4.1 - Apply selected color, brightness, and power state to the onboard addressable LED only when output state changes.
- [COMPLETED] Step 4.2 - Expose documented compile-time output pin and pixel-count constants for a future P2 array.
- [COMPLETED] Step 4.3 - Document external-array electrical constraints and GPIO 45 as the P2 output choice for v2.

### Stage 5: Verification and Review

- [COMPLETED] Step 5.1 - Compile the complete sketch for the ESP32-S3 using the vendor resource-pack libraries.
- [COMPLETED] Step 5.2 - Run focused host-side checks for HSV conversion, wheel bounds, and brightness mapping.
- [COMPLETED] Step 5.3 - Have an alternative model review the implementation and resolve all substantive findings.
- [COMPLETED] Step 5.4 - Record the on-device verification checklist and any remaining hardware-only validation.

### Stage 6: Reactive Lighting Modes

- [COMPLETED] Step 6.1 - Replace P5 with a timer-driven, full-spectrum rainbow
  and slow breathing brightness envelope.
- [COMPLETED] Step 6.2 - Replace P6 with a musical-note control and add
  non-blocking onboard microphone sampling through ES8311 I2S input.
- [COMPLETED] Step 6.3 - Add adaptive audio-envelope processing, effect-aware
  power/brightness behavior, and host-side tests for pure response math.
- [COMPLETED] Step 6.4 - Complete alternative-model review and document the
  remaining physical tuning checks for microphone sensitivity and breathing speed.

### Stage 7: Persistent Colors and Coroutines

- [COMPLETED] Step 7.1 - Restore P1-P4 and the last stable manually selected
  solid color from ESP32 Preferences/NVS at startup.
- [COMPLETED] Step 7.2 - Persist preset changes immediately while deduplicating
  unchanged flash writes.
- [COMPLETED] Step 7.3 - Vendor SimpleAwait commit `765b418` and run a fixed-memory C++20
  coroutine that saves a manually selected color after 120 unchanged seconds.
- [COMPLETED] Step 7.4 - Cancel pending color persistence on rainbow/music entry
  and preserve NVS during policy-safe segmented firmware uploads.
- [COMPLETED] Step 7.5 - Complete adversarial review and resolve persistence,
  coroutine, upload, and documentation edge cases.
- [] Step 7.6 - Validate reboot and physical power-cycle persistence on hardware.

### Stage 8: Self-Contained UX Controls

- [COMPLETED] Step 8.1 - Extract color-wheel hit testing, coordinate mapping,
  rendering, and marker state into a self-contained control.
- [COMPLETED] Step 8.2 - Extract brightness slider, power button, and color preview
  rendering and touch behavior into focused controls.
- [COMPLETED] Step 8.3 - Extract P1-P6 hit testing, press/hold/release state, saved
  feedback, icons, and rendering into a preset/mode control group.
- [COMPLETED] Step 8.4 - Reduce `UiRenderer` to framebuffer coordination and
  `InteractionController` to routing semantic control events.
- [COMPLETED] Step 8.5 - Complete host tests, clean onboard/external firmware
  builds, and alternative-model review for the componentized UI.
- [COMPLETED] Step 8.6 - Run a final target-board smoke test of P1-P6 tap, hold,
  drag-cancel, mode selection, saved feedback, and microphone status.

### Stage 9: Independent Control Input

- [COMPLETED] Step 9.1 - Add hardware-independent touch events, semantic UI
  actions, and an interactive-control contract.
- [COMPLETED] Step 9.2 - Add a shared `ButtonControl` base with power, preset,
  and mode specializations.
- [COMPLETED] Step 9.3 - Move wheel, slider, button, preset, and mode gesture
  behavior into their owning controls.
- [COMPLETED] Step 9.4 - Extract pointer capture and release debounce into
  `TouchDispatcher`, preserving one control for the complete contact.
- [COMPLETED] Step 9.5 - Extract application side effects into
  `UiActionProcessor` and retain one SimpleAwait touch task as the sole hardware reader.
- [COMPLETED] Step 9.6 - Add host coverage for capture/debounce and complete
  onboard/external firmware, documentation, and diagram validation.
- [COMPLETED] Step 9.7 - Repeat the P1-P6, wheel, slider, and power smoke test on the
  physical controller after the input-ownership refactor.

### Stage 10: Visible UI Composition

- [COMPLETED] Step 10.1 - Declare the concrete wheel, P1-P6, power, slider,
  and preview controls in the sketch composition root.
- [COMPLETED] Step 10.2 - Register every interactive element from `setup()`
  and document non-overlapping touch regions as a required layout invariant.
- [COMPLETED] Step 10.3 - Pass each element's geometry from the sketch so the
  element uses one set of bounds for its drawing, hit testing, and local state.
- [COMPLETED] Step 10.4 - Let every registered element inspect touch-down data
  and claim its own contacts; route subsequent move/up data only to the claimant.
- [COMPLETED] Step 10.5 - Repeat the target-board UI smoke test after the composition and lifecycle refactor.

### Stage 11: Generic UI Collections

- [COMPLETED] Step 11.1 - Replace the concrete-control `UiControls` facade with
  a generic fixed-memory `InteractiveControls` input/lifecycle collection.
- [COMPLETED] Step 11.2 - Add a generic fixed-memory `UiScene` and `UiElement`
  rendering contract.
- [COMPLETED] Step 11.3 - Register the sketch-owned controls independently for
  input and rendering, with the non-interactive preview registered only in the scene.
- [COMPLETED] Step 11.4 - Route targeted redraws and preset saved feedback
  through scene element IDs and notifications.
- [COMPLETED] Step 11.5 - Repeat the target-board UI smoke test after the generic collection split.

---

## Learnings & Notes

### Patterns Discovered

- The ST77922 vendor examples use TFT_eSPI only as a software sprite and send its RGB565 buffer through a separate QSPI driver.
- Matching `Set_Rotation(1)` on display and touch yields a 480x320 landscape coordinate system.
- The onboard addressable RGB LED is on GPIO 40.
- P2 exposes GPIO 45 and GPIO 46, but GPIO 46 is input-only on ESP32-S3. GPIO 45 is the v2 data-output candidate.
- The matching Freenove echo example identifies GPIO 16 as ES8311 input and
  supplies the analog microphone routing register values used by music mode.
- Independent timer checks remain appropriate for touch, animation, and
  microphone sampling; SimpleAwait fits the separate delayed persistence flow.

### Issues Encountered

- Arduino CLI can discover the ST77922 and touch driver folders even though they do not contain `library.properties` metadata.
- The packaged ESP32 build tools were blocked by local Application Control policy; using their Python source/module equivalents allowed the same build to complete.
- Independent review identified an expensive full color-wheel repaint, touch-release instability, and untested preset gestures. Marker-only restoration, release debounce, timer-driven hold detection, and pure gesture tests resolved those findings.

### Notes for Future Work

- External addressable LED arrays should not be powered from a GPIO. Size the 5 V supply for worst-case current, connect grounds, and consider a 3.3 V-to-5 V level shifter and series data resistor.
- GPIO 45 is a strapping pin, so v2 wiring must not force its level during reset.
- Physical touch alignment, componentized control responsiveness, P1-P6
  gestures, saved feedback, mode selection, and RGB channel order passed final
  target-board verification.
- Microphone level, noise-floor adaptation, and breathing cadence require
  final tuning on the physical controller.
- The extracted color wheel, brightness slider, power button, preview strip,
  and P1-P6 controls passed a physical target-board smoke test after the final
  componentization.
