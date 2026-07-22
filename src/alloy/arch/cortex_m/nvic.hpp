// NVIC — ARM-architectural (same standing as SysTick): addresses fixed by
// the ARMv6-M/v7-M architecture, allowed in arch/ by contract guard #1.
//
// ARMv6-M constraint honored: IPR supports WORD access only — priorities
// are set read-modify-write on the containing word, never by byte store.

#pragma once

#include <cstdint>

#include "alloy/core/mmio.hpp"

namespace alloy::arch::cortex_m::nvic {

inline constexpr std::uintptr_t kIser = 0xE000E100;  // set-enable
inline constexpr std::uintptr_t kIcer = 0xE000E180;  // clear-enable
inline constexpr std::uintptr_t kIcpr = 0xE000E280;  // clear-pending
inline constexpr std::uintptr_t kIpr = 0xE000E400;   // priority

inline void enable(unsigned n) {
    *reinterpret_cast<rw32*>(kIser + 4u * (n >> 5)) = 1u << (n & 31u);
}

inline void disable(unsigned n) {
    *reinterpret_cast<rw32*>(kIcer + 4u * (n >> 5)) = 1u << (n & 31u);
    __asm volatile("dsb\n\tisb");  // ensure the disable takes effect before return
}

inline void clear_pending(unsigned n) {
    *reinterpret_cast<rw32*>(kIcpr + 4u * (n >> 5)) = 1u << (n & 31u);
}

// prio is the full 8-bit field; chips implement only the TOP nvic_prio_bits
// bits, so mid-scale 0x80 lands mid-scale on every part.
inline void set_priority(unsigned n, std::uint8_t prio) {
    rw32& word = *reinterpret_cast<rw32*>(kIpr + 4u * (n >> 2));
    const unsigned shift = 8u * (n & 3u);
    word = (word & ~(0xFFu << shift)) | (static_cast<std::uint32_t>(prio) << shift);
}

}  // namespace alloy::arch::cortex_m::nvic
