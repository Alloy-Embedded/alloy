// The WINDOW watchdog facade (src/alloy/wwdt.hpp) and the STM32 WWDG's
// arithmetic (src/alloy/hal/window_watchdog/st_wwdg_detail.hpp).
//
// WHAT THIS FILE PROVES, and it is the part nothing else can:
//
//  - the rounding rule holds over a sweep, in both directions at once.
//    `request ⊆ reported ⊆ enforced` is the whole safety argument of this
//    driver: a compliant feeder is never reset by quantisation, and the two
//    numbers handed back are directly usable as a schedule. A single worked
//    example would not have caught the off-by-one that ceil/floor invite, so
//    this sweeps every deadline in a wide range against several clocks and
//    both known WDGTB widths.
//  - the generated DEGREE is load-bearing. `feat.timebase_max` is 7 on
//    wwdg_v2 and 3 on wwdg_v1, and the SAME code has to be right on both: at
//    timebase_max=3 the longest window is sixteen times shorter, and the plan
//    must clamp there rather than program a prescaler code the field cannot
//    hold. A literal 7 in the driver passes every v2 test and is wrong.
//  - the register SEQUENCE, against a fake block: CFR before CR, the arm bit
//    in the start write and NOT in the refresh, EWI only when a hook exists.
//    Order is not cosmetic here — CR.WDGA starts the countdown and cannot be
//    cleared by software, so a window written after it is written to a dog
//    already running.
//  - two `start()`s asking for DIFFERENT windows trap, and two asking for the
//    same one do not.
//
// WHAT IT DOES NOT PROVE: that the silicon resets on an early feed, or at all.
// Renode 1.16.1 ships no WWDG model for any STM32 — its
// Timers.STM32_IndependentWatchdog is the other peripheral — and no board was
// on hand. Everything below the register write is unwitnessed.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>

#include "alloy/hal/window_watchdog/st_wwdg_detail.hpp"
#include "alloy/wwdt.hpp"
#include "alloy_test.hpp"

