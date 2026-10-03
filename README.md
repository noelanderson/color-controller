# Touch Color Controller

Touch-driven color and brightness control for the Elecrow 3.5-inch ESP32-S3
320x480 capacitive display (DLE06235B).

The firmware drives the board's onboard NeoPixel and, optionally, an external
NeoPixel array wired to P2 at the same time with the same color and brightness.

Ultimate goal is for this to be a wall panel control for led strip lights

## User interface

The display runs in 480x320 landscape orientation.

- **Top strip:** previews the selected color.
- **Color wheel:** touch or drag in the left half to choose hue and saturation.
- **Presets:** tap P1-P6 to recall a color. Hold a preset for at least 700 ms to
  replace it with the currently selected color. `SAVED` confirms the long press.
- **ON/OFF:** disables LED output without forgetting the selected color or brightness.
- **Brightness:** drag the bottom slider from 0 through 255.

Presets are currently held in RAM and return to their defaults after reset.

## Hardware mapping

| Function                                  | ESP32-S3 pin |
| ----------------------------------------- | -----------: |
| Onboard NeoPixel data                     |      GPIO 40 |
| LCD QSPI                                  |    GPIO 9-14 |
| LCD backlight                             |      GPIO 41 |
| Touch SDA/SCL                             |   GPIO 38/39 |
| Touch reset/interrupt                     |   GPIO 48/47 |
| P2 output used for external NeoPixel data |      GPIO 45 |
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

GPIO45 is used here for the external NeoPixel output. External circuitry should
still avoid strongly driving either strapping pin during reset. A logic-level
shifter connected to GPIO45 should present a high-impedance input to the ESP32-S3
and should not back-drive the pin while the ESP32-S3 is unpowered.

## Software prerequisites

- Arduino IDE 2.x or Arduino CLI
- Espressif ESP32 Arduino core 3.3.x
- The following libraries from `libraries`:
  - `Adafruit_NeoPixel`
  - `ST77922`
  - `ST77922_TOUCH`
  - `TFT_eSPI`

For Arduino IDE, copy those four folders into the Arduino libraries folder,
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

From the repository root in PowerShell:

```powershell
arduino-cli compile `
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc" `
  --libraries ".\libraries" `
  --output-dir ".\build\onboard-only" `
  ".\src\ColorController"
```

This builds with the external array disabled (onboard NeoPixel only). To also
drive an external array on P2, add:

```text
--build-property "compiler.cpp.extra_flags=-DCOLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT=60"
```

Replace `60` with the array's pixel count. See
[Driving an external NeoPixel array on P2](#driving-an-external-neopixel-array-on-p2)
below.

To upload, add `--upload --port COMx` and replace `COMx` with the board's port.

For example, to build and flash in one step over COM8:

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
| `ColorController.ino.merged.bin`     | Complete 16 MB image containing the bootloader, partition table, and application; use this for flashing a complete image at address `0x0`. |
| `ColorController.ino.bin`            | Application image only; Arduino uploads it at address `0x10000`.                                                                           |
| `ColorController.ino.bootloader.bin` | Bootloader image.                                                                                                                          |
| `ColorController.ino.partitions.bin` | Partition-table image.                                                                                                                     |

For normal development, prefer `arduino-cli compile --upload` so Arduino places
each image at the correct address. Use the merged image when a flashing tool
expects one complete binary.

OPI PSRAM is required for the approximately 307 KB RGB565 framebuffer and the
vendor display driver's full-frame transfer buffer. The sketch stops with a
fatal Serial message if the framebuffer cannot be allocated.

## Host-side tests

From a Visual Studio Developer PowerShell:

```powershell
cl /std:c++17 /EHsc tests\color_math_tests.cpp
.\color_math_tests.exe
```

The tests cover primary and round-trip color conversion, wheel bounds,
brightness mapping, preset tap/hold/cancel behavior, and timer rollover.

## Driving an external NeoPixel array on P2

The onboard NeoPixel on GPIO40 is always driven. To also drive an external
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
external array and drives only the onboard NeoPixel.

Do not power an external array from a GPIO. Use a properly sized 5 V supply and
connect the supply ground to the ESP32-S3 ground.

For reliable NeoPixel operation, a typical installation uses:

- A 3.3 V-to-5 V logic-level shifter with a high-impedance input
- A 330-500 ohm series resistor near the first pixel
- Bulk capacitance across the array's 5 V supply
- A common ground between the array supply and the ESP32-S3

Budget up to approximately 60 mA per RGB pixel as a conservative full-white
worst-case estimate. Actual consumption depends on the NeoPixel type,
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
3. Tap each preset and verify the selected color changes once.
4. Hold a preset until `SAVED`, select another color, then tap the saved preset.
5. Sweep brightness to both endpoints.
6. Turn output off, change color and brightness, then turn it on and verify the
   latest settings are restored.
7. Drag out of a pressed preset before release and verify it is not recalled or
   overwritten.
8. With the external NeoPixel array connected, power-cycle and reset the board
   several times and confirm reliable booting.

## Design notes

The UI uses a 16-bit TFT_eSPI sprite backed by the PSRAM-enabled ESP32 allocator
and sends that buffer through Elecrow's ST77922 QSPI driver.

Touch processing uses a non-blocking state machine, including long-press
detection.

### Classes and components

| Component               | Responsibility                                                                                                           |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| `ControllerModel`       | Owns the selected color, brightness, power state, and six RAM-only presets without depending on display or LED hardware. |
| `PresetGesture`         | Distinguishes a preset tap from a 700 ms hold and guarantees that a stored preset is not also recalled on release.       |
| `RgbColor` / `HsvColor` | Small color value types shared by the model, renderer, tests, and NeoPixel adapter.                                      |
| `ColorMath` functions   | Convert RGB/HSV values, map wheel coordinates to color, and map slider coordinates to brightness.                        |
| `TouchState`            | Records which control captured the active touch plus its latest coordinates until release debounce completes.            |
| `ColorController.ino`   | Composes the hardware drivers, model, renderer, touch routing, and Arduino `setup()`/`loop()` lifecycle.                 |

The code comments use Doxygen-style summaries for reusable types and public
methods. Straightforward drawing calls are left uncluttered; comments focus on
hardware constraints, timing behavior, and non-obvious performance choices.

The detailed functional and technical plan is in:

`specs/feature-touch-color-controller/specs-touch-color-controller-funct-and-tech.md`
