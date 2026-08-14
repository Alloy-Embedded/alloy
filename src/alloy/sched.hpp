// Cooperative, heapless, RTOS-free scheduler — turns the generated peripheral
// HAL into an application skeleton. Tasks are a fixed compile-time table run on
// a single stack: each fires on its own period and/or when an interrupt signals
// it. Timing is time-triggered polling over an injected tick; events are a
// per-task bitmask set from ISR context under the portable arch::irq_save
// critical section. No preemption, no context switch, no heap.
//
// run_once(now) is the PURE superstep — feed it a fake clock and fake signals
// to unit-test application logic on the host; run() is the on-target loop.
// That purity is the load-bearing property of this file: the scheduler NEVER
// reads a clock itself, so every dispatch decision is a function of the
// arguments it was handed.
//
// ---------------------------------------------------------------------------
// THE TWO DISPATCH DISCIPLINES, and why the second exists.
//
// The default, `sched_dispatch::run_all_ready`, runs every task whose period
// elapsed or that has pending events, in table order, every superstep — the
// natural discipline for a housekeeping loop, and exactly what this scheduler
// always did. THE DISCIPLINE IS A TEMPLATE PARAMETER, not a runtime switch,
// for the only reason that matters at -Os: the path you did not pick must
// COMPILE AWAY. Measured on cortex-m0plus, a default-discipline user's
// codegen is the historical scheduler's, byte for byte of behavior and
// within noise of its size; the budgeted machinery (readiness latch, class
// field, budget bookkeeping) exists only in images that name it.
//
// `sched_dispatch::drain_budgeted` is the discipline of slot-scheduled control
// firmware, where the worst case of any tick must be bounded BY CONSTRUCTION:
// per distinct value of `now` (NOT per run_once call — a caller may invoke
// run_once many times per tick), at most ONE deferred-class ready task is
// dispatched, chosen by strict table order, while always-class tasks run
// unconditionally on their period. A task that could not run stays READY and
// is drained on a later tick, still in table order. When a coincident period
// boundary makes eight tasks due at once they are therefore served on eight
// CONSECUTIVE ticks, in priority order — which bounds the tick at
// (always-class work + one deferred task) AND gives cross-task data flow a
// deterministic stagger ("task A of this boundary always ran before task B").
// Table order IS the priority; there is no separate number to disagree with it.
//
// Missed ticks do NOT accumulate dispatch budget: readiness is a latch, not a
// counter, so a thread that lagged three ticks drains one task on the tick it
// finally sees, exactly as a flag-based slot firmware would.
//
// ---------------------------------------------------------------------------
// THE ISR HALF. `tick_from_isr()` is the one entry an application calls from
// its hardware timer interrupt: it advances the internal tick count and
// nothing else — every task body runs in THREAD context from run_once(). The
// no-argument run_once() overload consumes that internal count, which is what
// makes `tick_from_isr` + `while (true) run_once();` the whole on-target
// arrangement for a base tick faster than the 1 kHz SysTick. The counter is a
// single 32-bit cell: naturally atomic to read on every core alloy targets.
//
// THE TICK UNIT belongs to the caller. `Tick` is whatever unsigned count the
// application advances — SysTick milliseconds, a 10 kHz timer's 100 µs, one
// per control-loop ADC batch. The chrono add() overload is a convenience for
// the common case where the tick IS the millisecond; on any other base state
// periods in ticks, because nothing here can know what a tick is worth.
//
// THE OBSERVER is the instrumentation seam: dispatch_begin/dispatch_end around
// every task body, idle_pass when a superstep dispatched nothing. The default
// `null_dispatch_observer` compiles to NOTHING — the hooks vanish behind
// `if constexpr`, measured at -Os. The scheduler ships the SEAM, never
// statistics: it hands the observer the task index and the wake reason, and
// the observer takes its own timestamps — filtering formulas, averages and
// reporting units are application contract, deliberately outside this file.
//
// THE IDLE LIST is likewise paid for only when sized: `IdleSlots` defaults to
// zero, and a zero-capacity list is no storage and no code. Entries run
// round-robin, ONE per superstep that dispatched nothing — the classic
// "spend spare cycles on comms polling" arrangement.

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "alloy/arch/irq.hpp"
#include "alloy/time.hpp"
#include "alloy/util/timer.hpp"

