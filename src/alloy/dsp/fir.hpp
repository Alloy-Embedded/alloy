// Finite impulse response (FIR) filter — a fixed set of taps convolved with the
// input. Linear phase, always stable. You supply the taps (from a design tool,
// or boxcar() for a plain moving average).
//
//   alloy::dsp::fir<8> f{alloy::dsp::boxcar<8>()};   // 8-point moving average
//   out = f.process(sample());
//
// State is a circular buffer, so process() is one pass over N taps with no
// sample shuffling. float throughout.

#pragma once

#include <array>
#include <cstddef>

namespace alloy::dsp {

template <std::size_t N>
class fir {
    static_assert(N >= 1, "fir needs at least one tap");

public:
    fir() = default;
    explicit fir(const std::array<float, N>& taps) : taps_(taps) {}

    void set(const std::array<float, N>& taps) { taps_ = taps; }
    void reset() {
        z_.fill(0.0f);
        pos_ = 0;
    }

    float process(float x) {
        z_[pos_] = x;
        float y = 0.0f;
        std::size_t k = pos_;  // newest sample pairs with taps_[0]
        for (std::size_t i = 0; i < N; ++i) {
            y += taps_[i] * z_[k];
            k = (k == 0) ? N - 1 : k - 1;
        }
        pos_ = (pos_ + 1) % N;
        return y;
    }

private:
    std::array<float, N> taps_{};
    std::array<float, N> z_{};
    std::size_t pos_{0};
};

// Equal-weight taps: an N-point moving average expressed as an FIR.
template <std::size_t N>
constexpr std::array<float, N> boxcar() {
    std::array<float, N> taps{};
    for (std::size_t i = 0; i < N; ++i) {
        taps[i] = 1.0f / static_cast<float>(N);
    }
    return taps;
}

}  // namespace alloy::dsp
