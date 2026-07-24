// Fixed-window moving average — O(1) per sample via a running sum. Classic on
// ADC / sensor smoothing. Choose a wider Accum than T when summing many
// integer samples so the running sum can't overflow, e.g.
// moving_average<std::int16_t, 64, std::int32_t>. For float T the default
// Accum = T is fine.

#pragma once

#include <array>
#include <cstddef>

namespace alloy {

template <class T, std::size_t Window, class Accum = T>
class moving_average {
    static_assert(Window >= 1, "moving_average needs window >= 1");
    std::array<T, Window> buf_{};
    Accum sum_{};
    std::size_t idx_{0};
    std::size_t filled_{0};

public:
    using value_type = T;

    // Add a sample; returns the current average.
    T add(T sample) {
        sum_ = static_cast<Accum>(sum_ - static_cast<Accum>(buf_[idx_]) +
                                  static_cast<Accum>(sample));
        buf_[idx_] = sample;
        idx_ = (idx_ + 1 == Window) ? std::size_t{0} : idx_ + 1;
        if (filled_ < Window) {
            ++filled_;
        }
        return value();
    }

    // Average over the samples seen so far (T{} before the first sample).
    [[nodiscard]] T value() const {
        if (filled_ == 0) {
            return T{};
        }
        return static_cast<T>(sum_ / static_cast<Accum>(filled_));
    }

    [[nodiscard]] bool ready() const { return filled_ == Window; }

    void reset() {
        buf_ = {};
        sum_ = Accum{};
        idx_ = 0;
        filled_ = 0;
    }
};

}  // namespace alloy
