# Agent Instructions

## Goal

Make small, reliable changes to the touch color controller firmware. Preserve
the existing hardware behavior unless the task explicitly requires a change.

## Repository Map

- `src/ColorController/`: project firmware and hardware configuration.
- `tests/`: host-side tests for hardware-independent logic.
- `libraries/`: vendored third-party libraries. Do not modify these unless the
  task specifically targets a library.
- `specs/`: feature and technical requirements.
- `README.md`: supported setup, build, upload, and wiring instructions.

## Working Rules

1. Read the relevant source, tests, and specification before editing.
2. Prefer focused changes; do not refactor unrelated code.
3. Keep reusable logic independent of Arduino hardware when practical so it can
   be covered by host-side tests.
4. Follow the existing C++ style, naming, types, and file organization.
5. Treat pin assignments, board settings, PSRAM requirements, and strapping-pin
   behavior as hardware constraints. Do not change them without documenting the
   reason and impact.
6. Update tests and user-facing documentation when behavior or setup changes.
7. Do not commit generated files from `build/` or local executables.

## Validation

Run the smallest relevant checks after a change.

Host-side tests, from a Visual Studio Developer PowerShell:

```powershell
cl /std:c++17 /EHsc tests\color_math_tests.cpp
.\color_math_tests.exe
```

Firmware build:

```powershell
arduino-cli compile `
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc" `
  --libraries ".\libraries" `
  --output-dir ".\build\onboard-only" `
  ".\src\ColorController"
```

Do not upload firmware unless the user explicitly requests it and provides the
target port.

## Completion

Before finishing:

- Confirm the requested behavior is implemented.
- Report which checks ran and their results.
- Call out checks that could not run, especially those requiring Arduino CLI,
  the ESP32 core, Visual Studio build tools, or physical hardware.
- Mention hardware assumptions and any manual verification still required.
