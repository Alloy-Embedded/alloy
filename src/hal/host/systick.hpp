/// Host SysTick timer implementation
///
/// Uses std::chrono for microsecond time tracking on host platforms.

#ifndef ALLOY_HAL_HOST_SYSTICK_HPP
#define ALLOY_HAL_HOST_SYSTICK_HPP

#include <chrono>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/systick.hpp"

namespace alloy::hal::host {

class SystemTick {
   public:
    static core::Result<void, core::ErrorCode> init() {
        start_time_ = std::chrono::steady_clock::now();
        initialized_ = true;
        return core::Ok();
    }

    static core::u32 micros() {
        if (!initialized_) {
            return 0;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
        return static_cast<core::u32>(elapsed.count());
    }

    static core::Result<void, core::ErrorCode> reset() {
        start_time_ = std::chrono::steady_clock::now();
        return core::Ok();
    }

    static bool is_initialized() { return initialized_; }

   private:
    static inline std::chrono::steady_clock::time_point start_time_{};
    static inline bool initialized_ = false;
};

static_assert(alloy::hal::SystemTick<SystemTick>,
              "Host SystemTick must satisfy SystemTick concept");

}  // namespace alloy::hal::host

namespace alloy::systick::detail {

inline core::u32 get_micros() {
    return alloy::hal::host::SystemTick::micros();
}

}  // namespace alloy::systick::detail

#endif  // ALLOY_HAL_HOST_SYSTICK_HPP
