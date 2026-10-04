# Touch Color Controller

Touch-driven color and brightness control for the Elecrow 3.5-inch ESP32-S3
320x480 capacitive display (DLE06235B).

The firmware drives the board's onboard WS2812-compatible addressable LED and,
optionally, an external array wired to P2 with the same color and brightness.

Ultimate goal is for this to be a wall panel control for led strip lights

## User interface

The display runs in 480x320 landscape orientation.

- **Top strip:** previews the selected color.
- **Color wheel:** touch or drag in the left half to choose hue and saturation.
- **Presets:** tap P1-P4 to recall a color. Hold a preset for at least 700 ms to
  replace it with the currently selected color. `SAVED` confirms the long press.
- **P5 Rainbow:** cycles through the full color spectrum with a slow breathing
  brightness effect. External arrays show a moving rainbow across the strip.
- **P6 Music:** uses the onboard microphone to pulse a changing color and its
  brightness with the detected audio level. The musical-note button is outlined
  in green while active.
- **ON/OFF:** disables LED output without forgetting the selected color or brightness.
- **Brightness:** drag the bottom slider from 0 through 255.

P1-P4 persist across reboot and power loss as soon as `SAVED` appears. A color
chosen from the wheel or recalled from a preset is persisted after it remains
unchanged in solid mode for two minutes. Rainbow and music colors are never
saved, and entering either effect cancels a pending selected-color save.
Touching the color wheel or a static preset exits either animated mode. The
brightness slider is the maximum effect brightness, and power off/on preserves
the active mode until a reboot; startup always restores in solid mode.

## Hardware mapping

| Function                                  | ESP32-S3 pin |
| ----------------------------------------- | -----------: |
| Onboard addressable LED data              |      GPIO 40 |
| LCD QSPI                                  |    GPIO 9-14 |
| LCD backlight                             |      GPIO 41 |
| Touch SDA/SCL                             |   GPIO 38/39 |
| Touch reset/interrupt                     |   GPIO 48/47 |
| ES8311 microphone I2S data                |      GPIO 16 |
| ES8311 speaker I2S data                   |      GPIO 15 |
| P2 output used for external LED data      |      GPIO 45 |
| P2 alternate GPIO signal                  |      GPIO 46 |

GPIO45 and GPIO46 are both normal input/output-capable GPIOs after reset.

Both pins are ESP32-S3 strapping pins and their logic levels are sampled during
power-on and hardware reset:

- **GPIO45** is normally associated wit VDD_SPI voltage selection. However, this
  board uses an ESP32-S3R8, for which VDD_SPI is fixed to 3.3 V by eFuse, so thhe
  GPIO45 strap does not control the flash/PSRAM supply voltage on this board.
- **GPIO46** participates in boot-mode selection together with GPIO0 and also
  affects ROM boot-message configuration. Its default state is weakly pulled low.

After reset has completed, both GPIO45 and GPIO46 are available as normal GPIOs.

GPIO45 is used here for the external addressable LED output. External circuitry should
still avoid strongly driving either strapping pin during reset. A logic-level
shifter connected to GPIO45 should present a high-impedance input to the ESP32-S3
and should not back-drive the pin while the ESP32-S3 is unpowered.

## Software prerequisites

- Arduino IDE 2.x or Arduino CLI
- Espressif ESP32 Arduino core 3.3.x
- The following libraries from `libraries`:
  - Repository-owned `AddressableLedStrip` transport using the ESP32 RMT peripheral
  - `ST77922`
  - `ST77922_TOUCH`
  - `TFT_eSPI`
  - `SimpleAwait` 1.0.1

For Arduino IDE, copy those five folders into the Arduino libraries folder,
then restart the IDE. Open:

`src/ColorController/ColorController.ino`

Required board settings:

