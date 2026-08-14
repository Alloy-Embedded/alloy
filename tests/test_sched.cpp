// Unit tests for the cooperative scheduler (src/alloy/sched.hpp) — the first
// suite this file has of its own (the services tests exercise the default
// discipline through an application; these pin the scheduler's OWN contract).
//
// What matters here, in order: that the DEFAULT discipline is byte-for-byte
// the historical one (run every ready task, mid-superstep signals included);
// that drain_budgeted bounds every tick by construction (one deferred task
// per distinct `now`, table order, readiness latched across supersteps); and
// that the observer seam brackets exactly what ran, in order, with the wake
// reason — because a slot firmware's published run times hang off it.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "alloy/sched.hpp"
#include "alloy/util/timer.hpp"
#include "alloy_test.hpp"

namespace {

// The budgeted-discipline spelling, once: the discipline is a TEMPLATE
// parameter (the unused path must fold away at -Os — measured in the commit).
template <std::size_t N>
using budgeted = alloy::scheduler<N, std::uint32_t, alloy::null_dispatch_observer,
                                  alloy::sched_dispatch::drain_budgeted>;
template <std::size_t N, std::size_t Idle>
using budgeted_idle =
    alloy::scheduler<N, std::uint32_t, alloy::null_dispatch_observer,
                     alloy::sched_dispatch::drain_budgeted, Idle>;

// A recording context: every task appends its tag.
struct trace {
    std::vector<int> runs;
    std::vector<std::uint32_t> events;
};

template <int Tag>
void tagged(void* ctx, std::uint32_t ev) {
    auto* t = static_cast<trace*>(ctx);
    t->runs.push_back(Tag);
    t->events.push_back(ev);
}

bool ran_exactly(const trace& t, std::initializer_list<int> want) {
    return t.runs == std::vector<int>(want);
}

}  // namespace

// ══════════════════ the default discipline is the historical one ═══════════

ALLOY_TEST(sched_default_runs_every_ready_task_in_table_order) {
    alloy::scheduler<4> s;
    trace t;
    s.add(&tagged<1>, &t, std::chrono::milliseconds{10});
    s.add(&tagged<2>, &t, std::chrono::milliseconds{10});
    s.add(&tagged<3>, &t, std::chrono::milliseconds{20});

    ALLOY_CHECK_EQ(s.run_once(10), 2u);  // 1 and 2 due
    ALLOY_CHECK(ran_exactly(t, {1, 2}));
    ALLOY_CHECK_EQ(s.run_once(20), 3u);  // all three due
    ALLOY_CHECK(ran_exactly(t, {1, 2, 1, 2, 3}));
}

// The historical mid-superstep property, now stated as its own pin: a task
// dispatched earlier in the table may signal a later one and the later one
// runs THE SAME superstep. (Under drain_budgeted this is deliberately not so —
// readiness there is the latch.)
ALLOY_TEST(sched_default_same_superstep_signal_reaches_later_tasks) {
    static alloy::scheduler<4>* sp = nullptr;
    static std::size_t victim = 0;
    alloy::scheduler<4> s;
    sp = &s;
    trace t;

    s.add([](void* ctx, std::uint32_t) {  // task 0: signals task 1
        sp->signal(victim, 0x4u);
        static_cast<trace*>(ctx)->runs.push_back(1);
    }, &t, std::chrono::milliseconds{10});
    victim = s.add(&tagged<2>, &t, std::chrono::milliseconds{0});  // event-only

    ALLOY_CHECK_EQ(s.run_once(10), 2u);
    ALLOY_CHECK(ran_exactly(t, {1, 2}));
    ALLOY_CHECK_EQ(t.events.back(), 0x4u);
}

// ═══════════════════════ the budgeted drain discipline ═════════════════════

