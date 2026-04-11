#pragma once

#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

class Watchdog {
   public:
    template <typename WatchdogPolicy>
    static void disable() {
        WatchdogPolicy::disable();
    }

    template <typename WatchdogPolicy>
    static void enable() {
        WatchdogPolicy::enable();
    }

    template <typename WatchdogPolicy>
    static void enable_with_timeout(u16 timeout_ms) {
        WatchdogPolicy::enable_with_timeout(timeout_ms);
    }

    template <typename WatchdogPolicy>
    static void feed() {
        WatchdogPolicy::feed();
    }

    template <typename WatchdogPolicy>
    [[nodiscard]] static auto is_enabled() -> bool {
        return WatchdogPolicy::is_enabled();
    }

    template <typename WatchdogPolicy>
    [[nodiscard]] static auto get_remaining_time_ms() -> u16 {
        return WatchdogPolicy::get_remaining_time_ms();
    }
};

}  // namespace alloy::hal