| Setting          | Value                           |
| ---------------- | ------------------------------- |
| Board            | ESP32S3 Dev Module              |
| USB Mode         | Hardware CDC and JTAG           |
| USB CDC On Boot  | Enabled                         |
| Flash Size       | 16 MB                           |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM            | OPI PSRAM                       |
| CPU Frequency    | 240 MHz                         |

## Arduino CLI build

### Standard build

Use the original Arduino CLI command in environments where the packaged ESP32
tools are permitted to run. From the repository root in PowerShell:

```powershell
arduino-cli compile `
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc" `
  --libraries ".\libraries" `
  --output-dir ".\build\onboard-only" `
  ".\src\ColorController"
```

Run only one raw `arduino-cli compile` for this sketch at a time because
Arduino's default incremental cache is shared. Use the PowerShell wrapper below
when multiple terminals or automated tools might build concurrently.

### Windows Application Control fallback

If the standard command fails with `Failed to load Python DLL` because Windows
Application Control blocks Arduino's packaged Python executables, use:

```powershell
.\tools\build-firmware.ps1
```

The script avoids Windows Application Control failures from Arduino's packaged
Python executables and uses a repository-local build cache to avoid shared-cache
locks. It serializes concurrent builds and automatically cleans an interrupted
or incomplete build before reusing the cache. It requires the Python `esptool` package; if needed, run
`python -m pip install esptool`. Pass `-Clean` to discard the build cache. To
build for an external array, pass its pixel count, for example
`.\tools\build-firmware.ps1 -ExternalPixelCount 60`.

This builds with the external array disabled (onboard addressable LED only). To also
drive an external array on P2, add:

```text
--build-property "compiler.cpp.extra_flags=-DCOLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT=60"
```

Replace `60` with the array's pixel count. See
[Driving an external addressable LED array on P2](#driving-an-external-addressable-led-array-on-p2)
below.

To build and flash in one step over COM8:

```powershell
.\tools\build-firmware.ps1 -Port COM8
```

To flash the existing build again without recompiling, including to a second
board on another port:

```powershell
.\tools\build-firmware.ps1 -UploadOnly -Port COM8
.\tools\build-firmware.ps1 -UploadOnly -Port COM9
```

`-UploadOnly` fails if the required files are missing, so run one normal build
first. Add `-ExternalPixelCount 60` when uploading an existing external-array
build from its matching output folder.

For policy-restricted Windows systems, the script compiles first and then uses
`python -m esptool` to flash the bootloader, partition table, boot app, and
application at their standard offsets, bypassing Arduino's packaged
`flasher.exe` and `esptool.exe`. It does not overwrite the NVS partition that
stores presets and the stable selected color.

In an environment without the Python block, the equivalent standard command is:

```powershell
arduino-cli compile `
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc" `
  --libraries ".\libraries" `
  --output-dir ".\build\onboard-only" `
  --upload --port COM8 `
  ".\src\ColorController"
```

### Build output

The compile command writes persistent build artifacts to the chosen
`--output-dir` instead of leaving them only in Arduino's temporary cache.

| File                                 | Purpose                                                                                                                                    |
| ------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `ColorController.ino.merged.bin`     | Complete 16 MB image containing the bootloader, partition table, and application; flashing it at address `0x0` erases NVS and resets stored colors. |
| `ColorController.ino.bin`            | Application image only; Arduino uploads it at address `0x10000`.                                                                           |
| `ColorController.ino.bootloader.bin` | Bootloader image.                                                                                                                          |
| `ColorController.ino.partitions.bin` | Partition-table image.                                                                                                                     |

For normal development, prefer `arduino-cli compile --upload` so Arduino places
each image at the correct address. Use the merged image when a flashing tool
expects one complete binary and resetting persisted colors is acceptable.

OPI PSRAM is required for the approximately 307 KB RGB565 framebuffer and the
vendor display driver's full-frame transfer buffer. The sketch stops with a
fatal Serial message if the framebuffer cannot be allocated.

## CI/CD firmware artifact

