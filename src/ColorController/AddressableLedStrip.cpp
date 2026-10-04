#include "AddressableLedStrip.h"

#include <new>

namespace {

uint8_t applyBrightness(uint8_t value, uint8_t brightness) {
  if (brightness == UINT8_MAX) {
    return value;
  }
  // Match the conventional 8-bit fixed-point LED brightness scale, including
  // exact passthrough at 255 and a fully dark result at zero.
  return static_cast<uint8_t>((static_cast<uint16_t>(value) * (brightness + 1U)) >> 8);
}

}  // namespace

AddressableLedStrip::AddressableLedStrip(uint8_t pin, uint16_t pixelCount)
    : pin_(pin), pixelCount_(pixelCount) {}

AddressableLedStrip::~AddressableLedStrip() {
  if (rmtInitialized_) {
    rmtDeinit(pin_);
  }
  delete[] symbols_;
}

bool AddressableLedStrip::begin() {
  if (pixelCount_ == 0) {
    ready_ = true;
    return true;
  }

  // Keep the data line low in hardware after every frame. This preserves the
  // 300 us latch guard without adding an application-level blocking delay.
  symbolCount_ = static_cast<size_t>(pixelCount_) * kBitsPerPixel + 1;
  symbols_ = new (std::nothrow) rmt_data_t[symbolCount_];
  if (symbols_ == nullptr) {
    return false;
  }

  if (!rmtInit(pin_, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, kRmtFrequencyHz)) {
    delete[] symbols_;
    symbols_ = nullptr;
    return false;
  }

  rmtInitialized_ = true;
  ready_ = true;
  clear();
  return true;
}

uint16_t AddressableLedStrip::size() const { return pixelCount_; }

void AddressableLedStrip::clear() {
  if (!ready_ || symbols_ == nullptr) {
    return;
  }
  for (size_t index = 0; index < static_cast<size_t>(pixelCount_) * kBitsPerPixel; ++index) {
    symbols_[index].level0 = 1;
    symbols_[index].duration0 = kZeroHighTicks;
    symbols_[index].level1 = 0;
    symbols_[index].duration1 = kZeroLowTicks;
  }
  symbols_[symbolCount_ - 1].level0 = 0;
  symbols_[symbolCount_ - 1].duration0 = kLatchLowTicks;
  symbols_[symbolCount_ - 1].level1 = 0;
  symbols_[symbolCount_ - 1].duration1 = 1;
}

void AddressableLedStrip::setPixel(uint16_t index, const RgbColor& color, uint8_t brightness) {
  if (!ready_ || symbols_ == nullptr || index >= pixelCount_) {
    return;
  }

  size_t symbolIndex = static_cast<size_t>(index) * kBitsPerPixel;
  encodeByte(symbolIndex, applyBrightness(color.green, brightness));
  encodeByte(symbolIndex, applyBrightness(color.red, brightness));
  encodeByte(symbolIndex, applyBrightness(color.blue, brightness));
}

bool AddressableLedStrip::show() {
  if (!ready_) {
    return false;
  }
  if (pixelCount_ == 0) {
    return true;
  }
  const bool succeeded = rmtWrite(pin_, symbols_, symbolCount_, RMT_WAIT_FOR_EVER);
  if (!succeeded && !transmissionErrorReported_) {
    Serial.printf("Addressable LED frame transmission failed on GPIO %u.\n", pin_);
  }
  transmissionErrorReported_ = !succeeded;
  return succeeded;
}

void AddressableLedStrip::encodeByte(size_t& symbolIndex, uint8_t value) {
  // WS2812-compatible strips consume each color byte most-significant bit first.
  for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
    const bool highBit = (value & mask) != 0;
    symbols_[symbolIndex].level0 = 1;
    symbols_[symbolIndex].duration0 = highBit ? kOneHighTicks : kZeroHighTicks;
    symbols_[symbolIndex].level1 = 0;
    symbols_[symbolIndex].duration1 = highBit ? kOneLowTicks : kZeroLowTicks;
    ++symbolIndex;
  }
}
