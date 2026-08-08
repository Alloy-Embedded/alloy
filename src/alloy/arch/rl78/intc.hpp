// The RL78 interrupt controller, as the architecture defines it.
//
// These addresses are architectural, not per-part — the same in every RL78, the
// way 0xE000E100 is the same in every Cortex-M. That is why they live under
// src/alloy/arch/ (which the address contract gate exempts) rather than in
// alloy-devices: no chip file could make them different.
//
// A vector number selects a BANK and a bit inside it:
//
//   bank 0  vectors  0..15   IF0 0xFFFE0  MK0 0xFFFE4  PR00 0xFFFE8  PR10 0xFFFEC
//   bank 1  vectors 16..31   IF1 0xFFFE2  MK1 0xFFFE6  PR01 0xFFFEA  PR11 0xFFFEE
//   bank 2  vectors 32..47   IF2 0xFFFD0  MK2 0xFFFD4  PR02 0xFFFD8  PR12 0xFFFDC
//
// Banks 0 and 1 interleave inside 0xFFFE0..0xFFFEF; bank 2 sits on its own at
// 0xFFFD0..0xFFFDF. WHICH bits a given part populates is a chip fact and stays
// in the device database — this file only says where the registers are.
//
// Two RL78 conventions worth stating because they invert the usual sense:
//   * MK is a MASK: 1 disables the source, 0 enables it. Enabling a line CLEARS
//     a bit, which is the opposite of the NVIC's ISER.
//   * priority is two bits, (PR1,PR0), and 0 is the MOST urgent — which matches
//     alloy's irq_line_priority contract, but 3 (not 255) is the least.

#pragma once

#include <cstdint>

namespace alloy::arch::rl78 {

// RL78 is a 20-bit machine addressed by 16-bit near pointers: a data operand
// of 0xFFE0 IS the byte at physical 0xFFFE0, because near addressing covers
// 0xF0000..0xFFFFF. So `std::uintptr_t` here is 16 bits and a physical SFR
// address does not fit in one.
//
// Writing the physical address and letting it truncate happens to work and is
// the wrong way to rely on it — GCC warns, and the warning is right: the value
// changes. `near()` keeps the manual's address in the source, proves it is
// inside the window that mapping covers, and hands back what the CPU wants.
[[nodiscard]] constexpr std::uintptr_t near(unsigned long physical) {
    // Outside the near window there is no 16-bit form; a far access is a
    // different instruction and a different decision.
    return static_cast<std::uintptr_t>(physical & 0xFFFFu);
}

inline constexpr unsigned kVectorsPerBank = 16;
inline constexpr unsigned kBanks = 3;
inline constexpr unsigned kMaxVector = kBanks * kVectorsPerBank;

// Bank 0/1 are two bytes apart inside one block; bank 2 lives elsewhere.
inline constexpr std::uintptr_t kIfBase = near(0xFFFE0);
inline constexpr std::uintptr_t kMkBase = near(0xFFFE4);
inline constexpr std::uintptr_t kPr0Base = near(0xFFFE8);
inline constexpr std::uintptr_t kPr1Base = near(0xFFFEC);
inline constexpr std::uintptr_t kBank2If = near(0xFFFD0);
inline constexpr std::uintptr_t kBank2Mk = near(0xFFFD4);
inline constexpr std::uintptr_t kBank2Pr0 = near(0xFFFD8);
inline constexpr std::uintptr_t kBank2Pr1 = near(0xFFFDC);

//: The 16-bit register holding `n`'s bit, for one of the four control planes.
enum class plane { request, mask, prio_low, prio_high };

[[nodiscard]] constexpr std::uintptr_t reg_of(plane p, unsigned n) {
    const unsigned bank = n / kVectorsPerBank;
    if (bank == 2) {
        switch (p) {
            case plane::request: return kBank2If;
            case plane::mask: return kBank2Mk;
            case plane::prio_low: return kBank2Pr0;
            case plane::prio_high: return kBank2Pr1;
        }
    }
    const std::uintptr_t off = 2u * bank;
    switch (p) {
        case plane::request: return kIfBase + off;
        case plane::mask: return kMkBase + off;
        case plane::prio_low: return kPr0Base + off;
        case plane::prio_high: return kPr1Base + off;
    }
    return 0;  // unreachable; keeps every path returning a value
}

[[nodiscard]] constexpr std::uint16_t bit_of(unsigned n) {
    return static_cast<std::uint16_t>(std::uint32_t{1} << (n % kVectorsPerBank));
}

inline volatile std::uint16_t& reg(plane p, unsigned n) {
    return *reinterpret_cast<volatile std::uint16_t*>(reg_of(p, n));
}

}  // namespace alloy::arch::rl78
