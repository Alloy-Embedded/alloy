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

// Microseconds elapsed since board::init(), modulo 2^32 (wraps every ~71.6
// minutes). Intended for DELTAS via wrap-safe unsigned subtraction — protocol
// inter-frame gaps (Modbus RTU t3.5 is 1.75 ms above 19200 baud, below the
// 1 ms tick), edge timing, short timeouts. On Cortex-M it interpolates the
// SysTick counter between ticks, so it advances even while interrupts are
// masked — but the millisecond base freezes if the tick ISR is blocked longer
// than one period. Same CCOUNT derivation (and v1 wrap caveat) as uptime_ms
// on Xtensa.
[[nodiscard]] std::uint32_t uptime_us();

namespace literals {
using namespace std::chrono_literals;  // NOLINT: intentional re-export
}

}  // namespace alloy
