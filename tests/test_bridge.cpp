// The complementary bridge — a timer's THIRD personality (src/alloy/bridge.hpp)
// and the block where a wrong number destroys hardware instead of blinking an
// LED.
//
// WHY THIS FILE EXISTS AT ALL. Both bridge headers already SAID it existed:
// src/alloy/bridge.hpp promised "the dead-time arithmetic is pinned by host
// tests at every boundary of the DTG encoding" and
// hal/bridge/st_tim_adv.hpp promised "the arithmetic below is covered by host
// tests that pin every boundary". There were no such tests. The first thing
// swept here turned up a live defect in the bound those promises were about
// (see `admitted_dead_times_are_all_programmable`), which is roughly what an
// unchecked promise is worth.
//
// WHAT IT PROVES, IN TWO HALVES.
//
//   The ARITHMETIC half runs the REAL ST code — alloy/hal/bridge/st_tim_adv_dtg.hpp
//   is the driver's own arithmetic, split out of st_tim_adv.hpp precisely so a
//   host test can reach it (the driver proper needs a generated
//   alloy/ip/st/tim_adv.hpp that exists only inside a built project, and
//   tests/ deliberately never reads generated headers — see tests/test_crc.cpp
//   for the same rule, and hal/window_watchdog/st_wwdg_detail.hpp for the same
//   split). The sweeps are exhaustive over the encoding, not sampled.
//
//   The FACADE half runs the real `alloy::bridge::bind::open()` — its real
//   admission tests, its real claim, its real mux sequence — against a fake
//   chip declared in this file, with only the register pokes replaced. Same
//   seam, and same idiom, as tests/test_encoder.cpp.
//
// WHAT IT DOES NOT PROVE, AND IT IS THE HALF THAT MATTERS TO SOMEBODY WITH AN
// INVERTER ON THE BENCH: that the register sequence in st_tim_adv.hpp inserts
// a dead time, that MOE keeps the pins dark, or that the break input switches
// six outputs off. Nothing available to this project can prove those — no
// board is on hand, and alloy's generated Renode platform does not instantiate
// TIM1, so no model responds to any of those writes. What emulation DOES give
// is a bus-level log of the writes in order, which pins the sequence and not
// its effect; the driver header carries that trace. Put a scope on CH1 and
// CH1N before you connect a DC link.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>
#include <type_traits>

#include "alloy/bridge.hpp"
#include "alloy/hal/bridge/st_tim_adv_dtg.hpp"
#include "alloy/pwm.hpp"
#include "alloy_test.hpp"

namespace dt = alloy::hal::detail;

namespace {

// The curated width of TIM1's DTG field on the G0B1RE (registers/st/tim_adv.yaml
// gives BDTR.DTG as eight bits). Written here as the MASK the driver passes,
// so every number below is the number the driver computes.
constexpr std::uint32_t kMask = 0xFFu;

// The four range boundaries of the encoding, from RM0444 section 21. Not
// derived — transcribed, so that a change to the derivation has something
// independent to fail against.
constexpr std::uint32_t kLongest = 1008u;

}  // namespace

// ── The DTG encoding ─────────────────────────────────────────────────────

ALLOY_TEST(dtg_decodes_every_range_boundary) {
    // The four ranges and both ends of each. A single wrong shift here is a
    // dead time off by 2x, 8x or 16x — the difference between a working
    // inverter and a shoot-through.
    ALLOY_CHECK_EQ(dt::dtg_ticks(0x00u, kMask), 0u);
    ALLOY_CHECK_EQ(dt::dtg_ticks(0x7Fu, kMask), 127u);   // step 1 ends
    ALLOY_CHECK_EQ(dt::dtg_ticks(0x80u, kMask), 128u);   // step 2 begins
    ALLOY_CHECK_EQ(dt::dtg_ticks(0xBFu, kMask), 254u);   // step 2 ends
    ALLOY_CHECK_EQ(dt::dtg_ticks(0xC0u, kMask), 256u);   // step 8 begins
    ALLOY_CHECK_EQ(dt::dtg_ticks(0xDFu, kMask), 504u);   // step 8 ends
    ALLOY_CHECK_EQ(dt::dtg_ticks(0xE0u, kMask), 512u);   // step 16 begins
    ALLOY_CHECK_EQ(dt::dtg_ticks(0xFFu, kMask), kLongest);
}

ALLOY_TEST(dtg_longest_is_derived_from_the_curated_field_width) {
    // The bound is a DEGREE fact — the widest code this field holds, decoded —
    // and not the constant 1008 written into a driver. A narrower DTG gives a
    // shorter maximum with no edit anywhere.
    ALLOY_CHECK_EQ(dt::dtg_max_ticks(kMask), kLongest);
    ALLOY_CHECK_EQ(dt::dtg_max_ticks(0x7Fu), 127u);
    ALLOY_CHECK_EQ(dt::dtg_max_ticks(0x3Fu), 63u);
}

ALLOY_TEST(dtg_never_programs_less_dead_time_than_asked) {
    // THE SAFETY PROPERTY OF THE WHOLE FILE, swept over every representable
    // request. Too much dead time distorts the output voltage; too little
    // destroys the bridge, so the encoder is allowed to overshoot and never to
    // undershoot.
    bool ok = true;
    for (std::uint32_t ticks = 0; ticks <= kLongest; ++ticks) {
        if (dt::dtg_ticks(dt::dtg_code(ticks, kMask), kMask) < ticks) { ok = false; }
    }
    ALLOY_CHECK(ok);
}

ALLOY_TEST(dtg_overshoot_is_minimal) {
    // …and never overshoots more than it has to. Without this, "round up"
    // would be satisfied by always programming the widest code, which passes
    // the test above and gives every bridge 1008 ticks of dead time.
    bool minimal = true;
    for (std::uint32_t ticks = 1; ticks <= kLongest; ++ticks) {
        const std::uint32_t code = dt::dtg_code(ticks, kMask);
        if (code > 0u && dt::dtg_ticks(code - 1u, kMask) >= ticks) { minimal = false; }
    }
    ALLOY_CHECK(minimal);
}

ALLOY_TEST(dtg_is_monotone_in_its_code) {
    // The minimality argument above relies on the decoding being monotone
    // across the range switches, which is not obvious: the code jumps from
    // 0xBF to 0xC0 and the step changes from 2 to 8 at the same instant.
    bool monotone = true;
    std::uint32_t prev = 0u;
    for (std::uint32_t code = 0; code <= kMask; ++code) {
        const std::uint32_t t = dt::dtg_ticks(code, kMask);
        if (t < prev) { monotone = false; }
        prev = t;
    }
    ALLOY_CHECK(monotone);
}

