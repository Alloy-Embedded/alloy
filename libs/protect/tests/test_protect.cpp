// Protection limits: the debounce and hysteresis the original lacked, the
// compile-time count folding, and the one that matters most — that four
// simultaneous faults produce four bits and not one.
//
// Most of this is static_assert, because the whole library is constexpr and a
// protection scheme you can evaluate at compile time is one you can be sure of
// before the board is powered.
#include "protect.hpp"

#include "alloy_test.hpp"

namespace {
using namespace alloy::lib::protect;

// ── counts_for: the physical limit becomes an integer before the program runs
static_assert(counts_for<std::uint16_t>(150.0, 250.0, 4095) == 2457);
static_assert(counts_for<std::uint16_t>(0.0, 250.0, 4095) == 0);
// A limit written above the measurable range clamps to full scale — "never
// trips" — rather than wrapping into a small count that trips on every sample.
static_assert(counts_for<std::uint16_t>(400.0, 250.0, 4095) == 4095);
// A nonsensical span cannot produce a division by zero or a huge count.
static_assert(counts_for<std::uint16_t>(150.0, 0.0, 4095) == 0);

// ── a threshold knows whether it makes sense
static_assert(threshold<std::uint16_t>{.trip = 100, .release = 90}
                  .valid<edge::above>());
// Release ABOVE trip on a rising limit would latch and never let go.
static_assert(!threshold<std::uint16_t>{.trip = 100, .release = 110}
                   .valid<edge::above>());
static_assert(threshold<std::uint16_t>{.trip = 100, .release = 110}
                  .valid<edge::below>());
// A zero debounce count would mean "trip after no samples".
static_assert(!threshold<std::uint16_t>{.trip = 100, .release = 90, .trip_after = 0}
                   .valid<edge::above>());

// ── debounce: N CONSECUTIVE samples, not N samples ever
consteval bool debounce_needs_a_run() {
    limit<edge::above> l{{.trip = 100, .release = 90, .trip_after = 3}};
    l.update(150);
    l.update(150);
    if (l.tripped()) return false;   // two of three
    l.update(150);
    if (!l.tripped()) return false;  // the third does it
    return true;
}
static_assert(debounce_needs_a_run());

consteval bool scattered_noise_never_trips() {
    limit<edge::above> l{{.trip = 100, .release = 90, .trip_after = 3}};
    // Alternating over/under, forever. A counter that only incremented would
    // trip here; a run counter must not.
    for (int i = 0; i < 200; ++i) {
        l.update(i % 2 ? 150 : 50);
        if (l.tripped()) return false;
    }
    return true;
}
static_assert(scattered_noise_never_trips());

// ── hysteresis: it lets go at the release threshold, not at the trip one
consteval bool hysteresis_holds_between_the_thresholds() {
    limit<edge::above> l{{.trip = 100, .release = 90}};
    l.update(150);
    if (!l.tripped()) return false;
    l.update(95);                    // below trip, above release
    if (!l.tripped()) return false;  // still tripped — this is the whole point
    l.update(85);                    // below release
    if (l.tripped()) return false;
    return true;
}
static_assert(hysteresis_holds_between_the_thresholds());

consteval bool no_hysteresis_when_thresholds_are_equal() {
    limit<edge::above> l{{.trip = 100, .release = 100}};
    l.update(150);
    if (!l.tripped()) return false;
    l.update(99);
    return !l.tripped();
}
static_assert(no_hysteresis_when_thresholds_are_equal());

// A falling limit is the mirror image, and the release is ABOVE the trip.
consteval bool below_edge_mirrors() {
    limit<edge::below> l{{.trip = 100, .release = 110}};
    l.update(50);
    if (!l.tripped()) return false;
    l.update(105);                   // above trip, below release
    if (!l.tripped()) return false;
    l.update(115);
    return !l.tripped();
}
static_assert(below_edge_mirrors());

// ── clearing is debounced too, and its counter also demands a run
consteval bool clearing_is_debounced() {
    limit<edge::above> l{{.trip = 100, .release = 90, .trip_after = 1,
                          .clear_after = 3}};
    l.update(150);
    l.update(50);
    l.update(50);
    if (!l.tripped()) return false;  // two of three below release
    l.update(50);
    return !l.tripped();
}
static_assert(clearing_is_debounced());

// ── window: two sides, each with its own debounce
consteval bool window_trips_on_both_sides() {
    window<std::uint16_t> w{{.trip = 100, .release = 110},   // under
                            {.trip = 900, .release = 890}};  // over
    w.update(500);
    if (w.tripped()) return false;
    w.update(50);
    if (!w.under() || w.over()) return false;
    w.update(950);
    if (!w.over()) return false;
    return true;
}
static_assert(window_trips_on_both_sides());

// ── THE ONE THAT MATTERS: every channel is evaluated, always
consteval bool all_simultaneous_faults_are_recorded() {
    fault_latch<4> faults;
    faults.observe_all(true, false, true, true);
    return faults.count() == 3 && faults.latched(0) && !faults.latched(1) &&
           faults.latched(2) && faults.latched(3);
}
static_assert(all_simultaneous_faults_are_recorded());

// ── a latch outlives the condition
consteval bool a_latch_survives_the_signal_clearing() {
    fault_latch<2> faults;
    faults.observe(0, true);
    faults.observe(0, false);   // the limit released
    if (!faults.latched(0)) return false;  // the record must not
    faults.clear(0);
    return !faults.any();
}
static_assert(a_latch_survives_the_signal_clearing());

consteval bool out_of_range_channels_are_ignored_not_undefined() {
    fault_latch<2> faults;
    faults.observe(7, true);
    faults.clear(7);
    return !faults.any() && !faults.latched(7);
}
static_assert(out_of_range_channels_are_ignored_not_undefined());

// ── seeding, which the original's file-scope state could not express
consteval bool a_test_can_start_mid_fault() {
    limit<edge::above> l{{.trip = 100, .release = 90, .clear_after = 2}};
    l.seed(true);
    if (!l.tripped()) return false;
    l.update(50);
    l.update(50);
    return !l.tripped();
}
static_assert(a_test_can_start_mid_fault());

int reaction_count = 0;

}  // namespace

