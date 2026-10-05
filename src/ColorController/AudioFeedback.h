#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"

/**
 * Duplex audio service for button feedback and onboard microphone sampling.
 *
 * The service owns the codec device handle, I2S channels, audio task, and
 * fixed-size tone/microphone buffers. Callers never block on playback or input.
 */
namespace AudioFeedback {

/** Observable state of asynchronous codec and microphone initialization. */
enum class MicrophoneStatus : uint8_t {
  kInitializing,
  kReady,
  kUnavailable,
};

/**
 * Starts asynchronous codec initialization and feedback processing.
 *
 * The supplied bus remains owned by the touch driver. The audio service adds
 * and later owns only its ES8311 device handle.
 *
 * @param touchBus Existing initialized I2C master bus shared with touch.
 * @return true when fixed resources and the service task were created.
 */
bool start(i2c_master_bus_handle_t touchBus);

/** Distinguishes startup from a confirmed microphone failure for the UI. */
MicrophoneStatus microphoneStatus();

/**
 * Reads one non-blocking block-amplitude sample when microphone data is ready.
 *
 * @param magnitude Receives the mean absolute sample magnitude on success.
 * @return true when a complete input block was available; false leaves
 *         magnitude unchanged.
 */
bool readMicrophoneLevel(uint16_t& magnitude);

/** Queues the normal button tone without blocking; duplicate pending requests coalesce. */
void beep();
/** Queues the longer preset-saved tone without blocking; duplicate pending requests coalesce. */
void beepLong();

}  // namespace AudioFeedback
