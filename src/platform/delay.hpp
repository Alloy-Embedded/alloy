#ifndef ALLOY_PLATFORM_DELAY_HPP
#define ALLOY_PLATFORM_DELAY_HPP

#include <cstdint>

#ifdef __cplusplus
#include <thread>
#include <chrono>
#endif

namespace alloy::platform {

/// Delay execution for specified milliseconds
///
/// This is a platform-specific implementation. On host (PC), it uses
/// std::this_thread::sleep_for(). On embedded targets, it will use
/// hardware timers or busy-wait loops.
///
/// @param milliseconds Number of milliseconds to delay
///
/// Example:
/// \code
/// #include "platform/delay.hpp"
///
/// alloy::platform::delay_ms(1000);  // Wait 1 second
/// \endcode
inline void delay_ms(uint32_t milliseconds) {
#ifdef __cplusplus
    // Host implementation using C++ standard library
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#else
    // For bare-metal targets, this would be implemented using hardware timers
    // or busy-wait loops. To be implemented per-platform.
    (void)milliseconds;
#endif
}

} // namespace alloy::platform

#endif // ALLOY_PLATFORM_DELAY_HPP
