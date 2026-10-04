#pragma once

#include <stdint.h>

#define SIMPLEAWAIT_MAX_TASKS 6
#define SIMPLEAWAIT_FRAME_POOL_BYTES 2048
#define SIMPLEAWAIT_ON_ERROR(error) (::AwaitStatus::record(error))

namespace AwaitStatus {

inline uint8_t lastError = 0;
inline bool hasError = false;

template <typename ErrorType>
void record(ErrorType error) {
  lastError = static_cast<uint8_t>(error);
  hasError = true;
}

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
