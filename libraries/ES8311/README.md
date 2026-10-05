# ES8311

A focused ES8311 codec driver for Arduino-ESP32 applications that already own
an ESP-IDF `i2c_master_bus_handle_t`.

This library is intentionally not a general codec framework. It configures the
operating point used by the Touch Color Controller:

- 16 kHz sample rate
- 6.144 MHz MCLK (384x)
- 16-bit stereo I2S frames
- Codec slave mode
- Analog microphone input
- DAC volume and mute control

## Why the driver accepts an existing bus

Many display boards place touch and audio devices on one I2C controller. The
library adds only an ES8311 device handle and never creates, reconfigures, or
deletes the parent bus. This lets it coexist with the bus owner.

## Initialization

```cpp
#include <Es8311.h>

es8311_handle_t codec = es8311_create(i2cBus, 0x18);
if (codec == nullptr) {
  // Report initialization failure.
}

if (es8311_init_begin(codec) != ESP_OK) {
  // Report reset failure.
}

// Wait at least 20 ms using the application's scheduler.

if (es8311_init_finish(codec) != ESP_OK ||
    es8311_microphone_config(codec) != ESP_OK) {
  // Report configuration failure.
}
```

The reset sequence is deliberately split into `es8311_init_begin()` and
`es8311_init_finish()`. The ES8311 requires at least 20 ms between them, and the
library does not hide that hardware wait in a blocking `delay()`. Use a
cooperative timer, RTOS delay, or startup mechanism appropriate to the host
application.

Call `es8311_delete()` before deleting the parent I2C bus. The function removes
the codec device and releases the handle but does not alter the bus.

## Error handling

Configuration functions return `esp_err_t`. A null device handle is reported as
`ESP_ERR_INVALID_ARG`. Register-access failures are returned as `ESP_FAIL`; use
application-level diagnostics to identify the failed initialization stage.

## Scope

The public API does not expose arbitrary register access, alternate sample
rates, master mode, or format negotiation. Add those capabilities only with
validated clock coefficients and hardware tests.

The register subset and fixed configuration are derived from Espressif's
Apache-2.0-licensed ES8311 driver and vendor example.