namespace alloy {

// A task body. ctx is the user pointer passed to add(); events carries the bits
// signalled since the previous run (0 on a purely periodic wake).
using task_fn = void (*)(void* ctx, std::uint32_t events);

// Why a dispatched task was ready. A task can be both.
namespace wake {
inline constexpr std::uint32_t timer = std::uint32_t{1} << 0;
inline constexpr std::uint32_t event = std::uint32_t{1} << 1;
}  // namespace wake

// The no-cost default observer. A real observer provides the same three
// members; it takes its OWN timestamps in them — the scheduler stays pure.
struct null_dispatch_observer {
    void dispatch_begin(std::size_t /*task*/, std::uint32_t /*wake_reason*/) {}
    void dispatch_end(std::size_t /*task*/) {}
    void idle_pass() {}
};

// The dispatch discipline — a TEMPLATE parameter so the unused path folds
// away (see the header note).
enum class sched_dispatch : std::uint8_t { run_all_ready, drain_budgeted };

// `always` runs on every superstep its period has elapsed, whatever the
// budget — the base-rate housekeeping class. `deferred` is subject to the
// drain budget under drain_budgeted (and identical to `always` under the
// default discipline).
enum class sched_class : std::uint8_t { deferred, always };

template <std::size_t MaxTasks, class Tick = std::uint32_t,
          class Observer = null_dispatch_observer,
          sched_dispatch Dispatch = sched_dispatch::run_all_ready,
          std::size_t IdleSlots = 0>
class scheduler {
    static_assert(Tick(-1) > Tick(0), "the scheduler Tick must be unsigned");

    static constexpr bool kBudgeted =
        Dispatch == sched_dispatch::drain_budgeted;
    static constexpr bool kObserved =
        !std::is_same_v<Observer, null_dispatch_observer>;

public:
    using task_class = sched_class;  // the add() parameter's spelling

    // Register a task, period in TICKS. period == 0 -> event-driven only.
    // Returns the task index — which under drain_budgeted is also its
    // PRIORITY (table order) — or MaxTasks when the table is full.
    std::size_t add(task_fn fn, void* ctx, Tick period_ticks,
                    task_class cls = task_class::deferred) {
        if (count_ >= MaxTasks) {
            return MaxTasks;
        }
        const std::size_t i = count_++;
        slots_[i].fn = fn;
        slots_[i].ctx = ctx;
        slots_[i].periodic = period_ticks > Tick{0};
        slots_[i].timer.set_interval(period_ticks);
        slots_[i].timer.reset(Tick{0});
        if constexpr (kBudgeted) {
            slots_[i].cls = cls;
        } else {
            (void)cls;  // one discipline: the class distinction has no meaning
        }
        return i;
    }

    // Convenience for the common case where the tick IS the millisecond (the
    // 1 kHz SysTick base). On any other base, state the period in ticks.
    std::size_t add(task_fn fn, void* ctx, std::chrono::milliseconds period,
                    task_class cls = task_class::deferred) {
        return add(fn, ctx, static_cast<Tick>(period.count()), cls);
    }

    // Attach the instrumentation observer. Only spellable when Observer is
    // not the null default — with the default there is nothing to attach and
    // the hook sites below fold away.
    void set_observer(Observer& o)
        requires (kObserved)
    {
        obs_ = &o;
    }