ALLOY_TEST(sched_budgeted_drains_one_deferred_per_distinct_now_in_table_order) {
    budgeted<4> s;
    trace t;
    // All three share a period: a coincident boundary, the slot firmware's
    // 1 s edge in miniature.
    s.add(&tagged<1>, &t, std::chrono::milliseconds{10});
    s.add(&tagged<2>, &t, std::chrono::milliseconds{10});
    s.add(&tagged<3>, &t, std::chrono::milliseconds{10});

    ALLOY_CHECK_EQ(s.run_once(10), 1u);  // the edge: one task, highest priority
    ALLOY_CHECK(ran_exactly(t, {1}));
    ALLOY_CHECK_EQ(s.run_once(11), 1u);  // next tick drains the next
    ALLOY_CHECK(ran_exactly(t, {1, 2}));
    ALLOY_CHECK_EQ(s.run_once(12), 1u);
    ALLOY_CHECK(ran_exactly(t, {1, 2, 3}));
    ALLOY_CHECK_EQ(s.run_once(13), 0u);  // drained: nothing left
}

ALLOY_TEST(sched_budgeted_budget_is_per_now_not_per_call) {
    budgeted<4> s;
    trace t;
    s.add(&tagged<1>, &t, std::chrono::milliseconds{10});
    s.add(&tagged<2>, &t, std::chrono::milliseconds{10});

    ALLOY_CHECK_EQ(s.run_once(10), 1u);
    ALLOY_CHECK_EQ(s.run_once(10), 0u);  // same now: budget spent
    ALLOY_CHECK_EQ(s.run_once(10), 0u);
    ALLOY_CHECK_EQ(s.run_once(11), 1u);  // now advanced: budget reopens
    ALLOY_CHECK(ran_exactly(t, {1, 2}));
}

// Tick ZERO must hold a budget like any other tick — the "have I dispatched
// at this now yet" state is a flag, not a zero sentinel.
ALLOY_TEST(sched_budgeted_tick_zero_is_a_first_class_budget_holder) {
    budgeted<2> s;
    trace t;
    s.add(&tagged<1>, &t, std::uint32_t{0});  // event-only
    s.add(&tagged<2>, &t, std::uint32_t{0});
    s.signal(0, 1u);
    s.signal(1, 1u);

    ALLOY_CHECK_EQ(s.run_once(0), 1u);  // the very first tick still budgets
    ALLOY_CHECK_EQ(s.run_once(0), 0u);
    ALLOY_CHECK_EQ(s.run_once(1), 1u);
    ALLOY_CHECK(ran_exactly(t, {1, 2}));
}

ALLOY_TEST(sched_budgeted_always_class_is_never_budgeted) {
    budgeted<4> s;
    trace t;
    using tc = alloy::sched_class;
    s.add(&tagged<1>, &t, std::chrono::milliseconds{1}, tc::always);
    s.add(&tagged<2>, &t, std::chrono::milliseconds{10});
    s.add(&tagged<3>, &t, std::chrono::milliseconds{10});

    // At the coincident edge the always task AND one deferred run — the
    // "base work + one slot" bound, by construction.
    ALLOY_CHECK_EQ(s.run_once(10), 2u);
    ALLOY_CHECK(ran_exactly(t, {1, 2}));
    ALLOY_CHECK_EQ(s.run_once(11), 2u);  // always again, plus the drained 3
    ALLOY_CHECK(ran_exactly(t, {1, 2, 1, 3}));
}

// Readiness is a LATCH, not a counter: a thread that lagged does not replay
// the missed periods as a burst.
ALLOY_TEST(sched_budgeted_missed_ticks_coalesce_instead_of_bursting) {
    budgeted<2> s;
    trace t;
    s.add(&tagged<1>, &t, std::chrono::milliseconds{10});

    // The thread vanishes for five periods, then sees one tick.
    ALLOY_CHECK_EQ(s.run_once(50), 1u);
    ALLOY_CHECK_EQ(s.run_once(51), 1u);  // poll() catch-up re-latched once...
    ALLOY_CHECK_EQ(s.run_once(52), 1u);
    ALLOY_CHECK_EQ(s.run_once(53), 1u);
    ALLOY_CHECK_EQ(s.run_once(54), 1u);  // ...one per elapsed interval,
    ALLOY_CHECK_EQ(s.run_once(55), 0u);  // and then the grid is caught up:
    // five intervals were owed and five ran — but ONE PER TICK, bounded,
    // never five bodies inside one superstep.
    ALLOY_CHECK_EQ(t.runs.size(), 5u);
}

