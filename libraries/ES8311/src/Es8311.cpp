#include "Es8311.h"

#include <new>

#include "Es8311Reg.h"

namespace {

constexpr uint32_t kI2cTimeoutMs = 1000;
constexpr uint32_t kI2cClockHz = 100000;
constexpr int kMinimumVolume = 0;
constexpr int kMaximumVolume = 100;
constexpr int kVolumeRegisterRange = 256;
constexpr uint8_t kMuteMask = (1 << 6) | (1 << 5);

struct Es8311Dev {
  i2c_master_dev_handle_t handle;
};

esp_err_t writeReg(es8311_handle_t dev, uint8_t reg, uint8_t value) {
  if (dev == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  auto* es = static_cast<Es8311Dev*>(dev);
  const uint8_t buffer[2] = {reg, value};
  return i2c_master_transmit(es->handle, buffer, sizeof(buffer), kI2cTimeoutMs);
}

esp_err_t readReg(es8311_handle_t dev, uint8_t reg, uint8_t* value) {
  if (dev == nullptr || value == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  auto* es = static_cast<Es8311Dev*>(dev);
  return i2c_master_transmit_receive(es->handle, &reg, 1, value, 1, kI2cTimeoutMs);
}

// Fixed operating point: 16 kHz sample rate, 384x MCLK (6.144 MHz). Register
// values mirror the vendor coefficient table row for this exact (mclk, rate).
esp_err_t configureClocks(es8311_handle_t dev) {
  if (writeReg(dev, ES8311_CLK_MANAGER_REG01, 0x3F) != ESP_OK) {
    return ESP_FAIL;
  }

  uint8_t regv;
  if (readReg(dev, ES8311_CLK_MANAGER_REG06, &regv) != ESP_OK) {
    return ESP_FAIL;
  }
  regv &= ~(1 << 5);  // SCLK not inverted
  if (writeReg(dev, ES8311_CLK_MANAGER_REG06, regv) != ESP_OK) {
    return ESP_FAIL;
  }

  if (readReg(dev, ES8311_CLK_MANAGER_REG02, &regv) != ESP_OK) {
    return ESP_FAIL;
  }
  regv = (regv & 0x07) | (2 << 5) | (1 << 3);  // pre_div=3, pre_multi=1
  if (writeReg(dev, ES8311_CLK_MANAGER_REG02, regv) != ESP_OK) {
    return ESP_FAIL;
  }

  if (writeReg(dev, ES8311_CLK_MANAGER_REG03, 0x10) != ESP_OK) {  // adc_osr
    return ESP_FAIL;
  }
  if (writeReg(dev, ES8311_CLK_MANAGER_REG04, 0x10) != ESP_OK) {  // dac_osr
    return ESP_FAIL;
  }
  if (writeReg(dev, ES8311_CLK_MANAGER_REG05, 0x00) != ESP_OK) {  // adc/dac div=1
    return ESP_FAIL;
  }

  if (readReg(dev, ES8311_CLK_MANAGER_REG06, &regv) != ESP_OK) {
    return ESP_FAIL;
  }
  regv = (regv & 0xE0) | 0x03;  // bclk_div=4
  if (writeReg(dev, ES8311_CLK_MANAGER_REG06, regv) != ESP_OK) {
    return ESP_FAIL;
  }

  if (readReg(dev, ES8311_CLK_MANAGER_REG07, &regv) != ESP_OK) {
    return ESP_FAIL;
  }
  regv &= 0xC0;  // lrck_h=0
  if (writeReg(dev, ES8311_CLK_MANAGER_REG07, regv) != ESP_OK) {
    return ESP_FAIL;
  }

  return writeReg(dev, ES8311_CLK_MANAGER_REG08, 0xFF);  // lrck_l
}

esp_err_t configureFormat(es8311_handle_t dev) {
  uint8_t reg00;
  if (readReg(dev, ES8311_RESET_REG00, &reg00) != ESP_OK) {
    return ESP_FAIL;
  }
  reg00 &= 0xBF;  // slave mode
  if (writeReg(dev, ES8311_RESET_REG00, reg00) != ESP_OK) {
    return ESP_FAIL;
  }

  const uint8_t resolution16 = 3 << 2;
  if (writeReg(dev, ES8311_SDPIN_REG09, resolution16) != ESP_OK) {
    return ESP_FAIL;
  }
  return writeReg(dev, ES8311_SDPOUT_REG0A, resolution16);
}

}  // namespace

es8311_handle_t es8311_create(i2c_master_bus_handle_t bus, uint8_t device_address) {
  if (bus == nullptr) {
    return nullptr;
  }
  auto* es = new (std::nothrow) Es8311Dev();
  if (es == nullptr) {
    return nullptr;
  }
  const i2c_device_config_t devConfig = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = device_address,
      .scl_speed_hz = kI2cClockHz,
  };
  if (i2c_master_bus_add_device(bus, &devConfig, &es->handle) != ESP_OK) {
    delete es;
    return nullptr;
  }
  return es;
}

void es8311_delete(es8311_handle_t dev) {
  if (dev == nullptr) {
    return;
  }
  auto* es = static_cast<Es8311Dev*>(dev);
  i2c_master_bus_rm_device(es->handle);
  delete es;
}

esp_err_t es8311_init_begin(es8311_handle_t dev) { return writeReg(dev, ES8311_RESET_REG00, 0x1F); }

esp_err_t es8311_init_finish(es8311_handle_t dev) {
  if (writeReg(dev, ES8311_RESET_REG00, 0x00) != ESP_OK) {
    return ESP_FAIL;
  }
  if (writeReg(dev, ES8311_RESET_REG00, 0x80) != ESP_OK) {  // power-on
    return ESP_FAIL;
  }

  if (configureClocks(dev) != ESP_OK) {
    return ESP_FAIL;
  }
  if (configureFormat(dev) != ESP_OK) {
    return ESP_FAIL;
  }

  if (writeReg(dev, ES8311_SYSTEM_REG0D, 0x01) != ESP_OK) {  // power up analog
    return ESP_FAIL;
  }
  if (writeReg(dev, ES8311_SYSTEM_REG0E, 0x02) != ESP_OK) {  // enable PGA/ADC modulator
    return ESP_FAIL;
  }
  if (writeReg(dev, ES8311_SYSTEM_REG12, 0x00) != ESP_OK) {  // power up DAC
    return ESP_FAIL;
  }
  if (writeReg(dev, ES8311_SYSTEM_REG13, 0x10) != ESP_OK) {  // enable output drive
    return ESP_FAIL;
  }
  if (writeReg(dev, ES8311_ADC_REG1C, 0x6A) != ESP_OK) {  // bypass ADC equalizer
    return ESP_FAIL;
  }
  return writeReg(dev, ES8311_DAC_REG37, 0x08);  // bypass DAC equalizer
}

esp_err_t es8311_microphone_config(es8311_handle_t dev) {
  if (writeReg(dev, ES8311_ADC_REG17, 0xC8) != ESP_OK) {
    return ESP_FAIL;
  }
  return writeReg(dev, ES8311_SYSTEM_REG14, 0x1A);
}

esp_err_t es8311_voice_volume_set(es8311_handle_t dev, int volume, int* volume_set) {
  if (volume < kMinimumVolume) {
    volume = kMinimumVolume;
  } else if (volume > kMaximumVolume) {
    volume = kMaximumVolume;
  }
  const int reg32 = volume == 0 ? 0 : ((volume * kVolumeRegisterRange / kMaximumVolume) - 1);
  if (volume_set != nullptr) {
    *volume_set = volume;
  }
  return writeReg(dev, ES8311_DAC_REG32, static_cast<uint8_t>(reg32));
}

esp_err_t es8311_voice_mute(es8311_handle_t dev, bool mute) {
  uint8_t reg31;
  if (readReg(dev, ES8311_DAC_REG31, &reg31) != ESP_OK) {
    return ESP_FAIL;
  }
  if (mute) {
    reg31 |= kMuteMask;
  } else {
    reg31 &= ~kMuteMask;
  }
  return writeReg(dev, ES8311_DAC_REG31, reg31);
}
