// alloy::async::periodic — a drift-free cadence for coroutine tasks.
//
//   alloy::async::periodic every_100ms{100ms};
//   for (;;) { co_await every_100ms.next(); sample(); }
//
// WHY delay() IS THE WRONG TOOL FOR A CADENCE, stated with the arithmetic:
// `co_await delay(100ms)` computes its deadline as now + 100 — so every
// microsecond between the intended wake and the moment the task actually got
// the CPU is ADDED to the period, forever. A task that runs 2 ms late every
// cycle ticks at 102 ms, and two tasks started together walk apart. This type
// keeps a PHASE ACCUMULATOR instead: each next() advances the deadline by
// exactly one interval from the previous deadline (the same rule as
// util/timer.hpp's software_timer::poll), so lateness changes when one wake
// happens, never when the next one is due.
//
// CATCH-UP, not pile-up: when the phase has fallen more than one whole
// interval behind (the task was starved, or a debugger held the core),
// next() realigns the phase to the CURRENT cycle boundary — one wake, not a
// burst of stale ones, and still on the original grid. The original epoch is
// the moment the `periodic` was constructed, which is therefore the anchor
// every subsequent wake is an exact multiple of an interval away from.
//
// Thread context only, like every awaiter here; the deadline node lives on
// the coroutine frame via the returned awaiter (no heap), and a task
// destroyed mid-wait unlinks itself exactly as delay() does.

#pragma once

#include <chrono>
#include <coroutine>
#include <cstdint>

#include "alloy/async/executor.hpp"
#include "alloy/time.hpp"

namespace alloy::async {

class periodic {
    std::uint32_t interval_ms_;
    std::uint32_t phase_ms_;  // the NEXT deadline on the grid

public:
    explicit periodic(std::chrono::milliseconds interval)
        : interval_ms_(interval.count() > 0
                           ? static_cast<std::uint32_t>(interval.count())
                           : 1u),
          phase_ms_(alloy::uptime_ms() + interval_ms_) {}

    class awaiter {
        std::uint32_t deadline_ms_;
        timer_node node_{};

    public:
        explicit awaiter(std::uint32_t deadline_ms) : deadline_ms_(deadline_ms) {}
        ~awaiter() {
            if (node_.armed) {
                executor_core::the().cancel_timer(node_);
            }
        }
        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;

        // Already at/past the deadline: resume immediately, no suspend. The
        // wrap-safe signed read matches the executor's own timer test.
        bool await_ready() const noexcept {
            return static_cast<std::int32_t>(alloy::uptime_ms() - deadline_ms_) >= 0;
        }
        void await_suspend(std::coroutine_handle<> h) noexcept {
            node_.waiter = h;
            node_.deadline_ms = deadline_ms_;
            executor_core::the().arm_timer(node_);
        }
        void await_resume() const noexcept {}
    };

    // The next wake on the grid. Advances the phase by EXACTLY one interval;
    // when more than a whole interval behind, realigns to the current cycle
    // boundary (catch-up, not pile-up) — still a multiple of the interval
    // from the construction epoch.
    [[nodiscard]] awaiter next() {
        const std::uint32_t now = alloy::uptime_ms();
        const auto behind = static_cast<std::int32_t>(now - phase_ms_);
        if (behind >= static_cast<std::int32_t>(interval_ms_)) {
            const std::uint32_t whole =
                static_cast<std::uint32_t>(behind) / interval_ms_;
            phase_ms_ += whole * interval_ms_;
        }
        const std::uint32_t deadline = phase_ms_;
        phase_ms_ += interval_ms_;
        return awaiter{deadline};
    }

    // The deadline the NEXT next() will target — a test/diagnostic window.
    [[nodiscard]] std::uint32_t phase_ms() const { return phase_ms_; }
};

}  // namespace alloy::async
