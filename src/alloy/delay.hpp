// A blocking delay object satisfying the DelayNs concept, built on the portable
// sleep_for timebase. Drivers (sensor conversion waits, reset pulses) constrain
// on DelayNs and take one of these; user code just passes alloy::delay{}.
//
// Resolution is the platform tick (~1 ms on Cortex-M's SysTick, CCOUNT-derived
// on ESP32 v1); sub-tick requests round UP to at least the requested time, never
// less. For microsecond-precise pulses a driver should use a cycle-counter
// delay instead — not offered in v1.

#pragma once

#include <chrono>
#include <cstdint>

#include "alloy/concepts.hpp"
#include "alloy/time.hpp"

namespace alloy {

struct delay {
    void delay_ns(std::uint32_t ns) const {
        alloy::sleep_for(std::chrono::microseconds{(ns + 999u) / 1000u});
    }
    void delay_us(std::uint32_t us) const {
        alloy::sleep_for(std::chrono::microseconds{us});
    }
    void delay_ms(std::uint32_t ms) const {
        alloy::sleep_for(std::chrono::microseconds{ms * 1000u});
    }
};

static_assert(DelayNs<delay>);

}  // namespace alloy
