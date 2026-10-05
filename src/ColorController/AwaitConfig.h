#pragma once

#include <stdint.h>

#define SIMPLEAWAIT_MAX_TASKS 8
#define SIMPLEAWAIT_FRAME_POOL_BYTES 3072
#define SIMPLEAWAIT_ON_ERROR(error) (::AwaitStatus::record(error))

/**
 * Single-slot scheduler diagnostic bridge.
 *
 * SimpleAwait calls record() from normal cooperative scheduler context. The
 * touch task consumes the latest value and reports it over Serial. Multiple
 * failures before the next poll intentionally coalesce because the firmware
 * enters persistent fatal reporting for startup allocation failures.
 */
namespace AwaitStatus {

inline uint8_t lastError = 0;
inline bool hasError = false;

template <typename ErrorType>
void record(ErrorType error) {
  lastError = static_cast<uint8_t>(error);
  hasError = true;
}

/**
 * Retrieves and clears the most recent scheduler error.
 *
 * @param error Receives the recorded error code when one is pending.
 * @return true when an error was consumed.
 */
inline bool take(uint8_t& error) {
  if (!hasError) {
    return false;
  }
  error = lastError;
  hasError = false;
  return true;
}

}  // namespace AwaitStatus

#include <SimpleAwait.h>
