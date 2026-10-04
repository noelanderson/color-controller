#pragma once

#include <Arduino.h>

#include "ColorMath.h"

/**
 * Minimal WS2812-compatible addressable LED transport using the ESP32 RMT peripheral.
 *
 * Construction is hardware-safe. begin() allocates the fixed pixel buffer and claims an
 * RMT channel for the configured pin.
 */
class AddressableLedStrip {
 public:
  AddressableLedStrip(uint8_t pin, uint16_t pixelCount);
  ~AddressableLedStrip();

  AddressableLedStrip(const AddressableLedStrip&) = delete;
  AddressableLedStrip& operator=(const AddressableLedStrip&) = delete;

  bool begin();
  uint16_t size() const;
  void clear();
  void setPixel(uint16_t index, const RgbColor& color, uint8_t brightness);
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
