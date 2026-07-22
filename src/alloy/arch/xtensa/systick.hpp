// Xtensa timebase — CCOUNT cycle-counter polling (v1: no interrupt
// machinery on this arch yet, so no tick ISR; alloy::sleep_for busy-waits
// on CCOUNT deltas, which are wrap-safe for waits < 2^32 cycles ≈ 53 s at
// 80 MHz). Named systick.* so the generated board.cpp stays arch-agnostic.

#pragma once

#include <cstdint>

namespace alloy::arch::xtensa {

// Record the CPU clock for CCOUNT-based delays. Called by board::init().
void systick_init(std::uint32_t core_hz);

// Drop PS.INTLEVEL to 0: level-1 interrupts (the only ones alloy enables)
// become deliverable. Called at the end of board::init().
inline void enable_irq() {
    std::uint32_t tmp;
    __asm volatile("rsil %0, 0" : "=a"(tmp));
}

}  // namespace alloy::arch::xtensa
