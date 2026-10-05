#pragma once

#include <stdint.h>

#include "ControllerMessages.h"
#include "driver/i2c_master.h"

/**
 * Duplex audio service for button feedback and onboard microphone sampling.
 *
 * The service owns the codec device handle, I2S channels, audio task, and
 * fixed-size tone/microphone buffers. Callers never block on playback or input.
 * Cue requests coalesce in ControllerMessages; microphone delivery is a
 * destructive latest-value read rather than a historical sample stream.
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
bool start(i2c_master_bus_handle_t touchBus, ControllerMessages& messages);

/** Distinguishes startup from a confirmed microphone failure for the UI. */
MicrophoneStatus microphoneStatus();

/**
 * Takes the latest amplitude sample published by the audio coroutine.
 *
 * @param magnitude Receives the mean absolute sample magnitude on success.
 * @return true when a newer amplitude was published since the previous
 *         successful read; false leaves magnitude unchanged.
 */
bool readMicrophoneLevel(uint16_t& magnitude);

}  // namespace AudioFeedback
