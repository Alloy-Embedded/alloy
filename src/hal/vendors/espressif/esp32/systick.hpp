/// ESP32 SysTick Timer Implementation
///
/// Uses esp_timer API for microsecond time tracking.
/// ESP32 has a 64-bit timer at 80MHz (APB clock), we use esp_timer_get_time()
/// which provides microsecond resolution.
///
/// Memory Usage:
/// - 0 bytes RAM (uses ESP-IDF timer)
/// - ~50 bytes code

#ifndef ALLOY_HAL_ESP32_SYSTICK_HPP
#define ALLOY_HAL_ESP32_SYSTICK_HPP

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/interface/systick.hpp"

// ESP-IDF timer function (provided by esp-idf)
extern "C" {
    int64_t esp_timer_get_time(void);
}

namespace alloy::hal::espressif::esp32 {

/// ESP32 SysTick implementation using esp_timer
class SystemTick {
public:
    /// Initialize SysTick timer
    ///
    /// ESP32's esp_timer is always running, so this just marks as initialized.
    ///
    /// @return Ok on success
    static core::Result<void> init() {
        initialized_ = true;
        return core::Result<void>::ok();
    }

    /// Get current time in microseconds
    ///
    /// Uses esp_timer_get_time() which returns 64-bit microsecond counter.
    /// We return lower 32 bits to match interface.
    ///
    /// @return Microseconds (lower 32 bits)
    static core::u32 micros() {
        return static_cast<core::u32>(esp_timer_get_time());
    }

    /// Reset counter to zero
    ///
    /// ESP32 timer cannot be reset, so we just store offset.
    ///
    /// @return Ok on success
    static core::Result<void> reset() {
        offset_ = esp_timer_get_time();
        return core::Result<void>::ok();
    }

    /// Check if initialized
    ///
    /// @return true if init() has been called
    static bool is_initialized() {
        return initialized_;
    }

private:
    static inline bool initialized_ = false;
    static inline int64_t offset_ = 0;
};

// Validate concept compliance at compile time
static_assert(alloy::hal::SystemTick<SystemTick>,
              "ESP32 SystemTick must satisfy SystemTick concept");

} // namespace alloy::hal::espressif::esp32

/// Global namespace implementation for ESP32
namespace alloy::systick::detail {
    inline core::u32 get_micros() {
        return alloy::hal::espressif::esp32::SystemTick::micros();
    }
}

#endif // ALLOY_HAL_ESP32_SYSTICK_HPP
