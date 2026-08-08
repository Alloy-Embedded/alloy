// RL78 timebase.
//
// There is no core timer and no cycle counter on this architecture — no SysTick,
// no CCOUNT. A tick therefore has to come from one of the part's own TAU
// channels, which is a BOARD decision (the application may already want that
// channel), not something this layer can assume.
//
// v1 does the honest minimum: record the core clock so `sleep_for` can busy-wait
// a calibrated loop, and say plainly that the wait is approximate. Nothing in
// alloy depends on a tick ISR existing; `alloy::uptime_us()` and the async
// executor do, and neither is available on this arch until a board declares
// which timer channel feeds them.
//
// Named systick.* like the other backends so the generated board.cpp stays
// arch-agnostic.

#pragma once

#include <cstdint>

namespace alloy::arch::rl78 {

// Record the CPU clock for the delay loop. Called by board::init().
void systick_init(std::uint32_t core_hz);

// Enable maskable interrupts (PSW.IE = 1). Called at the end of board::init(),
// after every line the board wants has been unmasked.
inline void enable_irq() { __asm volatile("ei" ::: "memory"); }

}  // namespace alloy::arch::rl78