    // The idle list (only when IdleSlots > 0 — a zero-capacity list is no
    // storage and no code). Returns false when the list is full.
    bool add_idle(task_fn fn, void* ctx)
        requires (IdleSlots > 0)
    {
        if (idle_count_ >= IdleSlots) {
            return false;
        }
        idle_[idle_count_++] = {fn, ctx};
        return true;
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

    // The ISR half: advance the internal tick. Call at the base rate from a
    // hardware timer interrupt; every task body still runs in THREAD context,
    // from run_once(). Single 32-bit store — atomic on every alloy core.
    void tick_from_isr() { isr_ticks_ = isr_ticks_ + Tick{1}; }

    // The internal tick the ISR half advances (also the `now` the no-argument
    // run_once() consumes).
    [[nodiscard]] Tick isr_ticks() const { return isr_ticks_; }

    // One scheduling superstep against a single clock snapshot. Pure — inject
    // `now`. Returns the number of task bodies dispatched (idle entries do
    // not count).
    std::size_t run_once(Tick now) {
        std::size_t ran = 0;
        if constexpr (!kBudgeted) {
            // THE HISTORICAL BODY, verbatim in behavior: walk the table once,
            // snapshot-and-clear each task's events, run it if signalled or
            // due. A task dispatched earlier in the table may signal a later
            // one and the later one runs THE SAME superstep — pinned.
            for (std::size_t i = 0; i < count_; ++i) {
                slot& t = slots_[i];
                const auto st = arch::irq_save();
                const std::uint32_t ev = t.events;
                t.events = 0;
                arch::irq_restore(st);
                const bool timed = t.periodic && t.timer.poll(now);
                if (ev != 0 || timed) {
                    dispatch_one(i, ev,
                                 (timed ? wake::timer : 0u) |
                                     (ev != 0 ? wake::event : 0u));
                    ++ran;
                }
            }
        } else {
            // 1. LATCH readiness. poll() advances the phase by exactly one
            //    interval per elapse (util/timer.hpp), so a task that cannot
            //    run this superstep stays ready and its cadence never drifts.
            //    The latch is a bool, not a counter: missed periods coalesce,
            //    exactly as flag-based slot firmware behaves. (A mid-superstep
            //    signal is therefore NEXT-superstep news here, by design —
            //    deferral makes arrival order superstep-granular.)
            for (std::size_t i = 0; i < count_; ++i) {
                slot& t = slots_[i];
                if (t.periodic && t.timer.poll(now)) {
                    t.ready = true;
                    t.wake_reason |= wake::timer;
                }
                if (t.events != 0) {  // benign racy read; snapshot at dispatch
                    t.ready = true;
                    t.wake_reason |= wake::event;
                }
            }
            // 2. always-class first, every one that is ready: the base-rate
            //    work is never budgeted (it is the budget's denominator).
            for (std::size_t i = 0; i < count_; ++i) {
                if (slots_[i].ready && slots_[i].cls == task_class::always) {
                    dispatch_latched(i);
                    ++ran;
                }
            }
            // 3. ...then AT MOST ONE deferred task per distinct `now`, in
            //    table order. `have_deferred_now_` (not a sentinel value) is
            //    what makes tick zero a first-class budget holder.
            const bool budget_open =
                !have_deferred_now_ || last_deferred_now_ != now;
            if (budget_open) {
                for (std::size_t i = 0; i < count_; ++i) {
                    if (slots_[i].ready &&
                        slots_[i].cls == task_class::deferred) {
                        dispatch_latched(i);
                        ++ran;
                        last_deferred_now_ = now;
                        have_deferred_now_ = true;
                        break;
                    }
                }
            }
        }

        // IDLE: one round-robin entry per empty superstep.
        if (ran == 0) {
            if constexpr (kObserved) {
                if (obs_ != nullptr) {
                    obs_->idle_pass();
                }
            }
            if constexpr (IdleSlots > 0) {
                if (idle_count_ > 0) {
                    idle_entry& e = idle_[idle_next_];
                    idle_next_ = (idle_next_ + 1) % idle_count_;
                    e.fn(e.ctx, 0);
                }
            }
        }
        return ran;
    }

    // The no-argument superstep: consume the tick the ISR half advanced.
    std::size_t run_once() { return run_once(isr_ticks_); }

    // On-target forever loop over the injected-ms base: one superstep per
    // tick. Sleeps only when the superstep dispatched nothing AND there is no
    // idle work — an idle list takes precedence over the nap, by design: the
    // application declared it had something better to do with the slack.
    [[noreturn]] void run() {
        const std::uint32_t base = alloy::uptime_ms();
        for (std::size_t i = 0; i < count_; ++i) {
            slots_[i].timer.reset(static_cast<Tick>(base));
        }
        for (;;) {
            const std::size_t ran =
                run_once(static_cast<Tick>(alloy::uptime_ms()));
            bool have_idle = false;
            if constexpr (IdleSlots > 0) {
                have_idle = idle_count_ > 0;
            }
            if (ran == 0 && !have_idle) {
                alloy::sleep_for(std::chrono::milliseconds{1});
            }
        }
    }

    [[nodiscard]] std::size_t size() const { return count_; }

private:
    // The slot carries the budgeted-discipline fields ONLY when that
    // discipline is compiled in — a default-discipline scheduler's RAM
    // footprint is the historical one.
    struct slot_base {
        task_fn fn{nullptr};
        void* ctx{nullptr};
        software_timer<Tick> timer{};
        bool periodic{false};
        volatile std::uint32_t events{0};
    };
    struct slot_budgeted : slot_base {
        task_class cls{task_class::deferred};
        bool ready{false};
        std::uint32_t wake_reason{0};
    };
    using slot = std::conditional_t<kBudgeted, slot_budgeted, slot_base>;

    struct idle_entry {
        task_fn fn{nullptr};
        void* ctx{nullptr};
    };
    struct empty {};

    // Run one task with a pre-taken event snapshot (the historical path).
    void dispatch_one(std::size_t i, std::uint32_t ev, std::uint32_t reason) {
        if constexpr (kObserved) {
            if (obs_ != nullptr) {
                obs_->dispatch_begin(i, reason);
            }
        } else {
            (void)reason;
        }
        slots_[i].fn(slots_[i].ctx, ev);
        if constexpr (kObserved) {
            if (obs_ != nullptr) {
                obs_->dispatch_end(i);
            }
        }
    }

    // Run one LATCHED task (the budgeted path): snapshot-and-clear the
    // events AT DISPATCH, not at latch, so bits that arrive while the task
    // waited its turn ride along instead of waiting a full extra turn.
    void dispatch_latched(std::size_t i)
        requires (kBudgeted)
    {
        slot& t = slots_[i];
        const auto st = arch::irq_save();
        const std::uint32_t ev = t.events;
        t.events = 0;
        arch::irq_restore(st);
        const std::uint32_t reason = t.wake_reason;
        t.ready = false;
        t.wake_reason = 0;
        dispatch_one(i, ev, reason);
    }

    slot slots_[MaxTasks]{};
    std::size_t count_{0};
    [[no_unique_address]] std::conditional_t<
        (IdleSlots > 0), std::array<idle_entry, (IdleSlots > 0 ? IdleSlots : 1)>,
        empty>
        idle_{};
    [[no_unique_address]] std::conditional_t<(IdleSlots > 0), std::size_t, empty>
        idle_count_{};
    [[no_unique_address]] std::conditional_t<(IdleSlots > 0), std::size_t, empty>
        idle_next_{};
    [[no_unique_address]] std::conditional_t<kObserved, Observer*, empty> obs_{};
    volatile Tick isr_ticks_{0};
    [[no_unique_address]] std::conditional_t<kBudgeted, Tick, empty>
        last_deferred_now_{};
    [[no_unique_address]] std::conditional_t<kBudgeted, bool, empty>
        have_deferred_now_{};
};

}  // namespace alloy
