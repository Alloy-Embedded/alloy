// Unit tests for the typed frequency + compile-time tolerance layer
// (alloy/core/units.hpp). The driver achieved_baud() formulas are exercised by
// the cross-board build (they need generated IP headers); here we pin the pure,
// board-free math: literals, the chrono bridge, and the tolerance predicate
// that rate_check<> gates on.

#include <chrono>
#include <cstdint>

#include "alloy/core/units.hpp"
#include "alloy_test.hpp"

using namespace alloy;
using namespace alloy::literals;

ALLOY_TEST(frequency_literals_scale) {
    ALLOY_CHECK_EQ((1_Hz).hz(), 1u);
    ALLOY_CHECK_EQ((115200_baud).hz(), 115200u);
    ALLOY_CHECK_EQ((100_kHz).hz(), 100000u);
    ALLOY_CHECK_EQ((16_MHz).hz(), 16000000u);
    ALLOY_CHECK_EQ((64_MHz).hz(), 64000000u);
    // baud is a frequency: the two literals are interchangeable values.
    ALLOY_CHECK(9600_baud == 9600_Hz);
}

ALLOY_TEST(frequency_compares_and_defaults) {
    ALLOY_CHECK(frequency{} == 0_Hz);
    ALLOY_CHECK(1_kHz > 999_Hz);
    ALLOY_CHECK(1_MHz == 1000_kHz);
    ALLOY_CHECK(48_MHz >= 48_MHz);
}

ALLOY_TEST(period_bridges_to_chrono) {
    // period is the reciprocal in nanoseconds; time stays chrono.
    ALLOY_CHECK(period(1_MHz) == std::chrono::nanoseconds{1000});   // 1 us
    ALLOY_CHECK(period(1000_Hz) == std::chrono::microseconds{1000});
    ALLOY_CHECK(period(0_Hz) == std::chrono::nanoseconds{0});       // guarded div-by-zero
}

ALLOY_TEST(within_permille_boundaries) {
    // exact match is always within any tolerance
    ALLOY_CHECK(within_permille(115200_baud, 115200_baud, 0u));
    // 20 permille == 2%. 115200 +/- 2% = [112896, 117504].
    ALLOY_CHECK(within_permille(115200_baud, frequency{117504u}, 20u));   // +2.0% edge -> in
    ALLOY_CHECK(within_permille(115200_baud, frequency{112896u}, 20u));   // -2.0% edge -> in
    ALLOY_CHECK(!within_permille(115200_baud, frequency{117505u}, 20u));  // just over -> out
    ALLOY_CHECK(!within_permille(115200_baud, frequency{100000u}, 20u));  // -13% -> out
    // tighter tolerance rejects what a looser one accepts
    ALLOY_CHECK(within_permille(115200_baud, frequency{115740u}, 20u));   // 0.47% within 2%
    ALLOY_CHECK(!within_permille(115200_baud, frequency{115740u}, 2u));   // 0.47% NOT within 0.2%
}

ALLOY_TEST(within_permille_no_overflow_at_high_rates) {
    // cross-multiply is u64: 480 MHz * 1000 would overflow u32 but must not here.
    ALLOY_CHECK(within_permille(480_MHz, 480_MHz, 1u));
    ALLOY_CHECK(within_permille(480_MHz, frequency{481000000u}, 5u));    // 0.21% within 0.5%
    ALLOY_CHECK(!within_permille(480_MHz, frequency{500000000u}, 5u));   // 4.2% out
}

// The realistic achieved-baud round trips the drivers compute (verified here as
// the exact integers the single-source formulas produce, so a regression in the
// tolerance math shows up even though the driver headers can't be host-included).
ALLOY_TEST(realistic_baud_tolerances) {
    // ST BRR: 64 MHz / round(64e6/115200)=556 -> 115107; 0.08% -> ok at 2%.
    ALLOY_CHECK(within_permille(115200_baud, frequency{115107u}, 20u));
    // SAME70 CD: 150 MHz / (16*round(150e6/(16*115200))=81) -> 115740; 0.47% -> ok.
    ALLOY_CHECK(within_permille(115200_baud, frequency{115740u}, 20u));
    // A 12 MHz kernel cannot make 115200 well: BRR=round(12e6/115200)=104 ->
    // 115384 (0.16%, fine) but 3 Mbaud is impossible: BRR=round(12e6/3e6)=4 ->
    // 3.0 MHz exact here; pick a genuinely bad one: 2.5 Mbaud on 12 MHz ->
    // BRR=round(12e6/2.5e6)=5 -> 2.4 MHz, 4% off -> must be rejected at 2%.
    ALLOY_CHECK(!within_permille(2500000_baud, frequency{2400000u}, 20u));
}
