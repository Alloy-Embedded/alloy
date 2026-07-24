// alloy::async::waiter_slot — the reusable, race-free single-waiter park/wake
// mechanism every driver awaiter builds on, so none re-derives it (and so none
// re-introduces the lost-wakeup race).
//
// A driver awaiter composes this with ITS OWN readiness source — an event flag,
// a byte ring, a DMA-complete latch. park() registers the task under a mask and
// re-checks readiness inside that mask, so a wake that races into the
// await_ready→await_suspend window is never lost. wake() (from the ISR) hands the
// parked task to the executor for a thread-context resume. Single owner: a
// second concurrent parker on one slot traps (a shared peripheral needs a queue,
// not two waiters).

#pragma once

#include <coroutine>

#include "alloy/arch/irq.hpp"
#include "alloy/async/executor.hpp"

namespace alloy::async {

class waiter_slot {
    std::coroutine_handle<> waiter_{};

public:
    // The body of a driver awaiter's await_suspend. `ready` is a predicate
    // evaluated UNDER the mask; if it is already true, returns false (do not
    // suspend — the event raced in). Returns true when the task is parked.
    template <class Ready>
    bool park(std::coroutine_handle<> h, Ready&& ready) {
        const arch::irq_state s = arch::irq_save();
        if (ready()) {
            arch::irq_restore(s);
            return false;  // ready in the window — resume immediately
        }
        if (waiter_) {
            arch::irq_restore(s);
            __builtin_trap();  // a second waiter on one slot — not allowed
        }
        waiter_ = h;
        arch::irq_restore(s);
        return true;
    }

    // From ISR context: wake the parked task through the executor (which resumes
    // it later in thread context). No-op when nobody is parked.
    void wake() {
        const arch::irq_state s = arch::irq_save();
        const auto w = waiter_;
        waiter_ = {};
        arch::irq_restore(s);
        if (w) {
            executor_core::the().schedule(w);
        }
    }

    // Introspection for tests/drivers (racy unmasked peek — advisory only).
    [[nodiscard]] bool has_waiter() const { return static_cast<bool>(waiter_); }
};

}  // namespace alloy::async
