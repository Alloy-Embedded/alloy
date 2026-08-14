// Metering, checked against synthetic signals whose answers are known exactly.
//
// The decisive test is `power_factor_is_cos_phi`: a voltage and a current sine
// at a known phase angle must produce a power factor of cos(phi). That is the
// one assertion that distinguishes "real power is the mean of v*i" from "real
// power is Vrms*Irms" — the latter gives a power factor of 1 at every angle,
// which looks perfectly reasonable until a reactive load is connected.
#include "meter.hpp"

#include <cmath>
#include <numbers>

#include "alloy_test.hpp"

namespace {
using namespace alloy::lib::meter;

constexpr double kPi = std::numbers::pi;
constexpr int kSamples = 2000;  // whole number of cycles: no leakage to argue about
constexpr int kCycles = 10;

/// Counts for a sine of the given amplitude and phase, biased to mid-scale.
std::int32_t sine_count(int n, double amplitude_counts, double phase, int bias = 2048) {
    const double theta = 2.0 * kPi * kCycles * n / kSamples + phase;
    return static_cast<std::int32_t>(std::lround(bias + amplitude_counts * std::sin(theta)));
}

/// A meter fed a voltage and current sine at `phase` radians apart.
reading measure(double phase, double v_amp = 1000.0, double i_amp = 500.0) {
    accumulator meter{{.volts_per_count = 0.1, .amps_per_count = 0.01,
                       .voltage_offset = 2048, .current_offset = 2048}};
    for (int n = 0; n < kSamples; ++n) {
        meter.update(sine_count(n, v_amp, 0.0), sine_count(n, i_amp, -phase));
    }
    return meter.report();
}

bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

}  // namespace

ALLOY_TEST(meter_rms_of_a_sine_is_amplitude_over_root_two) {
    const auto got = measure(0.0);
    // 1000 counts amplitude x 0.1 V/count = 100 V peak -> 70.71 V rms.
    ALLOY_CHECK(near(got.vrms, 100.0 / std::numbers::sqrt2, 0.2));
    // 500 counts x 0.01 A/count = 5 A peak -> 3.54 A rms.
    ALLOY_CHECK(near(got.irms, 5.0 / std::numbers::sqrt2, 0.02));
    ALLOY_CHECK_EQ(static_cast<int>(got.samples), kSamples);
}

ALLOY_TEST(meter_power_factor_is_cos_phi) {
    // THE test. Real power as Vrms*Irms would give 1.0 at every angle here.
    for (const double phase : {0.0, kPi / 6, kPi / 4, kPi / 3, kPi / 2}) {
        const auto got = measure(phase);
        ALLOY_CHECK(got.power_factor.has_value());
        ALLOY_CHECK(near(*got.power_factor, std::cos(phase), 0.01));
    }
}

ALLOY_TEST(meter_real_power_is_vrms_irms_cos_phi) {
    const auto got = measure(kPi / 3);  // 60 degrees, cos = 0.5
    const double expected = (100.0 / std::numbers::sqrt2) * (5.0 / std::numbers::sqrt2) * 0.5;
    ALLOY_CHECK(near(got.real_power_w, expected, 2.0));
    // Apparent power ignores the angle entirely — that is what makes the ratio
    // meaningful.
    ALLOY_CHECK(near(got.apparent_power_va, (100.0 / std::numbers::sqrt2) *
                                             (5.0 / std::numbers::sqrt2), 2.0));
}

ALLOY_TEST(meter_export_reads_as_negative_power_not_as_an_error) {
    // 180 degrees out of phase: the load is a source. An implementation that
    // took a magnitude would report this as consumption.
    const auto got = measure(kPi);
    ALLOY_CHECK(got.real_power_w < 0.0);
    ALLOY_CHECK(got.power_factor.has_value());
    ALLOY_CHECK(near(*got.power_factor, -1.0, 0.01));
}