// Readiness must survive the task's own timer firing AGAIN before the budget
// reaches it — the latch is SET, never toggled. (Found by a mutation that
// survived the first campaign: `ready = !ready` passed every drain test,
// because they never re-fired an undispatched task's period.)
ALLOY_TEST(sched_budgeted_readiness_survives_a_refire_before_dispatch) {
    budgeted<4> s;
    trace t;
    s.add(&tagged<1>, &t, std::chrono::milliseconds{10});
    s.add(&tagged<2>, &t, std::chrono::milliseconds{10});

    ALLOY_CHECK_EQ(s.run_once(10), 1u);  // t1 drains; t2 stays LATCHED
    ALLOY_CHECK(ran_exactly(t, {1}));
    // The thread skips straight to the NEXT period boundary: both timers
    // fire again while t2 is still waiting its turn.
    ALLOY_CHECK_EQ(s.run_once(20), 1u);  // t1 again (table order)
    ALLOY_CHECK_EQ(s.run_once(21), 1u);  // t2 MUST still be ready
    ALLOY_CHECK(ran_exactly(t, {1, 1, 2}));
}

// ═══════════════════════════ the idle round-robin ══════════════════════════

ALLOY_TEST(sched_idle_list_rotates_one_entry_per_empty_superstep) {
    alloy::scheduler<4, std::uint32_t, alloy::null_dispatch_observer,
                     alloy::sched_dispatch::run_all_ready, 2>
        s;
    trace t;
    s.add(&tagged<1>, &t, std::chrono::milliseconds{100});
    ALLOY_CHECK(s.add_idle(&tagged<7>, &t));
    ALLOY_CHECK(s.add_idle(&tagged<8>, &t));

    ALLOY_CHECK_EQ(s.run_once(1), 0u);  // nothing due: idle entry 7
    ALLOY_CHECK_EQ(s.run_once(2), 0u);  // idle entry 8
    ALLOY_CHECK_EQ(s.run_once(3), 0u);  // wraps to 7
    ALLOY_CHECK(ran_exactly(t, {7, 8, 7}));
    ALLOY_CHECK_EQ(s.run_once(100), 1u);  // a due task pre-empts idle
    ALLOY_CHECK(ran_exactly(t, {7, 8, 7, 1}));
}

// ═══════════════════════════ the observer seam ═════════════════════════════

namespace {
struct recording_observer {
    std::vector<int> log;          // +tag on begin, -tag on end, 0 on idle
    std::vector<std::uint32_t> reasons;
    void dispatch_begin(std::size_t task, std::uint32_t reason) {
        log.push_back(static_cast<int>(task) + 1);
        reasons.push_back(reason);
    }
    void dispatch_end(std::size_t task) { log.push_back(-(static_cast<int>(task) + 1)); }
    void idle_pass() { log.push_back(0); }
};
}  // namespace

