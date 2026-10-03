#pragma once

#include "driver/i2c_master.h"

/**
 * Short beep feedback through the onboard ES8311 codec and speaker amp: a
 * single beep for a tap, a double beep for a completed long-press.
 */
namespace AudioFeedback {

/** Shares the touch controller's I2C bus; returns false if the codec or I2S
 * output could not be initialized (feedback is then silently skipped). */
bool begin(i2c_master_bus_handle_t touchBus);

void beep();
void beepLong();

}  // namespace AudioFeedback
