// NTC conversion: the anchor point (R = R0 must read exactly t0 whatever the
// beta shape), a known off-anchor value, divider round-trip, and the rails —
// which must report a BROKEN SENSOR, never a plausible temperature.
#include "ntc.hpp"

#include <cmath>

#include "alloy_test.hpp"

namespace {
using namespace alloy::lib::ntc;
constexpr config k10k{};  // 10k pull-up, 10k@25C, beta 3435, 16-bit scale
}

ALLOY_TEST(ntc_anchor_point_reads_t0_exactly) {
    converter conv{k10k};
    // R = R0 = pull-up -> mid-scale counts.
    const auto t = conv.celsius(0x8000u);
    ALLOY_CHECK(t.has_value());
    ALLOY_CHECK(std::fabs(*t - 25.0f) < 0.05f);
}

ALLOY_TEST(ntc_divider_recovers_resistance) {
    converter conv{k10k};
    // counts = full * R/(R+Rp) with R = 30k -> 3/4 scale.
    const auto r = conv.resistance(static_cast<std::uint16_t>(0xFFFFu * 3u / 4u));
    ALLOY_CHECK(r.has_value());
    ALLOY_CHECK(std::fabs(*r - 30'000.0f) < 150.0f);
    // And a colder-than-anchor reading converts below 25 C.
    const auto t = conv.celsius(static_cast<std::uint16_t>(0xFFFFu * 3u / 4u));
    ALLOY_CHECK(t.has_value());
    ALLOY_CHECK(*t < 5.0f && *t > -20.0f);
}

ALLOY_TEST(ntc_ln_linear_beta_bends_the_curve) {
    config c = k10k;
    c.beta_ln = 25.0f;  // beta grows with ln(R)
    converter bent{c};
    converter fixed{k10k};
    const auto cold_counts = static_cast<std::uint16_t>(0xFFFFu * 3u / 4u);
    const auto a = bent.celsius(cold_counts);
    const auto b = fixed.celsius(cold_counts);
    ALLOY_CHECK(a.has_value() && b.has_value());
    ALLOY_CHECK(std::fabs(*a - *b) > 0.05f);  // the term is load-bearing
    // ...but both still agree exactly at the anchor.
    const auto aa = bent.celsius(0x8000u);
    ALLOY_CHECK(aa.has_value());
    ALLOY_CHECK(std::fabs(*aa - 25.0f) < 0.05f);
}

ALLOY_TEST(ntc_rails_report_broken_sensor_not_temperature) {
    converter conv{k10k};
    const auto open = conv.celsius(0xFFFFu);
    ALLOY_CHECK(!open);
    ALLOY_CHECK(open.error() == fault::open);
    const auto shorted = conv.celsius(3u);
    ALLOY_CHECK(!shorted);
    ALLOY_CHECK(shorted.error() == fault::shorted);
}