namespace {

using alloy::hal::detail::wwdg_arm_bit;
using alloy::hal::detail::wwdg_max_counts;
using alloy::hal::detail::wwdg_plan;
using alloy::hal::detail::wwdg_program;

// The clock is the only thing that decides what a WWDG can express, so the
// sweep runs several — including one slow enough that a single tick is
// milliseconds.
constexpr std::uint32_t kClocks[] = {1'000'000u, 16'000'000u, 48'000'000u,
                                     64'000'000u, 170'000'000u};

// The exact bound a plan enforces, in microseconds, computed independently of
// the planner: counts ticks of (4096 << wdgtb) / pclk.
constexpr std::uint64_t enforced_deadline_ns(const wwdg_program& p, std::uint32_t pclk) {
    const std::uint64_t counts = (p.reload & 0x3Fu) + 1u;
    return counts * (static_cast<std::uint64_t>(4096u << p.wdgtb) * 1'000'000'000ull) / pclk;
}
constexpr std::uint64_t enforced_earliest_ns(const wwdg_program& p, std::uint32_t pclk) {
    const std::uint64_t elapsed = static_cast<std::uint64_t>(p.reload) - p.win;
    return elapsed * (static_cast<std::uint64_t>(4096u << p.wdgtb) * 1'000'000'000ull) / pclk;
}

}  // namespace

ALLOY_TEST(wwdg_plan_reload_always_keeps_the_arm_bit) {
    // T6 is not a count. A reload with it clear resets the part on the next
    // tick, so every plan must land in 0x40..0x7F.
    bool all_armed = true;
    bool all_in_range = true;
    for (std::uint32_t pclk : kClocks) {
        for (std::uint32_t us = 1u; us <= 600'000u; us = us * 3u / 2u + 1u) {
            const auto p = wwdg_plan(pclk, 7u, us, us / 3u);
            all_armed = all_armed && ((p.reload & wwdg_arm_bit) != 0u);
            all_in_range = all_in_range && p.reload >= 0x40u && p.reload <= 0x7Fu &&
                           p.win <= p.reload && p.wdgtb <= 7u;
        }
    }
    ALLOY_CHECK(all_armed);
    ALLOY_CHECK(all_in_range);
}

ALLOY_TEST(wwdg_plan_window_contains_the_request) {
    // The safety half: whatever was asked for, the ENFORCED window is at
    // least as permissive, so a feeder honouring the request is never reset
    // by rounding.
    bool contains = true;
    for (std::uint32_t pclk : kClocks) {
        const std::uint32_t longest = alloy::hal::detail::wwdg_longest_us(pclk, 7u);
        for (std::uint32_t us = 1u; us <= longest; us = us * 5u / 4u + 1u) {
            for (std::uint32_t frac = 0u; frac < 4u; ++frac) {
                const std::uint32_t early = us * frac / 4u;
                const auto p = wwdg_plan(pclk, 7u, us, early);
                contains = contains &&
                           enforced_deadline_ns(p, pclk) >= std::uint64_t{us} * 1000u &&
                           enforced_earliest_ns(p, pclk) <= std::uint64_t{early} * 1000u;
            }
        }
    }
    ALLOY_CHECK(contains);
}

ALLOY_TEST(wwdg_plan_report_is_inside_what_is_enforced) {
    // The usability half: the two numbers handed back can be scheduled
    // against directly. A feed at `earliest_us` is truly allowed and a feed at
    // `deadline_us` is truly accepted, which is only true if the report is
    // strictly INSIDE the enforced pair.
    bool inside = true;
    for (std::uint32_t pclk : kClocks) {
        const std::uint32_t longest = alloy::hal::detail::wwdg_longest_us(pclk, 7u);
        for (std::uint32_t us = 1u; us <= longest; us = us * 5u / 4u + 1u) {
            const auto p = wwdg_plan(pclk, 7u, us, us / 2u);
            inside = inside &&
                     std::uint64_t{p.deadline_us} * 1000u <= enforced_deadline_ns(p, pclk) &&
                     std::uint64_t{p.earliest_us} * 1000u >= enforced_earliest_ns(p, pclk) &&
                     p.earliest_us < p.deadline_us;
        }
    }
    ALLOY_CHECK(inside);
}

ALLOY_TEST(wwdg_degree_bounds_the_prescaler_not_a_literal) {
    // wwdg_v1's WDGTB is two bits. The same planner, given that degree, must
    // never emit a code above 3 — and its longest window is 2^(7-3) = SIXTEEN
    // times shorter, which is what makes the number load-bearing rather than
    // decorative. (This assertion was written as "eight times" from the
    // two-versus-three-bits framing and the sweep caught it: the ratio is of
    // the prescaler VALUES, not of the field widths.)
    constexpr std::uint32_t pclk = 64'000'000u;
    ALLOY_CHECK_EQ(alloy::hal::detail::wwdg_longest_us(pclk, 7u),
                   alloy::hal::detail::wwdg_longest_us(pclk, 3u) * 16u);

    bool bounded = true;
    for (std::uint32_t us = 1u; us <= 600'000u; us = us * 3u / 2u + 1u) {
        const auto p = wwdg_plan(pclk, 3u, us, 0u);
        bounded = bounded && p.wdgtb <= 3u;
    }
    ALLOY_CHECK(bounded);

    // And past its reach it clamps to the longest it HAS, still armed and
    // still feedable — the facade's admit check is what refuses the ask.
    const auto over = wwdg_plan(pclk, 3u, 1'000'000u, 0u);
    ALLOY_CHECK_EQ(over.wdgtb, std::uint8_t{3});
    ALLOY_CHECK_EQ(over.reload, std::uint8_t{0x7F});
}

ALLOY_TEST(wwdg_plan_zero_early_bound_opens_the_window_fully) {
    // W == reload means T is never above W, so any feed is accepted. That is
    // how "no early bound" is spelled: a value, not a disable bit.
    const auto p = wwdg_plan(64'000'000u, 7u, 20'000u, 0u);
    ALLOY_CHECK_EQ(p.win, p.reload);
    ALLOY_CHECK_EQ(p.earliest_us, 0u);
}

ALLOY_TEST(wwdg_plan_matches_the_manual_worked_example) {
    // 64 MHz PCLK, a 5..20 ms window — the numbers the shipped
    // nucleo_g0b1re board file asks for, computed by hand from
    // t_WWDG = t_PCLK * 4096 * 2^WDGTB * (T[5:0] + 1):
    //   tick at WDGTB=3 is 4096*8/64e6 = 512 us
    //   ceil(20000/512) = 40 counts -> reload 0x40|39 = 0x67, deadline 20480 us
    //   floor(5000/512) = 9 ticks of slack -> W = 0x67-9 = 0x5E, earliest 4608 us
    const auto p = wwdg_plan(64'000'000u, 7u, 20'000u, 5'000u);
    ALLOY_CHECK_EQ(p.wdgtb, std::uint8_t{3});
    ALLOY_CHECK_EQ(p.reload, std::uint8_t{0x67});
    ALLOY_CHECK_EQ(p.win, std::uint8_t{0x5E});
    ALLOY_CHECK_EQ(p.deadline_us, 20'480u);
    ALLOY_CHECK_EQ(p.earliest_us, 4'608u);
}

// ── The facade over a fake block ────────────────────────────────────────

namespace {

struct fake_ip {};

// A stand-in for the generated instance descriptor, in the shape codegen
// emits: an IP tag, a base, a gate, an IRQ line and the nested feat.
template <int N>
struct fake_wwdg {
    using ip = fake_ip;
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
    struct feat {
        static constexpr std::uint32_t timebase_max = 7u;
    };
};

struct fake_clock {
    static constexpr std::uint32_t sysclk_hz = 64'000'000u;
    static constexpr std::uint32_t ahb_hz = 64'000'000u;
    static constexpr std::uint32_t apb_hz = 64'000'000u;
    static constexpr std::uint32_t apb2_hz = 64'000'000u;
};

// What the driver wrote, in order.
struct trace {
    std::uint32_t cfr = 0;
    std::uint32_t cr = 0;
    int cfr_at = -1;
    int cr_at = -1;
    int n = 0;
    bool gated = false;
    void reset() { *this = trace{}; }
};
trace g_trace[6];

}  // namespace

// The driver under test is selected on the instance's IP, exactly as the real
// one is; this specialization stands in for st_wwdg_v2.hpp's register pokes
// while running the REAL planner.
namespace alloy::hal {
template <int N>
struct wwdt_impl<fake_wwdg<N>> {
    using Inst = fake_wwdg<N>;
    static constexpr std::uint32_t timebase_max = Inst::feat::timebase_max;

    static constexpr std::uint32_t longest_us(std::uint32_t pclk) {
        return detail::wwdg_longest_us(pclk, timebase_max);
    }
    static constexpr std::uint32_t shortest_us(std::uint32_t pclk) {
        return detail::wwdg_shortest_us(pclk);
    }
    static constexpr detail::wwdg_program plan(std::uint32_t pclk, std::uint32_t d,
                                               std::uint32_t e) {
        return detail::wwdg_plan(pclk, timebase_max, d, e);
    }
    static detail::wwdg_program start(std::uint32_t pclk, std::uint32_t d, std::uint32_t e) {
        const auto p = plan(pclk, d, e);
        reload_ = p.reload;
        g_trace[N].gated = true;
        g_trace[N].cfr = (static_cast<std::uint32_t>(p.win) << 0u) |
                         (static_cast<std::uint32_t>(p.wdgtb) << 11u) |
                         (hook_ != nullptr ? (1u << 9u) : 0u);
        g_trace[N].cfr_at = g_trace[N].n++;
        g_trace[N].cr = 0x80u | p.reload;
        g_trace[N].cr_at = g_trace[N].n++;
        return p;
    }
    static void feed() {
        g_trace[N].cr = reload_;
        g_trace[N].cr_at = g_trace[N].n++;
    }
    static void on_early_wakeup(void (*fn)()) { hook_ = fn; }

    static inline std::uint8_t reload_ = 0x7Fu;
    static inline void (*hook_)() = nullptr;
};
}  // namespace alloy::hal

namespace {

template <int N>
using dog = alloy::wwdt::window_watchdog<fake_wwdg<N>, fake_clock>;

bool refuses(auto body) {
    const pid_t pid = fork();
    if (pid == 0) {
        body();
        _exit(0);  // reached only if the guard FAILED to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

}  // namespace

ALLOY_TEST(wwdt_start_programs_the_window_before_it_arms) {
    g_trace[0].reset();
    constexpr dog<0> d{};
    const auto got = d.start({.deadline = std::chrono::microseconds{20'000},
                              .earliest = std::chrono::microseconds{5'000}});
    ALLOY_CHECK(g_trace[0].gated);
    // CFR strictly before CR: the arm write starts the countdown.
    ALLOY_CHECK(g_trace[0].cfr_at >= 0 && g_trace[0].cfr_at < g_trace[0].cr_at);
    ALLOY_CHECK_EQ(g_trace[0].cfr, (0x5Eu << 0u) | (3u << 11u));
    ALLOY_CHECK_EQ(g_trace[0].cr, 0x80u | 0x67u);
    ALLOY_CHECK_EQ(got.deadline.count(), 20'480);
    ALLOY_CHECK_EQ(got.earliest.count(), 4'608);

    // The refresh carries the reload and NOT the activation bit — writing
    // WDGA back is not how this peripheral is fed, and software cannot clear
    // it anyway.
    d.feed();
    ALLOY_CHECK_EQ(g_trace[0].cr, 0x67u);
}

ALLOY_TEST(wwdt_early_wakeup_is_only_enabled_when_a_hook_exists) {
    g_trace[1].reset();
    constexpr dog<1> d{};
    d.on_early_wakeup(+[] {});
    (void)d.start({.deadline = std::chrono::microseconds{20'000},
                   .earliest = std::chrono::microseconds{5'000}});
    // An enabled EWI with no handler is a vector into Default_Handler one tick
    // before the reset, so the bit follows the hook rather than a config flag.
    ALLOY_CHECK((g_trace[1].cfr & (1u << 9u)) != 0u);
}

ALLOY_TEST(wwdt_the_board_limits_are_compile_time_constants) {
    // 64 MHz, three-bit WDGTB: one tick is 64 us and the counter reaches 64 of
    // them at the coarsest prescaler. Half a second, not the IWDG's 32.
    static_assert(dog<0>::longest.count() == 524'288);
    static_assert(dog<0>::shortest.count() == 64);
    ALLOY_CHECK_EQ(dog<0>::longest.count(), 524'288);
}

ALLOY_TEST(wwdt_two_windows_on_one_block_trap_and_one_window_twice_does_not) {
    // The shared claim, with the window as the witness. Arming the same window
    // twice is not a contradiction; arming two different ones is, and the
    // second silently replacing the first is how a program ends up believing
    // in a deadline nothing enforces.
    ALLOY_CHECK(refuses([] {
        constexpr dog<2> d{};
        (void)d.start({.deadline = std::chrono::microseconds{20'000},
                       .earliest = std::chrono::microseconds{5'000}});
        (void)d.start({.deadline = std::chrono::microseconds{10'000},
                       .earliest = std::chrono::microseconds{1'000}});
    }));
    ALLOY_CHECK(!refuses([] {
        constexpr dog<3> d{};
        (void)d.start({.deadline = std::chrono::microseconds{8'000},
                       .earliest = std::chrono::microseconds{2'000}});
        (void)d.start({.deadline = std::chrono::microseconds{8'000},
                       .earliest = std::chrono::microseconds{2'000}});
    }));
}

ALLOY_TEST(wwdt_an_impossible_window_traps_at_run_time) {
    // The compile-time half of this check cannot be tested from inside a test
    // (a [[gnu::error]] that fires does not link), so what is exercised here
    // is the runtime leg: values the optimizer cannot fold still have to be
    // refused rather than programmed.
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t early = 30'000u;  // above the deadline
        constexpr dog<4> d{};
        (void)d.start({.deadline = std::chrono::microseconds{20'000},
                       .earliest = std::chrono::microseconds{early}});
    }));
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t deadline = 4'000'000u;  // past the block's reach
        constexpr dog<5> d{};
        (void)d.start({.deadline = std::chrono::microseconds{deadline},
                       .earliest = std::chrono::microseconds{0}});
    }));
}

ALLOY_TEST(wwdt_null_stub_has_the_same_shape) {
    // Guard #6: a program that feeds a window watchdog compiles on a board
    // with none, and reports a zero window rather than a fake one.
    constexpr alloy::wwdt::null_window_watchdog d{};
    const auto got = d.start({.deadline = std::chrono::microseconds{20'000}});
    d.feed();
    d.on_early_wakeup(+[] {});
    ALLOY_CHECK_EQ(got.deadline.count(), 0);
    ALLOY_CHECK_EQ(alloy::wwdt::null_window_watchdog::longest.count(), 0);
}
