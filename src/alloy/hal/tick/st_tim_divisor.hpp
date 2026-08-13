// Choosing PSC and ARR for a requested update rate — arithmetic only, no
// registers, so it is testable on a host and shared by every ST timer driver.
//
// The identity the hardware implements is
//
//     update_hz = kernel_hz / ((PSC + 1) * (ARR + 1))
//
// with PSC and ARR each 16 bits. TWO NUMBERS, ONE PRODUCT: the same rate has
// many spellings, and the choice between them is not cosmetic. Any PSC above
// zero coarsens the counter — CNT then advances once per (PSC+1) kernel
// cycles, so it is also the resolution a caller reading count() gets. This
// picks the SMALLEST prescaler that makes the reload fit, which is the same as
// picking the finest counter that can reach the rate.
//
// NOT WITNESSED ON SILICON. These are arithmetic identities about a divider
// documented in RM0444; nothing here says the block then counts.

#pragma once

#include <cstdint>

namespace alloy::hal {

struct tick_divisor {
    std::uint16_t psc;  //: written to PSC; the divider is psc + 1
    std::uint16_t arr;  //: written to ARR; the period is arr + 1 counts
};

//: Both registers are 16 bits, so each divider is at most 65536.
inline constexpr std::uint32_t kTickMaxDivide = 65536u;

// The total division a rate needs, rounded to nearest. Zero means the rate is
// faster than the kernel clock; the caller treats that as impossible rather
// than silently running at kernel_hz.
[[nodiscard]] constexpr std::uint32_t tick_total_divide(std::uint32_t kernel_hz,
                                                        std::uint32_t hz) {
    if (hz == 0u || kernel_hz == 0u) {
        return 0u;
    }
    return (kernel_hz + hz / 2u) / hz;
}

// Can this kernel clock produce this rate at all? Both ends are real:
// a rate above the kernel clock has no divisor, and a rate below
// kernel_hz / 2^32 needs more division than two 16-bit registers hold.
[[nodiscard]] constexpr bool tick_representable(std::uint32_t kernel_hz,
                                                std::uint32_t hz) {
    // ABOVE THE KERNEL CLOCK IS NOT REPRESENTABLE, and rounding to nearest is
    // exactly why this test has to be written separately. tick_total_divide()
    // rounds, so a request of kernel_hz + 1 comes back as a divider of 1 — a
    // legal pair that runs the timer at kernel_hz and reports success. Every
    // rate between kernel_hz and 2 x kernel_hz has that shape. The check for
    // zero below catches only the far side of it.
    if (hz > kernel_hz) {
        return false;
    }
    const std::uint32_t total = tick_total_divide(kernel_hz, hz);
    if (total == 0u) {
        return false;
    }
    // ceil(total / 65536) is the smallest prescaler divider that fits ARR.
    const std::uint64_t psc_div =
        (static_cast<std::uint64_t>(total) + kTickMaxDivide - 1u) / kTickMaxDivide;
    return psc_div <= kTickMaxDivide;
}

// The finest divisor pair for this rate. Undefined unless
// tick_representable() — callers admit first (see alloy/tick.hpp), and the
// clamp below exists so a wrong caller produces the slowest legal timer
// rather than a wrapped register.
[[nodiscard]] constexpr tick_divisor tick_pick(std::uint32_t kernel_hz,
                                               std::uint32_t hz) {
    std::uint32_t total = tick_total_divide(kernel_hz, hz);
    if (total == 0u) {
        total = 1u;
    }
    std::uint64_t psc_div = (static_cast<std::uint64_t>(total) + kTickMaxDivide - 1u) /
                            kTickMaxDivide;
    if (psc_div == 0u) {
        psc_div = 1u;
    }
    if (psc_div > kTickMaxDivide) {
        psc_div = kTickMaxDivide;
    }
    // Round the reload to nearest inside the chosen prescaler, then clamp: the
    // rounding can land on 65536 exactly (total = 65536 * psc_div - something
    // small), which is one past what ARR holds.
    std::uint64_t arr_div = (static_cast<std::uint64_t>(total) + psc_div / 2u) / psc_div;
    if (arr_div == 0u) {
        arr_div = 1u;
    }
    if (arr_div > kTickMaxDivide) {
        arr_div = kTickMaxDivide;
    }
    return tick_divisor{static_cast<std::uint16_t>(psc_div - 1u),
                        static_cast<std::uint16_t>(arr_div - 1u)};
}

// What the pair actually produces. The requested rate and this one differ
// whenever the division is not exact, and a caller that cares (a protocol
// timeout, a sample clock) can read it instead of assuming.
[[nodiscard]] constexpr std::uint32_t tick_achieved_hz(std::uint32_t kernel_hz,
                                                       tick_divisor d) {
    const std::uint64_t div = (static_cast<std::uint64_t>(d.psc) + 1u) *
                              (static_cast<std::uint64_t>(d.arr) + 1u);
    if (div == 0u) {
        return 0u;
    }
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(kernel_hz) + div / 2u) /
                                      div);
}

}  // namespace alloy::hal
