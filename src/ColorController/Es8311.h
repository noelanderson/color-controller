#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

/**
 * Minimal ES8311 codec driver ported from Espressif's vendor example to use
 * the new i2c_master_bus API, so it can share a bus already owned by another
 * device (this board's touch controller) instead of claiming its own port.
 *
 * Trimmed to tone-out playback at a single fixed operating point: 16 kHz
 * sample rate with a 384x (6.144 MHz) MCLK, 16-bit stereo I2S frames.
 */

typedef void* es8311_handle_t;

/** Adds the codec as a device on an existing I2C bus; returns null on failure. */
es8311_handle_t es8311_create(i2c_master_bus_handle_t bus, uint8_t device_address);

void es8311_delete(es8311_handle_t dev);

/** Resets the codec and configures clocks/format for the fixed operating point. */
esp_err_t es8311_init(es8311_handle_t dev);

esp_err_t es8311_voice_volume_set(es8311_handle_t dev, int volume, int* volume_set);

esp_err_t es8311_voice_mute(es8311_handle_t dev, bool mute);