The [Build firmware workflow](.github/workflows/build-firmware.yml) runs for
pushes to `main`, pull requests, and manual dispatches. It installs ESP32 Arduino
core 3.3.12, compiles the onboard-only firmware, verifies the merged image, and
publishes `ColorController.ino.merged.bin` as a 30-day GitHub Actions artifact
named `color-controller-merged-<commit-sha>`.

The merged binary contains the bootloader, partition table, and application and
is intended for tools that flash a complete image at address `0x0`.

## Host-side tests

From a Visual Studio Developer PowerShell:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /std:c++20 /EHsc `
  /Fo:build\color_math_tests.obj `
  /Fe:build\color_math_tests.exe `
  tests\color_math_tests.cpp
.\build\color_math_tests.exe
```

The tests cover primary and round-trip color conversion, wheel bounds,
brightness mapping, preset tap/hold/cancel behavior, mode transitions, rainbow
and breathing math, microphone-envelope smoothing, persistence timing and
cancellation, packed-color storage, and timer rollover.

## Driving an external addressable LED array on P2

The onboard addressable LED on GPIO40 is always driven. To also drive an external
array wired to P2 using GPIO45, build with the array's pixel count:

```powershell
arduino-cli compile `
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc" `
  --libraries ".\libraries" `
  --output-dir ".\build\onboard-plus-p2-60pixels" `
  --build-property "compiler.cpp.extra_flags=-DCOLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT=60" `
  ".\src\ColorController"
```

That command writes the artifacts to:

`build/onboard-plus-p2-60pixels/`

Change both the pixel-count define and output folder name when building for a
different array size. Omitting the define, or setting it to `0`, disables the
external array and drives only the onboard addressable LED.

Do not power an external array from a GPIO. Use a properly sized 5 V supply and
connect the supply ground to the ESP32-S3 ground.

For reliable WS2812-compatible LED operation, a typical installation uses:

- A 3.3 V-to-5 V logic-level shifter with a high-impedance input
- A 330-500 ohm series resistor near the first pixel
- Bulk capacitance across the array's 5 V supply
- A common ground between the array supply and the ESP32-S3

Budget up to approximately 60 mA per RGB pixel as a conservative full-white
worst-case estimate. Actual consumption depends on the addressable LED type,
brightness setting, and displayed color.

Because GPIO45 is a strapping pin, the external circuit should not drive or
back-power it while the ESP32-S3 is starting or unpowered. A conventional
unidirectional logic buffer with a high-impedance input is suitable.

Confirm the P2 connector pin order against the board schematic before making a
cable.

## On-device verification

After flashing:

1. Confirm the UI is landscape and touch coordinates align with every control.
2. Drag through red, green, and blue portions of the wheel and verify the top
   strip and onboard LED channel order.
3. Tap P1-P4 and verify the selected color changes once.
4. Hold P1-P4 until `SAVED`, select another color, then tap the saved preset.
   Reboot and confirm the stored preset remains.
5. Sweep brightness to both endpoints.
6. Turn output off, change color and brightness, then turn it on and verify the
   latest settings are restored.
7. Tap P5 and verify a full hue cycle with a smooth, slow breathing effect.
   With an external array, verify the rainbow is distributed across the strip.
8. Tap the musical-note P6 button, play music near the onboard microphone, and
   verify brightness responds quickly to transients and decays smoothly.
9. While each effect is active, adjust brightness, toggle power, and touch the
   wheel to verify the documented mode transitions.
10. Drag out of a pressed preset before release and verify it is not recalled or
   overwritten.
11. With the external addressable LED array connected, power-cycle and reset the board
   several times and confirm reliable booting.
12. Select a solid color, wait at least two minutes without changing it, reboot,
    and confirm it is restored.
13. Select a solid color, enter rainbow or music before two minutes elapse,
    reboot, and confirm the effect-generated color was not stored.

