#pragma once

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace simpleawait {
namespace detail {

// Local ESP32 integration: a poll-only Arduino loop otherwise spins while all
// coroutines are waiting and can starve ESP-IDF driver and idle-task work.
inline void platform_idle() noexcept {
#if defined(ARDUINO_ARCH_ESP32)
    vTaskDelay(1);
#endif
}

} // namespace detail
} // namespace simpleawait
