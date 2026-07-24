// Cooperative, heapless, RTOS-free scheduler — turns the generated peripheral
// HAL into an application skeleton. Tasks are a fixed compile-time table run on
// a single stack: each fires on its own period and/or when an interrupt signals
// it. Timing is time-triggered polling over alloy::uptime_ms() (wrap-safe);
// events are a per-task bitmask set from ISR context under the portable
// arch::irq_save critical section. No preemption, no context switch, no heap.
//
// run_once(now) is the PURE superstep — feed it a fake clock and fake signals
// to unit-test application logic on the host; run() is the on-target loop.

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "alloy/arch/irq.hpp"
#include "alloy/time.hpp"
#include "alloy/util/timer.hpp"

namespace alloy {

// A task body. ctx is the user pointer passed to add(); events carries the bits
// signalled since the previous run (0 on a purely periodic wake).
using task_fn = void (*)(void* ctx, std::uint32_t events);

template <std::size_t MaxTasks>
class scheduler {
    struct slot {
        task_fn fn{nullptr};
        void* ctx{nullptr};
        software_timer<std::uint32_t> timer{};
        bool periodic{false};
        volatile std::uint32_t events{0};
    };
    slot slots_[MaxTasks]{};
    std::size_t count_{0};

public:
    // Register a task. period == 0ms → event-driven only (runs when signalled).
    // Returns the task index, or MaxTasks when the table is full.
    std::size_t add(task_fn fn, void* ctx, std::chrono::milliseconds period) {
        if (count_ >= MaxTasks) {
            return MaxTasks;
        }
        const std::size_t i = count_++;
        slots_[i].fn = fn;
        slots_[i].ctx = ctx;
        slots_[i].periodic = period.count() > 0;
        slots_[i].timer.set_interval(period);
        slots_[i].timer.reset(0);
        return i;
    }

    // Signal a task from any context (ISR-safe). ORs in `bits`; the task's next
    // run receives them.
    void signal(std::size_t task, std::uint32_t bits) {
        if (task >= count_) {
            return;
        }
        const auto s = arch::irq_save();
        slots_[task].events |= bits;
        arch::irq_restore(s);
    }

    // One scheduling superstep against a single clock snapshot: run each task
    // whose period elapsed or that has pending events. Pure — inject `now`.
    void run_once(std::uint32_t now) {
        for (std::size_t i = 0; i < count_; ++i) {
            slot& t = slots_[i];
            const auto s = arch::irq_save();
            const std::uint32_t ev = t.events;
            t.events = 0;
            arch::irq_restore(s);
            const bool timed = t.periodic && t.timer.poll(now);
            if (ev != 0 || timed) {
                t.fn(t.ctx, ev);
            }
        }
    }

    // On-target forever loop: one superstep per tick, idle 1 ms in between.
    [[noreturn]] void run() {
        const std::uint32_t base = alloy::uptime_ms();
        for (std::size_t i = 0; i < count_; ++i) {
            slots_[i].timer.reset(base);
        }
        for (;;) {
            run_once(alloy::uptime_ms());
            alloy::sleep_for(std::chrono::milliseconds{1});
        }
    }

    [[nodiscard]] std::size_t size() const { return count_; }
};

}  // namespace alloy