ALLOY_TEST(dtg_ranges_do_not_abut_and_the_gaps_round_up) {
    // 255 ticks and 505..511 ticks are dead times this generator cannot
    // produce AT ALL — the ranges leave holes. A driver that assumed the
    // ranges were contiguous would land in one and program the range below.
    ALLOY_CHECK_EQ(dt::dtg_ticks(dt::dtg_code(255u, kMask), kMask), 256u);
    for (std::uint32_t ticks = 505u; ticks <= 511u; ++ticks) {
        ALLOY_CHECK_EQ(dt::dtg_ticks(dt::dtg_code(ticks, kMask), kMask), 512u);
    }
}

ALLOY_TEST(dtg_code_out_of_range_saturates_inside_the_field) {
    // The facade refuses a too-long dead time before this is reached, so this
    // pins the behaviour when that refusal is bypassed. It matters because the
    // caller ORs this value straight into BDTR, and the bits just above DTG
    // are LOCK: an answer of mask+1 did not fail loudly, it programmed DTG = 0
    // — no dead time — plus a configuration lock nobody asked for.
    ALLOY_CHECK_EQ(dt::dtg_code(kLongest + 1u, kMask), kMask);
    ALLOY_CHECK_EQ(dt::dtg_code(100'000u, kMask), kMask);
    ALLOY_CHECK(dt::dtg_code(100'000u, kMask) <= kMask);
    // …and on a narrower field the saturation is that field's own widest code,
    // not this one's.
    ALLOY_CHECK_EQ(dt::dtg_code(200u, 0x3Fu), 0x3Fu);
}

// ── The three roundings, which are not all the same direction ────────────

ALLOY_TEST(a_request_rounds_up_to_whole_ticks) {
    // 64 MHz: one tick is 15.625 ns. 500 ns is exactly 32 ticks; 501 ns is
    // 32.06 and must become 33, never 32.
    ALLOY_CHECK_EQ(dt::dtg_ns_to_ticks(500u, 64'000'000u), 32u);
    ALLOY_CHECK_EQ(dt::dtg_ns_to_ticks(501u, 64'000'000u), 33u);
    ALLOY_CHECK_EQ(dt::dtg_ns_to_ticks(1u, 64'000'000u), 1u);
    ALLOY_CHECK_EQ(dt::dtg_ns_to_ticks(0u, 64'000'000u), 0u);
}

ALLOY_TEST(a_report_rounds_down_and_never_overstates_protection) {
    // The report answers "how much dead time is the silicon inserting", and a
    // program that compares it against its own requirement must not be told a
    // number the hardware does not deliver. 33 ticks at 64 MHz is 515.625 ns,
    // so the honest report is 515.
    ALLOY_CHECK_EQ(dt::dtg_ticks_to_ns(32u, 64'000'000u), 500u);
    ALLOY_CHECK_EQ(dt::dtg_ticks_to_ns(33u, 64'000'000u), 515u);
    ALLOY_CHECK_EQ(dt::dtg_ticks_to_ns(0u, 64'000'000u), 0u);
    // A zero clock is not a division by zero.
    ALLOY_CHECK_EQ(dt::dtg_ticks_to_ns(32u, 0u), 0u);
}

ALLOY_TEST(the_report_is_still_never_below_the_request) {
    // The two rules above pull in opposite directions, and this is the
    // invariant that says they compose: request -> ticks rounds up far enough
    // that ticks -> ns rounding down still lands at or above where it started.
    // Swept over every whole nanosecond the encoding can reach at 64 MHz.
    bool ok = true;
    const std::uint32_t hz = 64'000'000u;
    const std::uint32_t max_ns = dt::dtg_max_dead_time_ns(kMask, hz);
    for (std::uint32_t ns = 0; ns <= max_ns; ++ns) {
        if (dt::dtg_achieved_ns(ns, kMask, hz) < ns) { ok = false; }
    }
    ALLOY_CHECK(ok);
}

// ── The bound, and the defect this file was written to find ──────────────

ALLOY_TEST(admitted_dead_times_are_all_programmable) {
    // THE REGRESSION TEST FOR A LIVE BUG.
    //
    // `max_dead_time_ns` is the bound `bridge::open`'s admission compares
    // against: ask for more and you get a compile error, ask for that exact
    // figure and you are admitted. It used to be computed with the REPORT's
    // conversion, which rounds UP — so on any kernel clock that does not
    // divide the generator's maximum exactly, the bound named a dead time one
    // nanosecond longer than the encoding can reach. That request was
    // admitted, converted back to max_ticks + 1, fell off the end of the
    // encoding, and (before the saturation above) programmed DTG = 0 with a
    // configuration lock: ZERO dead time, permanently, from the API's own
    // advertised maximum.
    //
    // The sweep is every integer megahertz from 1 to 200. 144 of those 200
    // clocks were affected; 64 MHz — the lead board's — was not, which is why
    // nothing caught it.
    bool ok = true;
    for (std::uint32_t hz = 1'000'000u; hz <= 200'000'000u; hz += 1'000'000u) {
        const std::uint32_t bound = dt::dtg_max_dead_time_ns(kMask, hz);
        const std::uint32_t ticks = dt::dtg_ns_to_ticks(bound, hz);
        if (ticks > dt::dtg_max_ticks(kMask)) { ok = false; }
        if (dt::dtg_code(ticks, kMask) > kMask) { ok = false; }
    }
    ALLOY_CHECK(ok);
}

ALLOY_TEST(the_old_bound_is_still_wrong_on_the_clocks_it_was_wrong_on) {
    // THE NEGATIVE CONTROL for the test above. Without it, that sweep passes
    // just as well against a bound that was never broken, and the reader has
    // no way to tell a fixed bug from a test of nothing. This recomputes the
    // OLD (ceiling) bound inline and shows it naming an unreachable dead time
    // at 11 MHz — one nanosecond above the new one.
    const std::uint32_t hz = 11'000'000u;
    const std::uint64_t exact = static_cast<std::uint64_t>(kLongest) * 1'000'000'000u;
    const std::uint32_t old_bound = static_cast<std::uint32_t>((exact + hz - 1u) / hz);
    const std::uint32_t new_bound = dt::dtg_max_dead_time_ns(kMask, hz);
    ALLOY_CHECK_EQ(old_bound, new_bound + 1u);
    // The old bound converts to one tick past the end of the encoding…
    ALLOY_CHECK(dt::dtg_ns_to_ticks(old_bound, hz) > dt::dtg_max_ticks(kMask));
    // …and the new one does not.
    ALLOY_CHECK_EQ(dt::dtg_ns_to_ticks(new_bound, hz), dt::dtg_max_ticks(kMask));
}

ALLOY_TEST(the_bound_is_the_largest_request_that_is_admissible) {
    // Positive control on the other side: the bound must not be needlessly
    // conservative either, or a board loses dead time it could have had. One
    // nanosecond past it is genuinely unreachable.
    bool tight = true;
    for (std::uint32_t hz = 1'000'000u; hz <= 200'000'000u; hz += 1'000'000u) {
        const std::uint32_t bound = dt::dtg_max_dead_time_ns(kMask, hz);
        if (dt::dtg_ns_to_ticks(bound + 1u, hz) <= dt::dtg_max_ticks(kMask)) {
            tight = false;
        }
    }
    ALLOY_CHECK(tight);
}

ALLOY_TEST(the_achieved_dead_time_at_the_lead_boards_clock) {
    // The concrete numbers examples/bridge prints on a nucleo_g0b1re: 500 ns
    // asked, 500 ns programmed, because 64 MHz divides it exactly. Pinned so a
    // change to any of the three roundings has to explain itself against a
    // number a person can check on a scope.
    ALLOY_CHECK_EQ(dt::dtg_achieved_ns(500u, kMask, 64'000'000u), 500u);
    // …and one that does NOT divide exactly: 510 ns is 32.64 ticks -> 33 ticks
    // -> 515.625 ns of real dead time, reported as 515.
    ALLOY_CHECK_EQ(dt::dtg_achieved_ns(510u, kMask, 64'000'000u), 515u);
}

// ── The timebase ─────────────────────────────────────────────────────────

ALLOY_TEST(center_aligned_arr_is_half_the_edge_aligned_one) {
    // A center-aligned counter goes up and back down, so one switching period
    // is 2 x ARR ticks. Getting this backwards doubles or halves the switching
    // frequency of an inverter.
    const auto center = dt::tim_adv_timebase_for(64'000'000u, 20'000u, true, 65'536u);
    const auto edge = dt::tim_adv_timebase_for(64'000'000u, 20'000u, false, 65'536u);
    ALLOY_CHECK_EQ(center.psc, 0u);
    ALLOY_CHECK_EQ(center.arr, 1599u);   // 1600 ticks up + 1600 down = 3200
    ALLOY_CHECK_EQ(edge.psc, 0u);
    ALLOY_CHECK_EQ(edge.arr, 3199u);
}

ALLOY_TEST(the_prescaler_appears_only_when_the_period_does_not_fit) {
    // PSC = 0 wherever possible, because every prescaler tick is duty
    // resolution a motor controller does not get back. 20 kHz at 64 MHz has
    // 1600 counts with no prescaler; 5 Hz does not fit in 16 bits and must
    // take one.
    ALLOY_CHECK_EQ(dt::tim_adv_timebase_for(64'000'000u, 5u, false, 65'536u).psc > 0u,
                   true);
    bool fits = true;
    for (std::uint32_t freq = 1u; freq <= 100'000u; freq += 997u) {
        const auto tb = dt::tim_adv_timebase_for(64'000'000u, freq, true, 65'536u);
        if (tb.arr + 1u > 65'536u) { fits = false; }
    }
    ALLOY_CHECK(fits);
}

ALLOY_TEST(a_zero_frequency_timebase_is_not_a_division_by_zero) {
    const auto tb = dt::tim_adv_timebase_for(64'000'000u, 0u, true, 65'536u);
    ALLOY_CHECK_EQ(tb.psc, 0u);
    ALLOY_CHECK_EQ(tb.arr, 0u);
}

// ── A fake chip, so the real facade can run on a host ────────────────────

namespace {

struct fake_ip {};

template <int N>
struct fake_timer {
    using ip = fake_ip;
    static constexpr alloy::clock_node kernel = alloy::clock_node::apb;
    // The DEGREE block a real instance gets from the IP's curated data. Only
    // `trgo` is read by the facade under test (the sample point is gated on
    // it); the others are here so this fake looks like what codegen emits for
    // an advanced timer rather than like the minimum that compiles.
    struct feat {
        static constexpr std::uint32_t channels = 4u;
        static constexpr std::uint32_t complementary_channels = 3u;
        static constexpr std::uint32_t break_inputs = 2u;
        static constexpr std::uint32_t trgo = 1u;
        static constexpr std::uint32_t moe = 1u;
    };
};

template <int N>
struct fake_pin {};

struct fake_clock {
    static constexpr std::uint32_t sysclk_hz = 64'000'000u;
    static constexpr std::uint32_t ahb_hz = 64'000'000u;
    static constexpr std::uint32_t apb_hz = 64'000'000u;
    static constexpr std::uint32_t apb2_hz = 64'000'000u;
};

}  // namespace

namespace alloy::routes {
template <int P, int T, alloy::signal S>
struct route<fake_pin<P>, fake_timer<T>, S> {
    static constexpr kind k = kind::af_fixed;
    static constexpr std::uint8_t af = 2;
};
}  // namespace alloy::routes

namespace alloy::hal {

// What the driver would have written, recorded instead. The fake models the
// ONE hardware behaviour the facade's contract depends on: a break — from the
// pin or from software — clears the main output enable, and clearing the flag
// does not bring it back.
struct bridge_log {
    unsigned enables = 0;
    std::uint32_t kernel_hz = 0;
    hal::bridge_config cfg{};
    unsigned phase_count = 0;
    bool break_enabled = false;
    hal::bridge_break_polarity break_active = hal::bridge_break_polarity::active_low;
    bool moe = false;
    bool bif = false;
    unsigned forced_breaks = 0;
    std::uint16_t duty[4] = {0, 0, 0, 0};
    std::uint16_t sample_point = 0;
    bool uif = false;
    //: Which phase write the counter reloads on, 0 = none. Models the straddle
    //: the coherent write exists to witness: the flag has to be set BETWEEN the
    //: first and last write, not before the call — the call clears it first.
    unsigned tear_on_phase = 0;

    // WHAT HAPPENED IN WHICH ORDER, which for this facade is a safety
    // property and not a detail: a pin muxed to a timer whose BDTR has not
    // been written yet is driven by nothing (OSSI is 0 out of reset), so the
    // phase pins must arrive AFTER the driver has programmed the off state.
    // `mux` records the fake pin's ordinal; `kEnable` marks the driver's
    // programming sequence.
    static constexpr int kEnable = 0;
    int seq[32] = {};
    unsigned seq_n = 0;
    void note(int what) {
        if (seq_n < 32u) { seq[seq_n++] = what; }
    }
};
inline bridge_log log{};

template <int P>
struct pin_impl<fake_pin<P>> {
    static void make_af(std::uint8_t) { log.note(P); }
};

// The OTHER personality this timer could have had, stubbed at the register
// boundary so the personality-conflict tests below run the real `pwm::bind`
// rather than a mock of it.
template <int T>
struct pwm_impl<fake_timer<T>> {
    static void enable(std::uint32_t, std::uint32_t, unsigned) {}
    static void set_duty(unsigned, std::uint16_t) {}
};

template <int T>
struct bridge_impl<fake_timer<T>> {
    // Three pairs, like the tim_adv this facade was built against.
    static constexpr unsigned phases = 3u;
    static inline std::uint32_t period_ticks = 1600u;

    static constexpr std::uint32_t max_dead_time_ns(std::uint32_t kernel_hz) {
        return detail::dtg_max_dead_time_ns(0xFFu, kernel_hz);
    }
    static constexpr std::uint32_t achieved_dead_time_ns(std::uint32_t ns,
                                                         std::uint32_t kernel_hz) {
        return detail::dtg_achieved_ns(ns, 0xFFu, kernel_hz);
    }

    template <bridge_opts<fake_timer<T>> = {}>
    static void enable(std::uint32_t kernel_hz, hal::bridge_config c,
                       unsigned phase_count, bool break_enabled,
                       hal::bridge_break_polarity break_active) {
        log.note(bridge_log::kEnable);
        ++log.enables;
        log.kernel_hz = kernel_hz;
        log.cfg = c;
        log.phase_count = phase_count;
        log.break_enabled = break_enabled;
        log.break_active = break_active;
        log.moe = false;  // the driver writes BDTR last, with MOE clear
    }
    static void set_duty(unsigned phase, std::uint16_t duty) {
        if (phase >= 1u && phase <= 4u) { log.duty[phase - 1u] = duty; }
        if (log.tear_on_phase == phase) { log.uif = true; }
    }
    static void set_sample_point(std::uint16_t point) { log.sample_point = point; }
    static bool update_elapsed() { return log.uif; }
    static void clear_update() { log.uif = false; }
    static void outputs_on() { log.moe = true; }
    static void outputs_off() { log.moe = false; }
    static bool outputs_enabled() { return log.moe; }
    static bool faulted() { return log.bif; }
    static void acknowledge_fault() { log.bif = false; }
    static void force_break() {
        ++log.forced_breaks;
        log.bif = true;
        log.moe = false;  // the hardware path, not a software MOE clear
    }
};

}  // namespace alloy::hal

namespace {

// A child that must NOT exit cleanly: the guard under test has to fire. Same
// idiom, and the same reason, as tests/test_encoder.cpp — an admission trap
// and a claim conflict are runtime aborts, because C++ cannot see across
// translation units.
template <class Fn>
bool refuses(Fn body) {
    const pid_t pid = fork();
    if (pid == 0) {
        body();
        _exit(0);  // reached only if the guard FAILED to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

template <int T>
using inverter = alloy::bridge::bind<
    fake_timer<T>, fake_clock, alloy::bridge::brk<fake_pin<9>>,
    alloy::bridge::phase<1, fake_pin<1>, fake_pin<2>>,
    alloy::bridge::phase<2, fake_pin<3>, fake_pin<4>>,
    alloy::bridge::phase<3, fake_pin<5>, fake_pin<6>>>;

template <int T>
using pwm_on = alloy::pwm::bind<fake_timer<T>, 1u, fake_pin<1>,
                                alloy::signal::ch1, fake_clock>;

constexpr alloy::bridge::config good{
    .freq_hz = 20'000u,
    .dead_time = alloy::bridge::dead_time::ns(500u),
};

}  // namespace

// ── The phase-to-signal mapping ──────────────────────────────────────────

ALLOY_TEST(a_phase_maps_to_its_own_channel_and_that_channels_complement) {
    // Not a preference — the silicon's. Phase 2's low side is CH2N or the
    // bridge has no dead time between the pins it actually drives, because the
    // generator pairs CHx with CHxN and nothing else. Checked at compile time,
    // where a wrong pairing would otherwise be a plausible-looking mux.
    using alloy::bridge::detail::high_signal;
    using alloy::bridge::detail::low_signal;
    static_assert(high_signal(1) == alloy::signal::ch1);
    static_assert(low_signal(1) == alloy::signal::ch1n);
    static_assert(high_signal(2) == alloy::signal::ch2);
    static_assert(low_signal(2) == alloy::signal::ch2n);
    static_assert(high_signal(3) == alloy::signal::ch3);
    static_assert(low_signal(3) == alloy::signal::ch3n);
    ALLOY_CHECK(true);
}

// ── The bridge comes up dark ─────────────────────────────────────────────

ALLOY_TEST(open_leaves_every_output_off) {
    // THE MOE RULE, at the facade. open() programs the whole block and returns
    // a handle whose pins are dead; a bridge that came up live would be
    // switching before the application had chosen a single duty.
    auto inv = inverter<1>::open(good);
    ALLOY_CHECK_EQ(alloy::hal::log.enables, 1u);
    ALLOY_CHECK(!inv.outputs_enabled());
    ALLOY_CHECK(!alloy::hal::log.moe);

    inv.set_duty<1>(0x8000);
    inv.set_duty<2>(0x4000);
    inv.set_duty<3>(0x2000);
    ALLOY_CHECK(!inv.outputs_enabled());  // still dark, duties and all

    inv.enable_outputs();
    ALLOY_CHECK(inv.outputs_enabled());
    ALLOY_CHECK_EQ(alloy::hal::log.duty[0], std::uint16_t{0x8000});
    ALLOY_CHECK_EQ(alloy::hal::log.duty[1], std::uint16_t{0x4000});
    ALLOY_CHECK_EQ(alloy::hal::log.duty[2], std::uint16_t{0x2000});
    ALLOY_CHECK_EQ(alloy::hal::log.duty[3], std::uint16_t{0});  // phase 4 unbound
}

ALLOY_TEST(open_hands_the_driver_what_the_caller_asked_for) {
    auto inv = inverter<2>::open(good);
    ALLOY_CHECK_EQ(alloy::hal::log.kernel_hz, 64'000'000u);
    ALLOY_CHECK_EQ(alloy::hal::log.cfg.freq_hz, 20'000u);
    ALLOY_CHECK_EQ(alloy::hal::log.phase_count, 3u);
    ALLOY_CHECK(alloy::hal::log.break_enabled);
    ALLOY_CHECK(alloy::hal::log.break_active ==
                alloy::bridge::break_polarity::active_low);
    // The handle reports the dead time the HARDWARE will insert, not the
    // request — here they coincide, which is the point of pinning it at a
    // clock where they do.
    ALLOY_CHECK_EQ(inv.dead_time_ns(), 500u);
    ALLOY_CHECK_EQ(decltype(inv)::phases, 3u);
}

ALLOY_TEST(a_board_with_no_fault_line_says_so_and_still_opens) {
    using unguarded = alloy::bridge::bind<
        fake_timer<3>, fake_clock, alloy::bridge::unprotected,
        alloy::bridge::phase<1, fake_pin<1>, fake_pin<2>>>;
    auto inv = unguarded::open(good);
    ALLOY_CHECK(!alloy::hal::log.break_enabled);
    ALLOY_CHECK_EQ(alloy::hal::log.phase_count, 1u);
    ALLOY_CHECK(!inv.outputs_enabled());
}

// ── The fault path ───────────────────────────────────────────────────────

ALLOY_TEST(acknowledging_a_fault_does_not_re_arm_the_bridge) {
    // The distinction the API exists to force. A compressor that clears its
    // own fault flag and comes straight back is restarting into whatever
    // caused the fault; coming back has to be a separate decision, spelled
    // separately.
    auto inv = inverter<4>::open(good);
    inv.enable_outputs();
    ALLOY_CHECK(inv.outputs_enabled());

    inv.emergency_stop();                       // the software break
    ALLOY_CHECK(inv.faulted());
    ALLOY_CHECK(!inv.outputs_enabled());        // hardware took the outputs down

    inv.acknowledge_fault();
    ALLOY_CHECK(!inv.faulted());
    ALLOY_CHECK(!inv.outputs_enabled());        // …and did NOT bring them back

    inv.enable_outputs();
    ALLOY_CHECK(inv.outputs_enabled());
}

ALLOY_TEST(emergency_stop_takes_the_hardware_break_path) {
    // Not `disable_outputs()` under another name. An emergency stop that was
    // a software MOE clear would depend on this code running; the break
    // generator does not.
    auto inv = inverter<5>::open(good);
    inv.enable_outputs();
    const unsigned before = alloy::hal::log.forced_breaks;
    inv.emergency_stop();
    ALLOY_CHECK_EQ(alloy::hal::log.forced_breaks, before + 1u);

    // …and the ordinary disable is NOT the break path.
    inv.acknowledge_fault();
    inv.enable_outputs();
    inv.disable_outputs();
    ALLOY_CHECK_EQ(alloy::hal::log.forced_breaks, before + 1u);
    ALLOY_CHECK(!inv.faulted());
    ALLOY_CHECK(!inv.outputs_enabled());
}

// ── Layer 1's value admission ────────────────────────────────────────────
//
// Every case below is written through a `volatile` so the value is not a
// constant. That is the RUNTIME arm of each admission, and it is the arm that
// is unconditional.
//
// THE COMPILE-TIME ARM CANNOT BE TESTED FROM A SUITE THAT HAS TO COMPILE, so
// it is tested from outside: scripts/check_compile_errors.py compiles five
// rejected `open_checked<>` calls with -fsyntax-only and asserts on the
// diagnostic text, plus one positive control. What CAN be pinned from here is
// the mechanism those five rest on — see
// `the_config_is_a_usable_template_parameter` below — and the positive
// controls, which are the compile-time cases that must NOT fail.

ALLOY_TEST(a_bridge_with_no_stated_dead_time_is_refused) {
    // The whole reason `config::dead_time` is a three-state type instead of a
    // number: `config{}` must not be openable.
    ALLOY_CHECK(refuses([] {
        // The dead time must be opaque too, not just the frequency. This case
        // is the one where every OTHER field is a compile-time constant, so an
        // optimiser that can see `dead_time` is unstated fires the
        // [[gnu::error]] admission and the file stops compiling — which tests
        // the compile-time arm and destroys the runtime one. (It did exactly
        // that at -O1 the moment `config` grew a field: the header's own note
        // says that admission is "a bonus, not a promise", and this is what
        // that costs.) Leaving the value unknowable keeps the RUNTIME trap —
        // the guarantee this test is about — reachable.
        volatile std::uint32_t freq = 20'000u;
        volatile int state_it = 0;
        alloy::bridge::config c{};
        c.freq_hz = freq;
        if (state_it) { c.dead_time = alloy::bridge::dead_time::ns(500u); }
        (void)inverter<6>::open(c);
    }));
}

ALLOY_TEST(a_dead_time_of_zero_is_refused_separately) {
    // `ns(0)` and "I forgot" are different mistakes with different fixes, so
    // they are different diagnostics. Here they are merely both refused.
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t zero = 0u;
        alloy::bridge::config c{.freq_hz = 20'000u,
                                .dead_time = alloy::bridge::dead_time::ns(zero)};
        (void)inverter<7>::open(c);
    }));
}

ALLOY_TEST(a_dead_time_longer_than_the_generator_is_refused) {
    // The bound is the DEGREE fact — the curated DTG width against this
    // instance's kernel clock — not a constant. At 64 MHz it is 15750 ns.
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t too_long = 15'751u;
        alloy::bridge::config c{.freq_hz = 1'000u,
                                .dead_time = alloy::bridge::dead_time::ns(too_long)};
        (void)inverter<8>::open(c);
    }));
}

ALLOY_TEST(the_longest_admissible_dead_time_is_actually_admitted) {
    // The positive control for the test above, and the one the bound's bug
    // would have broken at the facade: exactly max_dead_time_ns must open.
    auto inv = inverter<9>::open({
        .freq_hz = 1'000u,
        .dead_time = alloy::bridge::dead_time::ns(15'750u),
    });
    ALLOY_CHECK_EQ(inv.dead_time_ns(), 15'750u);
    ALLOY_CHECK_EQ(alloy::hal::log.enables > 0u, true);
}

ALLOY_TEST(a_dead_time_that_swallows_the_switching_period_is_refused) {
    // Both edges of every pair carry the dead time, so two of them have to fit
    // inside one period with room left for a pulse. 8000 ns of dead time at
    // 100 kHz (a 10000 ns period) leaves none.
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t freq = 100'000u;
        alloy::bridge::config c{.freq_hz = freq,
                                .dead_time = alloy::bridge::dead_time::ns(8'000u)};
        (void)inverter<10>::open(c);
    }));
}

ALLOY_TEST(an_impossible_switching_frequency_is_refused) {
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t zero = 0u;
        alloy::bridge::config c{.freq_hz = zero,
                                .dead_time = alloy::bridge::dead_time::ns(500u)};
        (void)inverter<11>::open(c);
    }));
    ALLOY_CHECK(refuses([] {
        volatile std::uint32_t faster_than_the_clock = 65'000'000u;
        alloy::bridge::config c{.freq_hz = faster_than_the_clock,
                                .dead_time = alloy::bridge::dead_time::ns(500u)};
        (void)inverter<12>::open(c);
    }));
}

ALLOY_TEST(a_gate_driver_that_inserts_the_dead_time_is_admitted_when_said_aloud) {
    // The one legitimate way to program zero at the timer. It is spelled long
    // on purpose — a reviewer should see the claim being made — and it must
    // NOT take the ns(0) path.
    auto inv = inverter<13>::open({
        .freq_hz = 20'000u,
        .dead_time = alloy::bridge::dead_time::inserted_by_the_gate_driver(),
    });
    ALLOY_CHECK(alloy::hal::log.cfg.dead_time.stated() ==
                alloy::bridge::dead_time::kind::external);
    ALLOY_CHECK_EQ(alloy::hal::log.cfg.dead_time.requested_ns(), 0u);
    ALLOY_CHECK(!inv.outputs_enabled());
}

ALLOY_TEST(the_dead_time_type_has_exactly_three_states) {
    // The type IS the safety argument, so the states are pinned rather than
    // assumed. A default-constructed config is `unstated` — never zero.
    constexpr alloy::bridge::config fresh{};
    static_assert(fresh.dead_time.stated() ==
                  alloy::bridge::dead_time::kind::unstated);
    static_assert(alloy::bridge::dead_time::ns(500u).requested_ns() == 500u);
    static_assert(alloy::bridge::dead_time::ns(500u).stated() ==
                  alloy::bridge::dead_time::kind::nanoseconds);
    static_assert(alloy::bridge::dead_time::inserted_by_the_gate_driver().stated() ==
                  alloy::bridge::dead_time::kind::external);
    // …and `external` is NOT equal to `ns(0)`, which is what keeps the two
    // diagnostics apart.
    static_assert(alloy::bridge::dead_time::inserted_by_the_gate_driver() !=
                  alloy::bridge::dead_time::ns(0u));
    ALLOY_CHECK(true);
}

// ── The compile-time net's load-bearing mechanism ────────────────────────

namespace {

// The whole of `open_checked<>` rests on `alloy::bridge::config` being a
// STRUCTURAL type ([temp.param]/7): a non-type template parameter of class
// type needs every non-static data member public and non-mutable, all the way
// down. `bridge_dead_time`'s two members are public FOR THIS REASON and
// nothing else, and the comment saying so is not a check. This is.
//
// If someone re-privatises them, every static_assert in open_checked()
// vanishes along with the overload — silently, because open(config) still
// exists and still compiles. This alias fails to instantiate instead.
template <alloy::bridge::config C>
struct needs_a_structural_config {
    static constexpr std::uint32_t freq = C.freq_hz;
};
using structural_witness = needs_a_structural_config<alloy::bridge::config{
    .freq_hz = 20'000u,
    .dead_time = alloy::bridge::dead_time::ns(500u)}>;

}  // namespace

ALLOY_TEST(the_config_is_a_usable_template_parameter) {
    static_assert(structural_witness::freq == 20'000u);
    // …and the members that had to become public did not become a second way
    // to BUILD one: the class has user-declared constructors, so it is not an
    // aggregate, and the only state-setting constructor is still private.
    static_assert(!std::is_aggregate_v<alloy::bridge::dead_time>);
    static_assert(!std::is_constructible_v<alloy::bridge::dead_time,
                                           alloy::bridge::dead_time::kind,
                                           std::uint32_t>);
    ALLOY_CHECK(true);
}

ALLOY_TEST(open_checked_is_open_with_the_config_moved_into_the_template) {
    // The positive control for the five negative compile cases in
    // scripts/check_compile_errors.py: the checked call must still program the
    // block, still come up dark, and still report the same dead time. If this
    // ever diverged from open(), the checked path would be checking a
    // configuration the driver does not receive.
    const unsigned before = alloy::hal::log.enables;
    auto inv = inverter<30>::open_checked<good>();
    ALLOY_CHECK_EQ(alloy::hal::log.enables, before + 1u);
    ALLOY_CHECK_EQ(alloy::hal::log.cfg.freq_hz, 20'000u);
    ALLOY_CHECK(alloy::hal::log.cfg.dead_time == good.dead_time);
    ALLOY_CHECK(!inv.outputs_enabled());
    ALLOY_CHECK_EQ(inv.dead_time_ns(), 500u);
}

ALLOY_TEST(open_checked_admits_the_gate_driver_spelling_and_the_longest_one) {
    // Two boundaries that a wrong static_assert would reject rather than
    // accept, which is the failure a suite of negative cases cannot see.
    // `external` is a legitimate zero at the timer and must not take the
    // ns(0) path; 15750 ns is exactly max_dead_time_ns at 64 MHz.
    auto ext = inverter<31>::open_checked<alloy::bridge::config{
        .freq_hz = 20'000u,
        .dead_time = alloy::bridge::dead_time::inserted_by_the_gate_driver()}>();
    ALLOY_CHECK(alloy::hal::log.cfg.dead_time.stated() ==
                alloy::bridge::dead_time::kind::external);
    ALLOY_CHECK(!ext.outputs_enabled());

    auto longest = inverter<32>::open_checked<alloy::bridge::config{
        .freq_hz = 1'000u,
        .dead_time = alloy::bridge::dead_time::ns(15'750u)}>();
    ALLOY_CHECK_EQ(longest.dead_time_ns(), 15'750u);
}

// ── When each pin becomes the timer's ────────────────────────────────────

ALLOY_TEST(the_phase_pins_are_muxed_after_the_off_state_is_in_force) {
    // THE WINDOW THIS CLOSES. TIM1 leaves reset with BDTR = 0, so OSSI = 0 and
    // MOE = 0, and OSSI = 0 means the outputs are released to the GPIO — while
    // the GPIO is in alternate-function mode, which drives nothing either. A
    // phase pin muxed before BDTR is written is therefore held by NOBODY for
    // the length of the driver's programming sequence, with no dead time
    // programmed yet. If the application had been holding the gate inputs low
    // as plain outputs, make_af() is what releases them.
    //
    // So: break pin (an INPUT, and it wants to be connected before BKE arms
    // it), then the whole driver sequence ending in BDTR, then the six phase
    // pins — which arrive to a peripheral already forcing them to the CR2 idle
    // level.
    alloy::hal::log.seq_n = 0;
    (void)inverter<33>::open(good);

    ALLOY_CHECK_EQ(alloy::hal::log.seq_n, 8u);       // 1 break + enable + 6 phases
    ALLOY_CHECK_EQ(alloy::hal::log.seq[0], 9);       // fake_pin<9> is the brk<> tag
    ALLOY_CHECK_EQ(alloy::hal::log.seq[1], alloy::hal::bridge_log::kEnable);
    bool phases_last = true;
    for (unsigned i = 2; i < 8u; ++i) {
        const int p = alloy::hal::log.seq[i];
        if (p < 1 || p > 6) { phases_last = false; }
    }
    ALLOY_CHECK(phases_last);
}

ALLOY_TEST(a_bridge_with_no_break_pin_still_muxes_its_phases_last) {
    // The `unprotected` path takes a different branch through open(), and the
    // ordering rule is not a property of having a fault line.
    using unguarded = alloy::bridge::bind<
        fake_timer<34>, fake_clock, alloy::bridge::unprotected,
        alloy::bridge::phase<1, fake_pin<1>, fake_pin<2>>>;
    alloy::hal::log.seq_n = 0;
    (void)unguarded::open(good);

    ALLOY_CHECK_EQ(alloy::hal::log.seq_n, 3u);       // enable + 2 phase pins
    ALLOY_CHECK_EQ(alloy::hal::log.seq[0], alloy::hal::bridge_log::kEnable);
    ALLOY_CHECK_EQ(alloy::hal::log.seq[1], 1);
    ALLOY_CHECK_EQ(alloy::hal::log.seq[2], 2);
}

// ── The personality rule ─────────────────────────────────────────────────

ALLOY_TEST(a_bridge_refuses_a_timer_already_generating_pwm) {
    // THE CASE THE THIRD PERSONALITY EXISTS FOR. Both binders are legal C++,
    // both name routed pins, and the second is a program asking one block to
    // be a dead-time-paired bridge and a single-ended waveform generator at
    // once.
    ALLOY_CHECK(refuses([] {
        (void)pwm_on<20>::open({.freq_hz = 1'000});
        (void)inverter<20>::open(good);
    }));
}

ALLOY_TEST(pwm_refuses_a_timer_already_driving_a_bridge) {
    // AND THE OTHER ORDER, which is the dangerous one and is not the same
    // test: `pwm::bind` claims a CHANNEL before it claims the block, so what
    // it would otherwise do here is take CH1 out of the bridge's paired set
    // and drive it single-ended — a high side switching against a low side
    // that no longer has a dead time between them.
    ALLOY_CHECK(refuses([] {
        (void)inverter<21>::open(good);
        (void)pwm_on<21>::open({.freq_hz = 1'000});
    }));
}

ALLOY_TEST(a_bridge_refuses_a_second_binder_on_one_timer) {
    // `exclusive`, not `shared`: unlike a PWM channel, a bridge has no
    // legitimate co-owner. Dead time, the break input, the off-state and MOE
    // are one register for the whole block.
    ALLOY_CHECK(refuses([] {
        (void)inverter<22>::open(good);
        (void)inverter<22>::open(good);
    }));
}

ALLOY_TEST(a_bridge_opens_a_timer_nobody_else_wants) {
    // The positive control. Without it every test above passes on a facade
    // that refuses everything.
    auto inv = inverter<23>::open(good);
    ALLOY_CHECK_EQ(inv.period_ticks(), 1600u);
    ALLOY_CHECK((alloy::claim::held<fake_timer<23>,
                                    alloy::claim::personality::bridge>()));
}

// ── The trigger output ───────────────────────────────────────────────────
//
// A control loop's question is not "can I start a conversion" but WHERE IN
// THE PERIOD it samples: current in a switching leg only means something away
// from the edges, and a conversion the CPU starts lands wherever the CPU got
// to. These pin the FACADE half — the choice reaching the driver, and the
// sample point being its own register. The REGISTER half (that CR2 carries
// MMS and every idle bit stays zero) cannot be reached from here: the driver
// needs a generated alloy/ip/st/tim_adv.hpp, and this file deliberately never
// reads generated headers. It is pinned by the mask assertion in the driver.

ALLOY_TEST(a_bridge_defaults_to_publishing_nothing) {
    // A bridge nobody asked to trigger anything should not broadcast events
    // onto a bus some other peripheral may be slaved to.
    auto inv = inverter<40>::open(good);
    (void)inv;
    ALLOY_CHECK(alloy::hal::log.cfg.trigger == alloy::hal::bridge_trigger::none);
}

ALLOY_TEST(the_trigger_choice_reaches_the_driver) {
    constexpr alloy::bridge::config triggered{
        .freq_hz = 20'000u,
        .dead_time = alloy::bridge::dead_time::ns(500u),
        .trigger = alloy::hal::bridge_trigger::on_compare,
    };
    auto inv = inverter<41>::open(triggered);
    (void)inv;
    ALLOY_CHECK(alloy::hal::log.cfg.trigger ==
                alloy::hal::bridge_trigger::on_compare);
    // …and the fields around it are untouched by the new one.
    ALLOY_CHECK_EQ(alloy::hal::log.cfg.freq_hz, 20'000u);
    ALLOY_CHECK(alloy::hal::log.cfg.align == alloy::hal::bridge_alignment::center);
    ALLOY_CHECK(alloy::hal::log.cfg.when_off ==
                alloy::hal::bridge_off_state::drive_idle_level);
}

ALLOY_TEST(the_sample_point_is_its_own_register) {
    auto inv = inverter<42>::open(good);
    inv.set_sample_point(0xFFFFu);
    ALLOY_CHECK_EQ(alloy::hal::log.sample_point, 0xFFFFu);
    inv.set_sample_point(0u);
    ALLOY_CHECK_EQ(alloy::hal::log.sample_point, 0u);
    // Moving the sample point must not disturb a duty. On a live bridge that
    // would be a torn frame — one phase updated, the others not.
    inv.template set_duty<1>(0x4000u);
    inv.template set_duty<2>(0x8000u);
    inv.set_sample_point(0x2000u);
    ALLOY_CHECK_EQ(alloy::hal::log.duty[0], 0x4000u);
    ALLOY_CHECK_EQ(alloy::hal::log.duty[1], 0x8000u);
    ALLOY_CHECK_EQ(alloy::hal::log.sample_point, 0x2000u);
}

ALLOY_TEST(the_sample_point_is_gated_on_a_generated_number) {
    // The gate is `feat::trgo`, from the IP's curated data — the same shape
    // alloy::tick::trigger_on_update uses. A block whose data says 0 does not
    // get the method at all, which is a compile error naming the instance
    // instead of a register write that quietly does nothing.
    using handle_t = decltype(inverter<43>::open(good));
    static_assert(requires(handle_t h) { h.set_sample_point(0u); },
                  "a timer whose feat::trgo is non-zero must offer this");
    static_assert(fake_timer<43>::feat::trgo != 0u);
    ALLOY_CHECK(true);
}

// ── Update cadence and coherent duty writes ──────────────────────────────

ALLOY_TEST(rep_says_every_period_in_both_alignments) {
    // The asymmetry the config exists to hide: a centre-aligned counter meets
    // its update condition twice per period, so "every period" is a different
    // register value in the two modes. Getting this backwards halves or
    // doubles a control loop's rate silently.
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(1u, true), 1u);   // centre, every period
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(1u, false), 0u);  // edge, every period
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(2u, true), 3u);
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(2u, false), 1u);
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(8u, true), 15u);
}

ALLOY_TEST(rep_of_zero_periods_is_one_not_a_wrap) {
    // 0 periods is nonsense. Unsigned arithmetic would make it 65535 — one
    // update every ~32k periods, a bridge that looks alive and ignores every
    // duty written to it. It must clamp to "every period" instead.
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(0u, true), 1u);
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(0u, false), 0u);
}

ALLOY_TEST(rep_saturates_inside_the_field) {
    // REP is 16 bits on this IP. A request past it must clamp, not truncate:
    // truncation gives a SHORTER cadence than asked, which is the direction
    // that surprises a loop.
    ALLOY_CHECK_EQ(dt::tim_adv_rep_for(40'000u, true), 0xFFFFu);
    ALLOY_CHECK(dt::tim_adv_rep_for(100'000u, false) <= 0xFFFFu);
}

ALLOY_TEST(the_update_cadence_reaches_the_driver) {
    constexpr alloy::bridge::config slow{
        .freq_hz = 20'000u,
        .dead_time = alloy::bridge::dead_time::ns(500u),
        .update_every = 4u,
    };
    auto inv = inverter<44>::open(slow);
    (void)inv;
    ALLOY_CHECK_EQ(alloy::hal::log.cfg.update_every, 4u);
    // …and the default is still one, for a bridge that says nothing.
    ALLOY_CHECK_EQ(good.update_every, 1u);
}

ALLOY_TEST(a_coherent_write_reports_a_frame_that_did_not_straddle) {
    auto inv = inverter<45>::open(good);
    alloy::hal::log.uif = false;
    ALLOY_CHECK(inv.set_duties_coherent(0x1000u, 0x2000u, 0x3000u));
    ALLOY_CHECK_EQ(alloy::hal::log.duty[0], 0x1000u);
    ALLOY_CHECK_EQ(alloy::hal::log.duty[1], 0x2000u);
    ALLOY_CHECK_EQ(alloy::hal::log.duty[2], 0x3000u);
}

ALLOY_TEST(a_coherent_write_reports_a_TORN_frame) {
    // The case the method exists for: an update landed between the first and
    // last write, so two phases carry the new set and one the old. The duties
    // are still written — this reports, it does not roll back — and the caller
    // decides whether to repeat them.
    auto inv = inverter<46>::open(good);
    // The counter reloads while phase 2 is being written: phase 1 is from the
    // new frame, phases 2 and 3 land after the reload. Setting the flag BEFORE
    // the call would prove nothing — the call clears it first, which is what
    // makes the answer about THIS write and not about history.
    alloy::hal::log.tear_on_phase = 2u;
    ALLOY_CHECK(!inv.set_duties_coherent(0x1000u, 0x2000u, 0x3000u));
    // Reported, not rolled back: every duty was still written, and the caller
    // decides whether to repeat them.
    ALLOY_CHECK_EQ(alloy::hal::log.duty[0], 0x1000u);
    ALLOY_CHECK_EQ(alloy::hal::log.duty[2], 0x3000u);
    alloy::hal::log.tear_on_phase = 0u;
}

ALLOY_TEST(a_coherent_write_takes_one_duty_per_phase) {
    // Arity is checked against the bind's phase count, so a bridge cannot be
    // handed the wrong number of duties and quietly drop or invent one.
    //
    // Only the POSITIVE case is asserted here. The negative one is not a
    // requires-expression that evaluates false — clang reports the failed
    // overload as a hard error rather than substituting, so writing it that
    // way breaks the build instead of testing it. Its witness is the compiler
    // diagnostic itself, which names the constraint:
    //     note: because 'sizeof...(Duties) == 3U' (2 == 3) evaluated to false
    // A wrong-arity call belongs in scripts/check_compile_errors.py, where a
    // refusal is the expected result, not here.
    // The handle type is NAMED rather than taken from decltype(open(...)):
    // instantiating open() in an unevaluated context still fires its
    // [[gnu::error]] admissions, so decltype of a call is not free here.
    using three = alloy::bridge::handle<fake_timer<47>, 3u>;
    static_assert(requires(three h) { h.set_duties_coherent(0u, 0u, 0u); },
                  "a three-phase bridge must accept exactly three duties");
    ALLOY_CHECK(true);
}