ALLOY_TEST(sched_observer_brackets_each_dispatch_with_the_wake_reason) {
    alloy::scheduler<4, std::uint32_t, recording_observer> s;
    recording_observer obs;
    s.set_observer(obs);
    trace t;
    s.add(&tagged<1>, &t, std::uint32_t{10});
    s.add(&tagged<2>, &t, std::uint32_t{0});  // event-only
    s.signal(1, 0x8u);

    ALLOY_CHECK_EQ(s.run_once(10), 2u);
    // begin 1, end 1, begin 2, end 2 — strictly bracketed, table order.
    ALLOY_CHECK(obs.log == std::vector<int>({1, -1, 2, -2}));
    ALLOY_CHECK_EQ(obs.reasons[0], alloy::wake::timer);
    ALLOY_CHECK_EQ(obs.reasons[1], alloy::wake::event);

    obs.log.clear();
    ALLOY_CHECK_EQ(s.run_once(11), 0u);
    ALLOY_CHECK(obs.log == std::vector<int>({0}));  // the idle_pass
}

// A task both due and signalled reports BOTH reasons in one dispatch.
ALLOY_TEST(sched_observer_reason_carries_both_bits_when_both_apply) {
    alloy::scheduler<2, std::uint32_t, recording_observer> s;
    recording_observer obs;
    s.set_observer(obs);
    trace t;
    s.add(&tagged<1>, &t, std::uint32_t{10});
    s.signal(0, 1u);
    ALLOY_CHECK_EQ(s.run_once(10), 1u);
    ALLOY_CHECK_EQ(obs.reasons[0], alloy::wake::timer | alloy::wake::event);
}

// ═══════════════════════════ the ISR half ══════════════════════════════════

ALLOY_TEST(sched_isr_tick_feeds_the_no_argument_superstep) {
    alloy::scheduler<2> s;
    trace t;
    s.add(&tagged<1>, &t, std::chrono::milliseconds{3});

    ALLOY_CHECK_EQ(s.run_once(), 0u);  // tick 0: not due
    s.tick_from_isr();
    s.tick_from_isr();
    s.tick_from_isr();                 // internal tick now 3
    ALLOY_CHECK_EQ(s.isr_ticks(), 3u);
    ALLOY_CHECK_EQ(s.run_once(), 1u);
    ALLOY_CHECK(ran_exactly(t, {1}));
}

// ══════════════ the shared-epoch property of software_timer ════════════════
//
// Nested-period edges COINCIDE FOREVER when the timers share an epoch,
// because poll() advances each phase by exactly its own interval — never to
// `now`. Slot firmware's data-freshness ordering ("the 1 s work runs in the
// same superstep as a 100 ms edge") hangs off this arithmetic property, and
// no test asserted it before this one: a well-meaning "fix" that re-armed
// from `now` would silently split the edges apart after the first late poll.
ALLOY_TEST(sched_software_timers_from_one_epoch_keep_their_edges_aligned) {
    alloy::software_timer<std::uint32_t> t10{10};
    alloy::software_timer<std::uint32_t> t100{100};
    alloy::software_timer<std::uint32_t> t1000{1000};
    t10.reset(0);
    t100.reset(0);
    t1000.reset(0);

    for (std::uint32_t now = 1; now <= 5000; ++now) {
        const bool f10 = t10.poll(now);
        const bool f100 = t100.poll(now);
        const bool f1000 = t1000.poll(now);
        // A coarser edge NEVER fires without every finer one firing with it.
        if (f1000) {
            ALLOY_CHECK(f100);
        }
        if (f100) {
            ALLOY_CHECK(f10);
        }
    }

    // ...and the alignment survives a LATE poll (the thread stalled past
    // several fine edges): after catch-up, the grids still coincide.
    alloy::software_timer<std::uint32_t> a{10};
    alloy::software_timer<std::uint32_t> b{100};
    a.reset(0);
    b.reset(0);
    std::uint32_t now = 0;
    now = 437;  // a stall that is a multiple of neither period
    while (a.poll(now)) {
    }
    while (b.poll(now)) {
    }
    for (now = 438; now <= 2000; ++now) {
        const bool fa = a.poll(now);
        const bool fb = b.poll(now);
        if (fb) {
            ALLOY_CHECK(fa);
        }
        if (fb) {
            ALLOY_CHECK_EQ(now % 100, 0u);  // still on the ORIGINAL grid
        }
    }
}