ALLOY_TEST(meter_no_load_reports_no_power_factor_instead_of_nan) {
    // The original's failure: divide by an apparent power floored to zero and
    // every unloaded cycle produces NaN.
    accumulator meter{{.volts_per_count = 0.1, .amps_per_count = 0.01,
                       .voltage_offset = 2048, .current_offset = 2048}};
    for (int n = 0; n < 100; ++n) {
        meter.update(2048, 2048);  // dead quiet, exactly at bias
    }
    const auto got = meter.report();
    ALLOY_CHECK(!got.power_factor.has_value());
    ALLOY_CHECK_EQ(got.real_power_w, 0.0);
    ALLOY_CHECK(!std::isnan(got.vrms));
}

ALLOY_TEST(meter_an_empty_window_reports_zeroes_not_a_division_by_zero) {
    accumulator meter{{.volts_per_count = 1.0, .amps_per_count = 1.0}};
    const auto got = meter.report();
    ALLOY_CHECK_EQ(static_cast<int>(got.samples), 0);
    ALLOY_CHECK_EQ(got.vrms, 0.0);
    ALLOY_CHECK(!got.power_factor.has_value());
}

ALLOY_TEST(meter_a_wrong_bias_shows_up_as_dc_rather_than_hiding) {
    // Bias the current 100 counts high and tell the meter the wrong offset.
    accumulator meter{{.volts_per_count = 0.1, .amps_per_count = 0.01,
                       .voltage_offset = 2048, .current_offset = 2048}};
    for (int n = 0; n < kSamples; ++n) {
        meter.update(sine_count(n, 1000.0, 0.0), sine_count(n, 500.0, 0.0, 2148));
    }
    const auto got = meter.report();
    // 100 counts x 0.01 A/count = 1 A of DC, reported and not swept away.
    ALLOY_CHECK(near(got.dc_current, 1.0, 0.01));
    ALLOY_CHECK(near(got.dc_voltage, 0.0, 0.01));
}

ALLOY_TEST(meter_a_small_load_accumulates_energy_instead_of_vanishing) {
    // The original truncated power to int32 before integrating, so anything
    // under about a watt integrated to exactly zero forever. Half a watt for
    // an hour must land at half a watt-hour.
    accumulator meter{{.volts_per_count = 1.0, .amps_per_count = 1.0}};
    for (int hour_tick = 0; hour_tick < 3600; ++hour_tick) {
        meter.update(1, 0);            // v = 1, i = 0 -> and then...
        meter.update(1, 1);            // ...mean of v*i over the pair = 0.5 W
        meter.close_window(1.0);       // one second per window
    }
    ALLOY_CHECK(near(meter.energy_wh(), 0.5, 1e-6));
    ALLOY_CHECK(meter.energy_microjoules() > 0);
}

ALLOY_TEST(meter_closing_a_window_clears_it_but_not_the_total) {
    accumulator meter{{.volts_per_count = 1.0, .amps_per_count = 1.0}};
    meter.update(10, 10);
    meter.close_window(1.0);
    ALLOY_CHECK_EQ(static_cast<int>(meter.samples()), 0);
    const auto energy = meter.energy_microjoules();
    ALLOY_CHECK(energy > 0);

    meter.update(10, 10);
    meter.reset();                       // window only
    ALLOY_CHECK_EQ(meter.energy_microjoules(), energy);
    meter.reset_all();                   // everything
    ALLOY_CHECK_EQ(meter.energy_microjoules(), 0);
}

ALLOY_TEST(meter_energy_survives_a_power_cycle_by_being_seeded) {
    accumulator meter{{.volts_per_count = 1.0, .amps_per_count = 1.0}};
    meter.seed_energy(7'200'000'000);   // 2 Wh, restored from NVM
    ALLOY_CHECK(near(meter.energy_wh(), 2.0, 1e-9));
}

ALLOY_TEST(meter_two_meters_do_not_share_state) {
    // The original held its ISR-called write cursor in a template-member
    // static, so a second instance silently corrupted the first.
    accumulator a{{.volts_per_count = 1.0, .amps_per_count = 1.0}};
    accumulator b{{.volts_per_count = 1.0, .amps_per_count = 1.0}};
    for (int n = 0; n < 10; ++n) {
        a.update(10, 10);
    }
    ALLOY_CHECK_EQ(static_cast<int>(a.samples()), 10);
    ALLOY_CHECK_EQ(static_cast<int>(b.samples()), 0);
}
