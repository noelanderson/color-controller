#pragma once

#include "driver/i2c_master.h"

/**
 * Duplex audio service for button feedback and onboard microphone sampling.
 */
namespace AudioFeedback {

/** Starts asynchronous codec initialization and feedback processing. */
bool start(i2c_master_bus_handle_t touchBus);

bool microphoneAvailable();

/** Returns one non-blocking block-amplitude sample when microphone data is ready. */
bool readMicrophoneLevel(uint16_t& magnitude);

/** Queues feedback without blocking the caller. */
void beep();
void beepLong();

}  // namespace AudioFeedback
