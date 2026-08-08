// Cortex-M SysTick timebase (1 kHz).
//
// The SysTick register block is defined by the ARMv6-M/v7-M/v8-M
// architecture itself (address 0xE000E010 on every Cortex-M); architecture
// constants are the one class of hardware fact allowed in hand-written code,
// and they live only under src/alloy/arch/. See scripts/check_contract.sh.

#pragma once

#include <cstdint>

namespace alloy::arch::cortex_m {

// Start the 1 kHz tick from the core clock. Called by generated board::init().
void systick_init(std::uint32_t core_hz);

// Enable interrupts globally (CPSIE i). Called by generated board::init().
inline void enable_irq() { __asm volatile("cpsie i" ::: "memory"); }

// --- Core-clock stamps, for MEASURING short code paths ---------------------
//
// uptime_us() is the portable timebase and the one application code should use;
// it resolves to 1 µs, which at 16-64 MHz is 16-64 core clocks — too coarse to
// time an exception entry or one executor resume. These three expose the SysTick
// counter underneath it at its native resolution: one count is one core clock.
//
// Cortex-M ONLY, and deliberately not re-exported as portable alloy:: API —
// ARMv6-M has no DWT cycle counter, Xtensa and RL78 count something else, and a
// name that means a different thing per arch is worse than no name. The in-tree
// user is examples/concurrency_probe (see docs/guide/async.md).

// Core-clock counts in one 1 kHz tick (SysTick RVR + 1).
[[nodiscard]] std::uint32_t systick_period();

// Counts elapsed inside the CURRENT tick, up-counting 0..period-1. A single
// volatile load of the down-counting CVR, subtracted from the period — ISR-safe,
// no mask, no division.
[[nodiscard]] std::uint32_t systick_elapsed();

// Counts from stamp `a` to stamp `b`, correct across at most ONE reload.
// Intervals longer than one tick (1 ms) alias and must use uptime_us() instead.
[[nodiscard]] std::uint32_t systick_since(std::uint32_t a, std::uint32_t b);

}  // namespace alloy::arch::cortex_m