Microphone sensitivity and the perceived breathing speed require final tuning
on the physical board. P6 does not show a status indicator while its microphone
is initializing. If initialization definitively fails, Serial reports a warning
and P6 shows a red fault dot; the dot never indicates recording. Music mode
continues as a dim fallback color cycle without affecting the other controls.

## Design notes

The UI uses a 16-bit TFT_eSPI sprite backed by the PSRAM-enabled ESP32 allocator
and sends that buffer through Elecrow's ST77922 QSPI driver.

Touch processing uses a non-blocking state machine, including long-press
detection. Fixed-memory SimpleAwait C++20 tasks schedule touch polling, effect
frames, effect UI refreshes, audio initialization/feedback, and delayed NVS
writes. Arduino `loop()` contains only `simpleawait::poll()`; application waits
use coroutine delay primitives rather than blocking delay calls. The vendored
ESP32 scheduler yields one RTOS tick when no coroutine is ready so driver and
idle work are not starved by the poll-only Arduino loop.

The initial framebuffer is transferred before touch-controller initialization.
Touch startup bounds stale-status retries and reports I2C/status failure rather
than leaving the panel on its uninitialized pixel pattern indefinitely.
Display SPI and touch I2C are initialized explicitly from `setup()`; their
global C++ constructors never access hardware, which keeps cold power-on
behavior consistent with warm resets. The display backlight remains off until
the first complete framebuffer transfer, so uninitialized panel memory is never
shown during cold-start NVS and color-wheel setup.

### Classes and components

| Component               | Responsibility                                                                                                           |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| `ControllerModel`       | Owns the selected color, brightness, power state, active output mode, and four presets.                                   |
| `InteractionController` | Routes touch gestures into model, lighting, persistence, audio, and UI actions.                                           |
| `AddressableLedStrip`   | Owns the minimal WS2812-compatible GRB encoder, frame buffer, and ESP32 RMT transport.                                  |
| `LightingOutput`        | Owns addressable LED outputs, reactive effect state, microphone envelope, and live preview color.                       |
| `UiRenderer`            | Owns framebuffer rendering, wheel-marker repair, transient saved feedback, and panel transfers.                          |
| `UiLayout`              | Defines named screen geometry, hit regions, glyph dimensions, and RGB565 theme values.                                   |
| `ColorPersistenceService` | Coordinates delayed selected-color writes, queued preset writes, and retry timing.                                     |
| `PersistentState`       | Restores and deduplicates Preferences/NVS writes for presets and the stable selected color.                               |
| `ManualColorSaveTracker` | Tracks manual color changes, cancellation, rollover-safe elapsed time, and save readiness.                                |
| `SimpleAwait`           | Runs fixed-memory cooperative tasks for touch, effects, UI refresh, audio sequencing, and persistence.                   |
| `PresetGesture`         | Distinguishes a preset tap from a 700 ms hold and guarantees that a stored preset is not also recalled on release.       |
| `RgbColor` / `HsvColor` | Small color value types shared by the model, renderer, tests, and addressable LED adapter.                              |
| `ColorMath` functions   | Convert RGB/HSV values, map wheel coordinates to color, and map slider coordinates to brightness.                        |
| `ReactiveLighting`      | Provides host-tested rainbow, breathing, music-color, brightness-scaling, and adaptive audio-envelope math.              |
| `AudioFeedback`         | Asynchronously initializes duplex ES8311 audio, queues touch beeps, and reads non-blocking microphone amplitude samples. |
| `ColorController.ino`   | Composes services, initializes hardware, starts SimpleAwait tasks, and provides the poll-only Arduino loop.               |

The code comments use Doxygen-style summaries for reusable types and public
methods. Straightforward drawing calls are left uncluttered; comments focus on
hardware constraints, timing behavior, and non-obvious performance choices.

The detailed functional and technical plan is in:

`specs/feature-touch-color-controller/specs-touch-color-controller-funct-and-tech.md`
