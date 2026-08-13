// The LPUART's divisor, on its own, so a host test can reach it.
//
// WHY THIS IS A SEPARATE HEADER AND NOT FOUR LINES IN THE DRIVER: the driver
// is a template over a GENERATED instance, so nothing in tests/ can call it
// without a generated chip. The arithmetic that decides whether a port runs at
// the baud the caller asked for is the one part of a UART driver that can be
// wrong silently, so it is spelled where `tests/test_lpuart_baud.cpp` can pin
// it with numbers.
//
// THE MATHS, and it is the whole reason the LPUART is not st/usart_v4:
//
//     LPUARTDIV = (256 x f_ck) / baud          (RM0444 §37.4.4)
//
// against the USART's plain `f_ck / baud`. Two consequences, and the second is
// the one a tolerance check cannot express:
//
//   * 256 x 64 MHz overflows 32 bits, so every step here is 64-bit. A u32
//     intermediate wraps at ~16.7 MHz of kernel clock and produces a divisor
//     that looks perfectly reasonable.
//   * The divisor has a WINDOW, not just a rounding error. BRR is 20 bits, so
//     the largest divisor is 0xFFFFF, and RM0444 forbids writing anything
//     below 0x300. In terms the caller cares about: the kernel clock must be
//     between 3x and 4096x the baud rate. Outside that there is no divisor at
//     all — not a bad one, none — and a UART tolerance check phrased as
//     "within 2% of the target" cannot say so, because the achieved rate it
//     would compare against does not exist.
//
// Both bounds are ARGUMENTS here rather than constants: `brr_min` comes from
// the chip database (`Inst::feat::brr_min`, curated from the prose rule in
// RM0444) and `brr_max` from the curated width of the BRR field
// (`IP::brr.raw_mask`). Neither is a number typed into alloy.

#pragma once

#include <cstdint>

namespace alloy::hal::lpuart {

// Exact LPUARTDIV, rounded to nearest, with no 32-bit intermediate. Returns 0
// for a zero baud (the caller's admission test names that case; this one just
// refuses to divide by it).
[[nodiscard]] constexpr std::uint64_t divisor_wide(std::uint32_t kernel_hz,
                                                   std::uint32_t baud) {
    if (baud == 0u) {
        return 0u;
    }
    const std::uint64_t scaled = static_cast<std::uint64_t>(kernel_hz) << 8u;
    return (scaled + baud / 2u) / baud;
}

// Is there a divisor at all? The bounds are the silicon's, passed in.
[[nodiscard]] constexpr bool in_window(std::uint32_t kernel_hz, std::uint32_t baud,
                                       std::uint32_t brr_min, std::uint32_t brr_max) {
    if (kernel_hz == 0u || baud == 0u) {
        return false;
    }
    const std::uint64_t div = divisor_wide(kernel_hz, baud);
    return div >= brr_min && div <= brr_max;
}

// The value to program. Only meaningful when in_window() is true; out of the
// window it is clamped to the nearest legal divisor rather than allowed to
// truncate into BRR, because a truncated 20-bit write is a DIFFERENT baud
// rate that looks like a working port. Callers reject first; the clamp exists
// so the register write is never the lie.
[[nodiscard]] constexpr std::uint32_t divisor(std::uint32_t kernel_hz, std::uint32_t baud,
                                              std::uint32_t brr_min, std::uint32_t brr_max) {
    const std::uint64_t div = divisor_wide(kernel_hz, baud);
    if (div < brr_min) {
        return brr_min;
    }
    if (div > brr_max) {
        return brr_max;
    }
    return static_cast<std::uint32_t>(div);
}

// What the port will actually run at — computed by INVERTING the divisor this
// header programs, so a tolerance check can never disagree with the hardware.
// Zero outside the window, which is the honest answer: no divisor exists, so
// no rate is achieved, and `rate_check<requested, 0, tol>` fails loudly.
[[nodiscard]] constexpr std::uint32_t achieved_hz(std::uint32_t kernel_hz, std::uint32_t baud,
                                                  std::uint32_t brr_min, std::uint32_t brr_max) {
    if (!in_window(kernel_hz, baud, brr_min, brr_max)) {
        return 0u;
    }
    const std::uint64_t div = divisor_wide(kernel_hz, baud);
    const std::uint64_t scaled = static_cast<std::uint64_t>(kernel_hz) << 8u;
    return static_cast<std::uint32_t>(scaled / div);
}

}  // namespace alloy::hal::lpuart
