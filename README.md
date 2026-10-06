# Touch Color Controller

[![Build firmware](https://github.com/noelanderson/color-controller/actions/workflows/build-firmware.yml/badge.svg)](https://github.com/noelanderson/color-controller/actions/workflows/build-firmware.yml)
[![Publish firmware reference](https://github.com/noelanderson/color-controller/actions/workflows/publish-doxygen.yml/badge.svg)](https://github.com/noelanderson/color-controller/actions/workflows/publish-doxygen.yml)

Touch Color Controller turns an Elecrow 3.5-inch ESP32-S3 capacitive display
(DLE06235B) into a wall-panel controller for addressable LED lighting.

The firmware controls the onboard WS2812-compatible LED and can drive an
optional external strip from GPIO45. It provides editable color presets,
rainbow and microphone-reactive modes, brightness and power controls, and
persistent colors.

## Features

- 480x320 landscape touch interface
- Color-wheel selection with live preview
- Four persistent color presets
- Slow rainbow breathing effect
- Microphone-reactive music effect
- Brightness and non-destructive power controls
- Optional external WS2812-compatible strip
- Fixed-memory cooperative scheduling with event-driven SimpleAwait idle waiting
- Explicit, reusable UI composition in the Arduino sketch

## Using the interface

| Control | Behavior |
|---|---|
| Color wheel | Touch or drag to select hue and saturation. Selecting a color exits rainbow or music mode. |
| P1-P4 | Tap to recall. Hold inside for at least 700 ms to store the current color; `SAVED` confirms persistence. |
| Rainbow | Cycle through the full spectrum with a slow breathing envelope. External strips show a spatial rainbow. |
| Music | Pulse changing colors and brightness from onboard microphone amplitude. |
| ON/OFF | Disable or restore LED output without discarding color, brightness, or the active mode. |
| Brightness | Drag from 0 to 255. In effect modes this sets the maximum effect brightness. |

P1-P4 are saved when `SAVED` appears. A manually selected or recalled solid
color is saved after it remains unchanged for two minutes. Effect-generated
colors are never persisted, and entering an effect cancels a pending solid-color
save. Startup restores the saved solid color and presets.

The music button shows no status dot during audio initialization. A red dot
means microphone initialization failed; it never means recording. Music mode
continues with a dim fallback color cycle when microphone input is unavailable.

## Supported hardware

| Function | ESP32-S3 pin |
|---|---:|
| Onboard addressable LED data | GPIO40 |
| LCD QSPI | GPIO9-GPIO14 |
| LCD backlight | GPIO41 |
| Touch SDA/SCL | GPIO38/GPIO39 |
| Touch reset/interrupt | GPIO48/GPIO47 |
| ES8311 microphone I2S data | GPIO16 |
| ES8311 speaker I2S data | GPIO15 |
| Optional external LED data on P2 | GPIO45 |
| P2 alternate signal | GPIO46 |

GPIO45 and GPIO46 are strapping pins. GPIO45 is output-capable and is used for
the optional LED strip. GPIO46 is input-only on ESP32-S3 and is not suitable for
LED data output. External circuitry must not drive or back-power either pin
during reset or while the ESP32-S3 is unpowered.

On ESP32-S3R8 devices, VDD_SPI is fixed to 3.3 V by eFuse, so the GPIO45 strap
does not select the flash/PSRAM supply voltage. GPIO46 still participates in
boot configuration.

## Prerequisites

- Arduino IDE 2.x or Arduino CLI
- Espressif ESP32 Arduino core 3.3.x
- Python with `esptool` for the Windows build/upload wrapper
- Doxygen 1.9 or later (optional, for generating the firmware API reference)
- Repository libraries:
  - `AddressableLedStrip`
  - `ES8311`
  - `ST77922`
  - `ST77922_TOUCH`
  - `TFT_eSPI`
  - `SimpleAwait`

The repository includes a minimal
[`AddressableLedStrip`](libraries/AddressableLedStrip/README.md) library that
uses the ESP32 RMT peripheral directly.
The focused [`ES8311`](libraries/ES8311/README.md) library attaches the codec to
the display board's existing shared I2C bus.

### Board settings

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| Flash Size | 16 MB |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| CPU Frequency | 240 MHz |

OPI PSRAM is required for the approximately 307 KB RGB565 framebuffer and the
vendor display driver's full-frame transfer buffer.

## Generate the firmware reference

Browse the published
[firmware API reference](https://noelanderson.github.io/color-controller/).
GitHub Actions regenerates and deploys it after first-party firmware or the
Doxygen configuration changes on `main`.

The repository-root `Doxyfile` generates an HTML reference for first-party
firmware under `src/ColorController`. Vendored libraries are intentionally
outside the input set.

From the repository root, run:

```powershell
doxygen Doxyfile
```

Open `build/doxygen/html/index.html` in a browser. Doxygen warnings are written
to `build/doxygen/warnings.log`. The complete output remains under the ignored
`build/` directory so it can be reviewed locally without committing generated
files.

## Build and upload

### Recommended Windows workflow

From the repository root:

```powershell
python -m pip install esptool
.\tools\build-firmware.ps1
```

The wrapper serializes concurrent builds, recovers from interrupted build
caches, and avoids packaged Python executables that can be blocked by Windows
Application Control.

Useful options:

```powershell
# Clean onboard-only build
.\tools\build-firmware.ps1 -Clean

# Build for a 60-pixel external strip
.\tools\build-firmware.ps1 -ExternalPixelCount 60

# Build and upload
.\tools\build-firmware.ps1 -Port COM8

# Upload an existing matching build without recompiling
.\tools\build-firmware.ps1 -UploadOnly -Port COM8
```

Use the same `-ExternalPixelCount` value for `-UploadOnly` that was used to
produce the selected output folder.

### Direct Arduino CLI

```powershell
arduino-cli compile `
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc" `
  --libraries ".\libraries" `
  --output-dir ".\build\onboard-only" `
  ".\src\ColorController"
```

Run only one raw `arduino-cli compile` for this sketch at a time because its
default incremental cache is shared.

To enable an external strip without the wrapper:

```text
--build-property "compiler.cpp.extra_flags=-DCOLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT=60"
```

### Firmware images

| File | Use |
|---|---|
| `ColorController.ino.bin` | Application image uploaded at `0x10000`; normal uploads preserve NVS. |
| `ColorController.ino.bootloader.bin` | Bootloader image. |
| `ColorController.ino.partitions.bin` | Partition table. |
| `ColorController.ino.merged.bin` | Complete image flashed at `0x0`; flashing it erases NVS and resets stored colors. |

The GitHub Actions firmware workflow publishes the merged image as a
30-day artifact named `color-controller-merged-<commit-sha>`.

### Create a release

Push a semantic version tag to build a permanent GitHub Release from that exact
revision:

```powershell
git tag v1.0.0
git push origin v1.0.0
```

Tags must use `vMAJOR.MINOR.PATCH` or a prerelease suffix such as
`v1.1.0-rc.1`. The release workflow rebuilds the onboard-only firmware, creates
generated release notes, and attaches:

- `color-controller-<tag>.merged.bin`
- `color-controller-<tag>.merged.bin.sha256`

Prerelease tags automatically create GitHub prereleases. The merged image is a
complete flash image for address `0x0`; installing it erases NVS and resets
stored colors.

## Connecting an external LED strip

Build with the strip's exact pixel count and connect its data input to GPIO45 on
P2. Do not power the strip from a GPIO.

Recommended installation:

- A separate, correctly sized 5 V supply
- Common ground between the supply and ESP32-S3
- A 3.3 V-to-5 V unidirectional logic-level shifter
- A 330-500 ohm series resistor near the first pixel
- Bulk capacitance across the strip's 5 V input

Budget up to approximately 60 mA per RGB pixel for worst-case full white.
Confirm the P2 connector pin order against the board schematic before making a
cable. The level shifter input must remain high impedance during reset and when
the controller is unpowered.

## Customizing the UI

The UI composition and geometry are intentionally visible near the top of
[`ColorController.ino`](src/ColorController/ColorController.ino).

To build a different screen:

1. Declare each element with its geometry and a unique application-owned ID
   from [`UiElementIds.h`](src/ColorController/UiElementIds.h).
2. Register interactive elements with `InteractiveControls`.
3. Register drawable elements with `UiScene` in back-to-front draw order.
4. Register non-interactive elements, such as `ColorPreviewControl`, only with
   `UiScene`.
5. Keep interactive touch regions spatially disjoint.
6. Increase `InteractiveControls::kCapacity` or `UiScene::kCapacity` only if
   the new composition needs more entries; failed registration is reported at
   startup.
7. If the new UI requires additional coroutines, update the fixed task and
   frame-pool budgets in [`AwaitConfig.h`](src/ColorController/AwaitConfig.h).
   The current application uses six steady-state slots plus one transient slot
   for cooperative tone playback. Eight configured slots and 3072 frame-pool
   bytes leave one slot for startup diagnostics or a bounded extension.

An element may implement:

- `InteractiveControl` for touch claiming, movement, release, and timed
  lifecycle work
- `UiElement` for initial drawing, regular drawing, and notifications
- Both contracts for a normal interactive control

The dispatcher offers touch-down data to registered interactive elements. The
element that claims the contact exclusively receives subsequent move and
debounced release events. Elements emit semantic `UiAction` values rather than
writing persistence, LEDs, audio, or application state directly.

Shared visual style constants live in
[`UiLayout.h`](src/ColorController/UiLayout.h). Element geometry belongs to the
composition root and is used by the element for both drawing and hit testing.

See [UX interaction and class diagrams](docs/ux-interaction-and-class-diagrams.md)
for the complete event and ownership model.

## Verify a board

After flashing:

1. Confirm landscape orientation and alignment for every control.
2. Drag through red, green, and blue on the wheel and verify LED channel order.
3. Tap each preset once.
4. Hold each preset until `SAVED`, recall it, then reboot and confirm persistence.
5. Sweep brightness to both endpoints.
6. Toggle power after changing color and brightness.
7. Verify rainbow cycling and breathing.
8. Verify music response to transients and its slower decay.
9. Adjust brightness and power in both effect modes.
10. Release a preset outside its bounds and confirm no recall or store occurs.
11. Power-cycle repeatedly, especially when an external GPIO45 strip is fitted.
12. Verify a stable solid color survives reboot after two minutes.
13. Verify entering an effect before two minutes prevents saving an effect color.

## Troubleshooting

| Symptom | Check |
|---|---|
| Blank display | Confirm OPI PSRAM and the required partition/flash settings. Check Serial for framebuffer or display initialization failures. |
| Touch does not respond | Confirm display/touch rotation is 1 and inspect Serial for touch initialization warnings. |
| Music button has a red dot | Microphone initialization failed. Check ES8311/I2S wiring and Serial diagnostics. |
| External strip does not light | Confirm the compiled pixel count, GPIO45 data wiring, common ground, level shifting, and external 5 V power. |
| Board does not boot with external hardware | Ensure nothing drives GPIO45 or GPIO46 during reset. |
| Presets do not survive reboot | Wait for `SAVED`; avoid flashing a merged image at `0x0`, which erases NVS. |
| Packaged ESP32 tools fail under Windows policy | Use `tools\build-firmware.ps1` and install Python `esptool`. |

## Development

Run the host-side tests from a Visual Studio Developer PowerShell:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /std:c++20 /EHsc `
  /Fo:build\ `
  /Fe:build\controller_tests.exe `
  tests\controller_tests.cpp `
  src\ColorController\UiActionProcessor.cpp
.\build\controller_tests.exe
```

The tests cover color conversion, wheel and brightness mapping, preset gestures,
touch capture/debounce, collection capacity and duplicate registration,
reactive effects, persistence policy, and timer rollover.

Implementation constraints and contributor validation commands are documented
in [`AGENTS.md`](AGENTS.md). Detailed requirements and implementation status are
kept in the [technical specification](specs/feature-touch-color-controller/specs-touch-color-controller-funct-and-tech.md),
not in this adopter README.
