#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

/**
 * Opaque ES8311 device handle.
 *
 * The handle owns only the device attached to the supplied I2C bus. The caller
 * retains ownership of the bus itself.
 */
typedef void* es8311_handle_t;

/** Adds the codec as a device on an existing I2C bus; returns null on failure. */
es8311_handle_t es8311_create(i2c_master_bus_handle_t bus, uint8_t device_address);

/** Removes the I2C device and releases its handle; a null handle is accepted. */
void es8311_delete(es8311_handle_t dev);

/**
 * Starts the codec reset sequence.
 *
 * The caller must wait at least 20 ms before invoking es8311_init_finish().
 * Splitting initialization lets cooperative applications express that hardware
 * wait without a blocking delay inside the driver.
 */
esp_err_t es8311_init_begin(es8311_handle_t dev);

/** Completes reset and configures 16 kHz, 384x MCLK, 16-bit stereo I2S. */
esp_err_t es8311_init_finish(es8311_handle_t dev);

/** Enables the board's analog microphone input and applies the vendor gain. */
esp_err_t es8311_microphone_config(es8311_handle_t dev);

/** Sets DAC output volume on a clamped 0-100 scale. */
esp_err_t es8311_voice_volume_set(es8311_handle_t dev, int volume, int* volume_set);

/** Mutes or unmutes the DAC output. */
esp_err_t es8311_voice_mute(es8311_handle_t dev, bool mute);
