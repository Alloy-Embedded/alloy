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

// Map a portable urgency level (0 = most urgent) to the 8-bit priority field,
// placed in the top 3 bits so 8 distinct levels survive on any part that
// implements >= 3 priority bits (fewer bits coarsen the low end but keep order).
inline constexpr std::uint8_t prio_value(std::uint8_t level) {
    return static_cast<std::uint8_t>((level & 0x7u) << 5u);
}

#if !defined(__ARM_ARCH_6M__)
// BASEPRI exists on ARMv7-M and up (M3/M4/M7), not on ARMv6-M (M0/M0+). It masks
// every interrupt whose priority value is numerically >= BASEPRI (0 = masking
// off), so a critical section can leave more-urgent interrupts live.
inline std::uint32_t basepri_get() {
    std::uint32_t v;
    __asm volatile("mrs %0, BASEPRI" : "=r"(v));
    return v;
}
inline void basepri_set(std::uint32_t v) {
    __asm volatile("msr BASEPRI, %0" ::"r"(v) : "memory");
}
#endif

}  // namespace alloy::arch::cortex_m::nvic
