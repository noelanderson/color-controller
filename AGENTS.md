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
5. Favor readability over rigid line-length limits. Around 110 characters is
   usually comfortable, but longer lines are welcome when keeping a complete
   expression, signature, command, or configuration value together is clearer.
   Wrap at logical boundaries, not merely because a line crossed a threshold.
6. Comment hardware protocols, resource ownership, state transitions, timing
   choices, and non-obvious algorithms generously enough that a maintainer can
   understand why the code is shaped that way. Do not narrate obvious syntax.
7. Treat pin assignments, board settings, PSRAM requirements, and strapping-pin
   behavior as hardware constraints. Do not change them without documenting the
   reason and impact.
8. Update tests and user-facing documentation when behavior or setup changes.
9. Keep Arduino `loop()` limited to `simpleawait::poll()`. Put touch handling,
   effects, UI refreshes, persistence, audio sequencing, and other runtime work
   in fixed-memory SimpleAwait tasks. Steady-state task loops must suspend with
   a positive-duration wait; do not use `simpleawait::yield()`,
   `delay_ms(0)`, or another permanently-ready loop that defeats scheduler idle.
10. Do not add `delay()`, `vTaskDelay()`, sleeps, or hand-written busy waits to
   application runtime code. Express waits with
   `co_await simpleawait::delay_ms()` or another SimpleAwait primitive. The
   ST77922 touch driver's two startup-only reset delays are hardware-mandated
   vendor timing and must remain. The vendored SimpleAwait ESP32 idle hook is
   the single sanctioned runtime `vTaskDelay()`; it is required for touch
   responsiveness and must remain when updating the library.
11. Keep hardware initialization in `setup()`; spawn runtime tasks only after
   the state and hardware they use are ready. Check and report every task
   creation failure.
12. Never place an unbounded hardware-status polling loop in startup. Bound
    retries, surface failure, and render a usable diagnostic UI before optional
    peripherals can block initialization.
13. Global constructors must initialize data only. Do not access GPIO, SPI,
    I2C, display, touch, audio, storage, or other Arduino/ESP-IDF services until
    `setup()` runs.
14. Any translation unit that declares or uses SimpleAwait tasks must include
    `AwaitConfig.h`, never `<SimpleAwait.h>` directly, so every translation unit
    uses the same scheduler capacity, frame pool, and error hook.
15. Do not commit generated files from `build/` or local executables.

## Validation

Run the smallest relevant checks after a change.

Host-side tests, from a Visual Studio Developer PowerShell:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /std:c++20 /EHsc `
  /Fo:build\color_math_tests.obj `
  /Fe:build\color_math_tests.exe `
  tests\color_math_tests.cpp
.\build\color_math_tests.exe
```

Firmware build:

```powershell
.\tools\build-firmware.ps1
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
