// Unit tests for the DSP filters (alloy/dsp/biquad.hpp, fir.hpp). Filters are
// pure math, so this IS the validation: exact DC/Nyquist gains derived from the
// designed coefficients, plus a few time-domain sanity runs (a DC step settles
// to the DC gain, a notch kills its center tone, an FIR's impulse response is
// its taps).

#include <array>
#include <cmath>

#include "alloy/dsp/biquad.hpp"
#include "alloy/dsp/fir.hpp"
#include "alloy_test.hpp"

using namespace alloy::dsp;

namespace {
// |H(z)| at DC (z=1) and Nyquist (z=-1) from the difference-equation coeffs.
float dc_gain(const biquad_coeffs& c) {
    return (c.b0 + c.b1 + c.b2) / (1.0f + c.a1 + c.a2);
}
float nyquist_gain(const biquad_coeffs& c) {
    return (c.b0 - c.b1 + c.b2) / (1.0f - c.a1 + c.a2);
}
bool near(float a, float b, float tol = 0.02f) { return std::fabs(a - b) < tol; }
}  // namespace

ALLOY_TEST(biquad_lowpass_passes_dc_blocks_nyquist) {
    const auto c = lowpass(1000.0f, 48000.0f);
    ALLOY_CHECK(near(dc_gain(c), 1.0f));
    ALLOY_CHECK(near(nyquist_gain(c), 0.0f));
}

ALLOY_TEST(biquad_highpass_blocks_dc_passes_nyquist) {
    const auto c = highpass(1000.0f, 48000.0f);
    ALLOY_CHECK(near(dc_gain(c), 0.0f));
    ALLOY_CHECK(near(nyquist_gain(c), 1.0f));
}

ALLOY_TEST(biquad_notch_passes_dc_and_nyquist) {
    const auto c = notch(1000.0f, 48000.0f);
    ALLOY_CHECK(near(dc_gain(c), 1.0f));
    ALLOY_CHECK(near(nyquist_gain(c), 1.0f));
}

ALLOY_TEST(biquad_bandpass_blocks_dc_and_nyquist) {
    const auto c = bandpass(1000.0f, 48000.0f);
    ALLOY_CHECK(near(dc_gain(c), 0.0f));
    ALLOY_CHECK(near(nyquist_gain(c), 0.0f));
}

ALLOY_TEST(biquad_default_is_passthrough) {
    biquad f;
    ALLOY_CHECK(near(f.process(0.5f), 0.5f));
    ALLOY_CHECK(near(f.process(-0.3f), -0.3f));
    ALLOY_CHECK(near(f.process(1.25f), 1.25f));
}

ALLOY_TEST(biquad_lowpass_step_settles_to_input) {
    biquad f{lowpass(1000.0f, 48000.0f)};
    float y = 0.0f;
    for (int i = 0; i < 3000; ++i) {
        y = f.process(1.0f);  // unit DC step
    }
    ALLOY_CHECK(near(y, 1.0f));
}

ALLOY_TEST(biquad_notch_attenuates_center_tone) {
    const float fs = 48000.0f;
    const float fc = 1000.0f;
    biquad f{notch(fc, fs, 5.0f)};
    float peak = 0.0f;
    for (int i = 0; i < 6000; ++i) {
        const float x = std::sin(2.0f * detail::kPi * fc / fs * static_cast<float>(i));
        const float y = f.process(x);
        if (i > 5000) {  // steady state only
            peak = std::fabs(y) > peak ? std::fabs(y) : peak;
        }
    }
    ALLOY_CHECK(peak < 0.2f);  // a 1.0-amplitude tone at fc knocked well down
}

ALLOY_TEST(biquad_cascade_matches_two_sections) {
    biquad_cascade<2> casc;
    const auto c = lowpass(1000.0f, 48000.0f);
    casc[0].set(c);
    casc[1].set(c);
    float y = 0.0f;
    for (int i = 0; i < 3000; ++i) {
        y = casc.process(1.0f);
    }
    ALLOY_CHECK(near(y, 1.0f));  // two DC-unity sections still pass DC
}

ALLOY_TEST(fir_boxcar_averages_dc_to_unity) {
    fir<8> f{boxcar<8>()};
    float y = 0.0f;
    for (int i = 0; i < 16; ++i) {
        y = f.process(1.0f);
    }
    ALLOY_CHECK(near(y, 1.0f));
}

ALLOY_TEST(fir_impulse_response_equals_taps) {
    const std::array<float, 3> taps{0.25f, 0.5f, 0.25f};
    fir<3> f{taps};
    ALLOY_CHECK(near(f.process(1.0f), 0.25f));  // taps[0] pairs with newest
    ALLOY_CHECK(near(f.process(0.0f), 0.5f));
    ALLOY_CHECK(near(f.process(0.0f), 0.25f));
    ALLOY_CHECK(near(f.process(0.0f), 0.0f));
}
