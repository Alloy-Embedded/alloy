// Portable timebase — part of the chip contract, never user code.
//
// board::init() starts the architecture timebase (SysTick on Cortex-M);
// after that, alloy::sleep_for() works identically on every board. This is
// the API whose absence forced #ifdef __ARM_ARCH into the old blink.

#pragma once

#include <chrono>

namespace alloy {

// Blocks for at least `d`. Resolution is the timebase tick (1 ms on the
// walking skeleton); sub-tick requests round up to one tick.
void sleep_for(std::chrono::microseconds d);

// Milliseconds elapsed since board::init().
[[nodiscard]] std::uint32_t uptime_ms();

namespace literals {
using namespace std::chrono_literals;  // NOLINT: intentional re-export
}

}  // namespace alloy
