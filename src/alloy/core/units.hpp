// Typed frequency + compile-time rate tolerance.
//
// Frequencies (clocks, baud, bus speeds) are the one physical unit alloy adds
// on top of std::chrono (which keeps owning time). `frequency` is a strong
// type over u32 Hz — a bare integer at runtime, but distinct from a
// chrono::duration, so `64_MHz + 1ms` never compiles by accident.
//
// The payoff is `rate_check`: given a kernel clock and a requested rate, a
// driver's OWN divisor formula (hand-written behavior, single source of truth
// with enable()) computes the ACHIEVED rate, and a static_assert rejects it at
// COMPILE TIME when it drifts past tolerance — the class of bug that let the
// SAME70 baud diverge silently. The requested/achieved/tolerance numbers ride
// out in the failing `rate_check<…>` template arguments (C++23 static_assert
// messages are plain literals, so the values travel as template parameters).

#pragma once

#include <chrono>
#include <cstdint>
#include <limits>

namespace alloy {

// Strong type over an integer Hz count. Structural (public member, defaulted
// comparisons) so it works as a non-type template parameter: open_checked<115'200_baud>().
struct frequency {
    std::uint32_t hz_ = 0;

    constexpr frequency() = default;
    explicit constexpr frequency(std::uint32_t hz) noexcept : hz_(hz) {}

    [[nodiscard]] constexpr std::uint32_t hz() const noexcept { return hz_; }

    friend constexpr bool operator==(frequency, frequency) = default;
    friend constexpr auto operator<=>(frequency, frequency) = default;
};

namespace detail {
// Only declared: naming it in a constant expression is a hard error whose
// diagnostic prints THIS name — the readable reason a literal is out of range.
std::uint32_t frequency_literal_exceeds_u32_hz();

consteval std::uint32_t narrow_hz(unsigned long long v) {
    // kHz/MHz literals multiply in 64-bit; a frequency past ~4.29 GHz cannot
    // be a u32 Hz, so reaching the else makes the literal ill-formed.
    return v <= std::numeric_limits<std::uint32_t>::max()
               ? static_cast<std::uint32_t>(v)
               : frequency_literal_exceeds_u32_hz();
}
}  // namespace detail

namespace literals {
consteval frequency operator""_Hz(unsigned long long v) { return frequency{detail::narrow_hz(v)}; }
consteval frequency operator""_kHz(unsigned long long v) { return frequency{detail::narrow_hz(v * 1000ull)}; }
consteval frequency operator""_MHz(unsigned long long v) {
    return frequency{detail::narrow_hz(v * 1000ull * 1000ull)};
}
// A baud rate is a frequency (symbols/s) — same type, clearer at the call site.
consteval frequency operator""_baud(unsigned long long v) { return frequency{detail::narrow_hz(v)}; }
}  // namespace literals

// Bridge to chrono: the period of one cycle. Time stays chrono — you only cross
// here when a peripheral wants a period instead of a rate.
[[nodiscard]] constexpr std::chrono::nanoseconds period(frequency f) {
    return std::chrono::nanoseconds{f.hz() ? 1000ull * 1000ull * 1000ull / f.hz() : 0ull};
}

// True when `achieved` is within `tol_permille` (parts per thousand; 20 = 2%)
// of `target`. u64 cross-multiply avoids overflow and division rounding.
[[nodiscard]] constexpr bool within_permille(frequency target, frequency achieved,
                                             std::uint32_t tol_permille) {
    const std::uint64_t t = target.hz();
    const std::uint64_t a = achieved.hz();
    const std::uint64_t diff = t > a ? t - a : a - t;
    return diff * 1000ull <= t * std::uint64_t{tol_permille};
}

// Compile-time gate. Instantiating it with an out-of-tolerance pair fires the
// static_assert AND surfaces the numbers: the compiler prints
// `rate_check<115200, 96000, 20>` so the requested/achieved/tolerance are visible.
template <std::uint32_t RequestedHz, std::uint32_t AchievedHz, std::uint32_t TolPermille>
struct rate_check {
    static_assert(within_permille(frequency{RequestedHz}, frequency{AchievedHz}, TolPermille),
                  "requested rate is not achievable within tolerance on this kernel clock — "
                  "see rate_check<requested_hz, achieved_hz, tol_permille> in the diagnostic; "
                  "fix the clock profile or widen the tolerance");
};

}  // namespace alloy
