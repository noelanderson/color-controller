# AddressableLedStrip

A small Arduino-ESP32 transport for WS2812-compatible addressable LEDs. It uses
the ESP32 RMT peripheral, emits GRB data at 10 MHz RMT resolution, and appends a
300 microsecond low latch symbol to every frame.

## Requirements

- Arduino-ESP32 3.x
- An ESP32 target with the Arduino RMT API
- A suitable external 5 V supply and logic-level shifter for 5 V strips

## Basic use

```cpp
#include <AddressableLedStrip.h>

AddressableLedStrip pixels(45, 60);

void setup() {
  Serial.begin(115200);
  if (!pixels.begin()) {
    Serial.println("LED transport initialization failed");
    return;
  }

  for (uint16_t index = 0; index < pixels.size(); ++index) {
    pixels.setPixel(index, 255, 0, 0, 128);
  }
  pixels.show();
}

void loop() {}
```

`setPixel()` accepts red, green, blue, and brightness values from 0 to 255. It
updates the encoded frame but does not transmit; call `show()` after updating
the desired pixels.

## Resource and timing behavior

- Construction is hardware-safe.
- `begin()` allocates the complete encoded frame and claims RMT.
- Repeated successful `begin()` calls are no-ops.
- A zero-length strip is valid, allocates nothing, and claims no RMT channel.
- The destructor releases RMT and the encoded frame.
- Copying is disabled because the object exclusively owns those resources.
- `show()` waits for the RMT frame to finish; it does not insert a software
  delay for the latch interval.

The transport uses WS2812-compatible timing:

| Bit | High | Low |
|---|---:|---:|
| 0 | 400 ns | 800 ns |
| 1 | 800 ns | 400 ns |

## Electrical notes

Do not power an LED strip from a GPIO. Use a supply sized for the strip, connect
the controller and strip grounds, and use an appropriate unidirectional
3.3 V-to-5 V level shifter. A 330-500 ohm data resistor near the first pixel and
bulk capacitance across the strip supply are recommended.
