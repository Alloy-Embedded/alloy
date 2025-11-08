/// Host (PC) Delay Implementation
///
/// Uses std::this_thread::sleep_for for accurate delays on host platforms.
/// This allows testing embedded code on a PC without hardware.
///
/// Platform Support:
/// - Linux (POSIX)
/// - macOS
/// - Windows
///
/// Note: Unlike embedded busy-wait delays, this yields CPU time to other threads.

#ifndef ALLOY_HAL_HOST_DELAY_HPP
#define ALLOY_HAL_HOST_DELAY_HPP

#include <chrono>
#include <thread>

#include <stdint.h>

namespace alloy::hal::host {

/// Sleep for specified milliseconds
///
/// Uses std::this_thread::sleep_for which yields CPU time.
/// More efficient than busy-waiting on host platforms.
///
/// @param milliseconds Number of milliseconds to delay
inline void delay_ms(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

/// Sleep for specified microseconds
///
/// Uses std::this_thread::sleep_for with microsecond precision.
/// Actual precision depends on OS scheduler (typically 1-100us).
///
/// @param microseconds Number of microseconds to delay
inline void delay_us(uint32_t microseconds) {
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

}  // namespace alloy::hal::host

#endif  // ALLOY_HAL_HOST_DELAY_HPP
