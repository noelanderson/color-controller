#pragma once

#include "driver/i2c_master.h"

/**
 * Duplex audio service for button feedback and onboard microphone sampling.
 */
namespace AudioFeedback {

/** Shares the touch controller's I2C bus; returns false if the codec or I2S
 * output could not be initialized (feedback is then silently skipped). */
bool begin(i2c_master_bus_handle_t touchBus);

bool microphoneAvailable();

/** Returns one non-blocking block-amplitude sample when microphone data is ready. */
bool readMicrophoneLevel(uint16_t& magnitude);

void beep();
void beepLong();

}  // namespace AudioFeedback
