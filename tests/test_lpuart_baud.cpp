// The LPUART divisor (src/alloy/hal/uart/st_lpuart_baud.hpp).
//
// WHY THIS FILE EXISTS. Every other UART alloy drives divides its kernel clock
// by the baud rate. The LPUART divides 256 x kernel, into a 20-bit register
// with a forbidden floor, and those two differences produce two failure modes
// a USART cannot have:
//
//   1. A 32-bit intermediate WRAPS. 256 x 64 MHz is 16.4 billion.
//   2. Some rates have no divisor AT ALL — not an inaccurate one, none. The
//      window is 3x .. 4096x the baud rate, so a 64 MHz kernel cannot reach
//      9600 baud, and a check phrased as "within 2% of the target" cannot say
//      so because the number it would compare against does not exist.
//
// Both are silent in the failing direction: the wrap and the truncation both
// produce a divisor the hardware accepts and a port that runs at some other
// speed. So the numbers below are asserted against ST's own arithmetic —
// UART_DIV_LPUART in stm32g0xx_hal_uart.h is
// `((PCLK/presc)*256 + BAUD/2) / BAUD`, and LPUART_BRR_MIN/MAX in
// stm32g0xx_hal_uart.c are 0x300 / 0xFFFFF.
//
// NOT WITNESSED ON SILICON. No board; these are arithmetic identities, and
// they say nothing about whether the LPUART then puts bits on a wire.

#include <cstdint>

#include "alloy/hal/uart/st_lpuart_baud.hpp"
#include "alloy_test.hpp"

namespace {

using namespace alloy::hal;

// The bounds the chip database supplies, spelled once here. `MIN` is
// registers/st/lpuart_v4.yaml's `feat.brr_min`; `MAX` is the curated width of
// BRR, which the generated accessor states as raw_mask.
constexpr std::uint32_t MIN = 768u;      // 0x300
constexpr std::uint32_t MAX = 1048575u;  // 0xFFFFF, 20 bits

// The board this work is aimed at: nucleo_g0b1re, HSI16 /1 x8 /2 = 64 MHz,
// and the LPUART's kernel_clock is `apb` — RCC_CCIPR's reset selection.
constexpr std::uint32_t PCLK = 64'000'000u;

}  // namespace

ALLOY_TEST(lpuart_divisor_matches_st_formula) {
    // 115200 on 64 MHz: (64e6 * 256 + 57600) / 115200 = 142222.
    ALLOY_CHECK(lpuart::divisor_wide(PCLK, 115'200u) == 142'222u);
    // And inverting it lands back on the requested rate, which is the only
    // property the tolerance check depends on.
    ALLOY_CHECK(lpuart::achieved_hz(PCLK, 115'200u, MIN, MAX) == 115'200u);

    // 9600 from a 32.768 kHz LSE — the profile the peripheral exists for, and
    // the one alloy's clock_node cannot select yet. (32768 * 256 + 4800) /
    // 9600 = 874, comfortably inside the window.
    ALLOY_CHECK(lpuart::divisor_wide(32'768u, 9'600u) == 874u);
    ALLOY_CHECK(lpuart::in_window(32'768u, 9'600u, MIN, MAX));
}

ALLOY_TEST(lpuart_divisor_does_not_overflow_32_bits) {
    // The wrap this test exists for: 256 x 64 MHz is 16 384 000 000, which is
    // 3.8x the 32-bit range. A u32 intermediate would leave 3 799 065 600 and
    // compute a divisor of 32 977 for 115200 — a plausible-looking number that
    // programs 27 000 baud.
    ALLOY_CHECK(lpuart::divisor_wide(PCLK, 115'200u) != 32'977u);
    // The largest kernel this family ships: 256 x 170 MHz (G4) still exact.
    ALLOY_CHECK(lpuart::divisor_wide(170'000'000u, 115'200u) == 377'778u);
}

ALLOY_TEST(lpuart_window_has_a_floor_and_a_ceiling) {
    // CEILING: the divisor may not exceed 20 bits, so the kernel may not be
    // more than 4096x the baud. On 64 MHz that is 15 625 baud exactly, and
    // 15 625 is one over the edge (its divisor is 1 048 576, MAX + 1).
    ALLOY_CHECK(lpuart::divisor_wide(PCLK, 15'625u) == MAX + 1u);
    ALLOY_CHECK(!lpuart::in_window(PCLK, 15'625u, MIN, MAX));
    ALLOY_CHECK(lpuart::in_window(PCLK, 15'626u, MIN, MAX));

    // THE HEADLINE CASE. 9600 baud is the default of half the RS-485 gear in
    // the world and it is unreachable from a 64 MHz kernel. Before this check
    // existed the driver wrote (16 384 000 000 / 9600) = 1 706 667 into a
    // 20-bit field, which truncates to 0x0A0AAB and runs the port at roughly
    // 24 400 baud with nothing to say so.
    ALLOY_CHECK(!lpuart::in_window(PCLK, 9'600u, MIN, MAX));
    ALLOY_CHECK(lpuart::achieved_hz(PCLK, 9'600u, MIN, MAX) == 0u);

    // FLOOR: the divisor may not be below 0x300, so the kernel must be at
    // least 3x the baud. 64 MHz / 3 = 21 333 333 is the fastest rate.
    ALLOY_CHECK(lpuart::divisor_wide(PCLK, 21'333'333u) == MIN);
    ALLOY_CHECK(lpuart::in_window(PCLK, 21'333'333u, MIN, MAX));
    ALLOY_CHECK(!lpuart::in_window(PCLK, 21'400'000u, MIN, MAX));
}

ALLOY_TEST(lpuart_divisor_never_truncates_into_brr) {
    // Outside the window the programmed value is CLAMPED to a legal divisor
    // rather than allowed to wrap into 20 bits. Neither is a working port —
    // the caller rejects first — but a clamp is a wrong rate that stays wrong,
    // where a truncation is a wrong rate that looks arbitrary.
    ALLOY_CHECK(lpuart::divisor(PCLK, 9'600u, MIN, MAX) == MAX);
    ALLOY_CHECK((lpuart::divisor(PCLK, 9'600u, MIN, MAX) & ~MAX) == 0u);
    ALLOY_CHECK(lpuart::divisor(PCLK, 30'000'000u, MIN, MAX) == MIN);
}

ALLOY_TEST(lpuart_zero_is_not_a_rate) {
    ALLOY_CHECK(lpuart::divisor_wide(PCLK, 0u) == 0u);
    ALLOY_CHECK(!lpuart::in_window(PCLK, 0u, MIN, MAX));
    ALLOY_CHECK(!lpuart::in_window(0u, 115'200u, MIN, MAX));
    ALLOY_CHECK(lpuart::achieved_hz(0u, 115'200u, MIN, MAX) == 0u);
}

// The whole point of the window living in DATA: change the floor and the
// answer changes with it. A driver with 0x300 typed into it would pass every
// test above and still be wrong on the day a part ships a different bound.
ALLOY_TEST(lpuart_window_bounds_come_from_the_database) {
    ALLOY_CHECK(!lpuart::in_window(PCLK, 9'600u, MIN, MAX));
    ALLOY_CHECK(lpuart::in_window(PCLK, 9'600u, MIN, 4'194'303u));  // a 22-bit BRR
    ALLOY_CHECK(lpuart::in_window(PCLK, 21'400'000u, 512u, MAX));   // a lower floor
}
