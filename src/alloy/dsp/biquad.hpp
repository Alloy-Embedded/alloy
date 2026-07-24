// Second-order IIR (biquad) filter + RBJ-cookbook coefficient designers.
//
//   auto lp = alloy::dsp::lowpass(1000.f, 48000.f);  // 1 kHz cutoff @ 48 kHz
//   alloy::dsp::biquad f{lp};
//   for (;;) out = f.process(sample());              // one MAC-heavy call/sample
//
// You give a cutoff/center frequency and Q; the designer returns the five
// normalized coefficients (RBJ "Audio EQ Cookbook"). Chain sections with
// biquad_cascade for higher-order responses (two Butterworth low-passes = 4th
// order). Direct Form II Transposed: two state words, good round-off behavior.
// float throughout — on a soft-float core process() is a handful of library
// multiplies, so keep it off the very hottest M0+ paths (or mark it
// ALLOY_FASTCODE).

#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace alloy::dsp {

// Normalized biquad coefficients (a0 == 1). Default is a pass-through.
struct biquad_coeffs {
    float b0{1.0f};
    float b1{0.0f};
    float b2{0.0f};
    float a1{0.0f};
    float a2{0.0f};
};

class biquad {
public:
    biquad() = default;
    explicit biquad(const biquad_coeffs& c) : c_(c) {}

    void set(const biquad_coeffs& c) { c_ = c; }
    void reset() { z1_ = z2_ = 0.0f; }

    // Push one input sample, get one output sample.
    float process(float x) {
        const float y = c_.b0 * x + z1_;
        z1_ = c_.b1 * x - c_.a1 * y + z2_;
        z2_ = c_.b2 * x - c_.a2 * y;
        return y;
    }

private:
    biquad_coeffs c_{};
    float z1_{0.0f};
    float z2_{0.0f};
};

namespace detail {
constexpr float kPi = 3.14159265358979323846f;

inline biquad_coeffs normalize(float b0, float b1, float b2,
                               float a0, float a1, float a2) {
    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

struct rbj {
    float cw;     // cos(w0)
    float alpha;  // sin(w0) / (2Q)
};

inline rbj terms(float fc, float fs, float q) {
    const float w0 = 2.0f * kPi * fc / fs;
    const float sw = std::sin(w0);
    return {std::cos(w0), sw / (2.0f * q)};
}
}  // namespace detail

constexpr float butterworth_q = 0.70710678f;  // Q for a maximally-flat response

inline biquad_coeffs lowpass(float fc, float fs, float q = butterworth_q) {
    const auto t = detail::terms(fc, fs, q);
    const float b1 = 1.0f - t.cw;
    return detail::normalize(b1 / 2.0f, b1, b1 / 2.0f,
                             1.0f + t.alpha, -2.0f * t.cw, 1.0f - t.alpha);
}

inline biquad_coeffs highpass(float fc, float fs, float q = butterworth_q) {
    const auto t = detail::terms(fc, fs, q);
    const float b0 = (1.0f + t.cw) / 2.0f;
    return detail::normalize(b0, -(1.0f + t.cw), b0,
                             1.0f + t.alpha, -2.0f * t.cw, 1.0f - t.alpha);
}

// Constant 0 dB peak-gain band-pass.
inline biquad_coeffs bandpass(float fc, float fs, float q = butterworth_q) {
    const auto t = detail::terms(fc, fs, q);
    return detail::normalize(t.alpha, 0.0f, -t.alpha,
                             1.0f + t.alpha, -2.0f * t.cw, 1.0f - t.alpha);
}

// Band-reject (notch) — kills a narrow band (e.g. 50/60 Hz mains hum).
inline biquad_coeffs notch(float fc, float fs, float q = butterworth_q) {
    const auto t = detail::terms(fc, fs, q);
    return detail::normalize(1.0f, -2.0f * t.cw, 1.0f,
                             1.0f + t.alpha, -2.0f * t.cw, 1.0f - t.alpha);
}

// A cascade of N biquad sections: process() runs them in series, so N low-passes
// give a steeper (2N-order) roll-off. Assign sections via operator[].
template <std::size_t N>
class biquad_cascade {
public:
    biquad& operator[](std::size_t i) { return s_[i]; }
    const biquad& operator[](std::size_t i) const { return s_[i]; }

    void reset() {
        for (auto& s : s_) {
            s.reset();
        }
    }
    float process(float x) {
        for (auto& s : s_) {
            x = s.process(x);
        }
        return x;
    }

private:
    std::array<biquad, N> s_{};
};

}  // namespace alloy::dsp