ALLOY_TEST(protect_reaction_runs_once_on_the_latching_edge) {
    // A plain function pointer, so the table costs nothing and lives in flash.
    reaction_count = 0;
    fault_latch<2> faults{{+[] { ++reaction_count; }, nullptr}};

    faults.observe(0, true);
    ALLOY_CHECK_EQ(reaction_count, 1);
    faults.observe(0, true);   // still faulted — but not a NEW fault
    ALLOY_CHECK_EQ(reaction_count, 1);

    faults.clear(0);
    faults.observe(0, true);   // acknowledged, then faulted again
    ALLOY_CHECK_EQ(reaction_count, 2);
}

ALLOY_TEST(protect_a_realistic_scheme_composes) {
    // 250 V full scale on the divider; trip at 150 V, release at 145 V, and
    // require three consecutive samples so a single spike does not stop a motor.
    constexpr auto kTrip = counts_for<std::uint16_t>(150.0, 250.0, 4095);
    constexpr auto kRelease = counts_for<std::uint16_t>(145.0, 250.0, 4095);
    limit<edge::above> overvolt{
        {.trip = kTrip, .release = kRelease, .trip_after = 3}};
    fault_latch<1> faults;

    for (int i = 0; i < 3; ++i) {
        faults.observe(0, overvolt.update(3000));  // ~183 V
    }
    ALLOY_CHECK(overvolt.tripped());
    ALLOY_CHECK(faults.latched(0));

    // Back to 120 V: the limit releases, the record does not.
    overvolt.update(counts_for<std::uint16_t>(120.0, 250.0, 4095));
    ALLOY_CHECK(!overvolt.tripped());
    ALLOY_CHECK(faults.latched(0));
}
