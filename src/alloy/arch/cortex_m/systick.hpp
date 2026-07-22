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

}  // namespace alloy::arch::cortex_m
