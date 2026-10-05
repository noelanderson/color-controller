#pragma once

#include <Arduino.h>

/**
 * Minimal WS2812-compatible addressable LED transport using the ESP32 RMT peripheral.
 *
 * Construction does not access hardware. begin() allocates one fixed frame buffer
 * and claims the RMT transmitter for the configured pin. Pixel bytes are encoded
 * in the GRB wire order expected by WS2812-compatible devices.
 */
class AddressableLedStrip {
 public:
  /**
   * Creates a transport for a strip without touching the pin or RMT peripheral.
   *
   * A zero pixel count is valid and produces a no-op transport that owns no RMT
   * resources. The pin is therefore ignored for zero-length strips.
   */
  AddressableLedStrip(uint8_t pin, uint16_t pixelCount);
  ~AddressableLedStrip();

  AddressableLedStrip(const AddressableLedStrip&) = delete;
  AddressableLedStrip& operator=(const AddressableLedStrip&) = delete;

  /** Allocates the encoded frame and claims the RMT channel. Safe to call repeatedly. */
  bool begin();
  /** Returns the configured number of pixels. */
  uint16_t size() const;
  /** Sets every encoded color bit to zero without transmitting the frame. */
  void clear();
  /**
   * Updates one pixel in the encoded frame.
   *
   * Out-of-range writes and writes before begin() are ignored. brightness uses
   * a linear 0-255 scale and is applied independently to each supplied channel.
   */
  void setPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
  /** Transmits the current frame and reports whether the RMT write succeeded. */
  bool show();

 private:
  static constexpr size_t kBitsPerByte = 8;
  static constexpr size_t kBytesPerPixel = 3;
  static constexpr size_t kBitsPerPixel = kBitsPerByte * kBytesPerPixel;
  // At 10 MHz each RMT tick is 100 ns. These pulse widths implement the
  // tolerant center of the WS2812-compatible zero/one timing windows.
  static constexpr uint32_t kRmtFrequencyHz = 10000000;
  static constexpr uint16_t kZeroHighTicks = 4;
  static constexpr uint16_t kZeroLowTicks = 8;
  static constexpr uint16_t kOneHighTicks = 8;
  static constexpr uint16_t kOneLowTicks = 4;
  static constexpr uint16_t kLatchLowTicks = 3000;

  void encodeByte(size_t& symbolIndex, uint8_t value);

  uint8_t pin_;
  uint16_t pixelCount_;
  size_t symbolCount_ = 0;
  rmt_data_t* symbols_ = nullptr;
  // A zero-length strip is logically ready but never owns an RMT channel.
  bool ready_ = false;
  bool rmtInitialized_ = false;
  bool transmissionErrorReported_ = false;
};
