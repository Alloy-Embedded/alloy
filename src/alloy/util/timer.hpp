// Wrap-around-safe software timer over an unsigned monotonic tick. The tick is
// INJECTED (expired(now) / poll(now)) so the same timer is host-testable with a
// fake clock and carries no arch dependency — on target, feed it
// alloy::uptime_ms(). Correctness rests on unsigned modular subtraction:
// (now - start) >= interval stays true across a counter wrap. Keep intervals
// under half the tick range — < ~24 days on the 1 kHz Cortex-M ms tick, and
// < ~26 s on the ESP32 CCOUNT-derived ms (its documented v1 wrap).

#pragma once

#include <chrono>
#include <cstdint>

namespace alloy {

template <class Tick = std::uint32_t>
class software_timer {
    static_assert(Tick(-1) > Tick(0), "software_timer Tick must be an unsigned type");
    Tick start_{0};
    Tick interval_{0};

public:
    constexpr software_timer() = default;
    constexpr explicit software_timer(Tick interval) : interval_(interval) {}
    explicit software_timer(std::chrono::milliseconds interval)
        : interval_(static_cast<Tick>(interval.count())) {}

    void set_interval(Tick interval) { interval_ = interval; }
    void set_interval(std::chrono::milliseconds interval) {
        interval_ = static_cast<Tick>(interval.count());
    }
    [[nodiscard]] Tick interval() const { return interval_; }

    // (Re)start the interval from `now`.
    void reset(Tick now) { start_ = now; }

    // Has the interval elapsed since start? Wrap-safe.
    [[nodiscard]] bool expired(Tick now) const { return (now - start_) >= interval_; }

    // Periodic poll: on elapse, advance the phase by exactly one interval (not
    // to `now`, so the cadence never drifts) and report the tick.
    bool poll(Tick now) {
        if ((now - start_) >= interval_) {
            start_ += interval_;
            return true;
        }
        return false;
    }
};

}  // namespace alloy
