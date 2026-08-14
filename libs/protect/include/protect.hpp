// Protection limits — the software half of "trip before something burns".
//
// A measurement crosses a threshold; a fault latches; the machine reacts. That
// is the whole subject, and it is worth a library because the three details
// that make it correct are the three everyone leaves out.
//
//   1. THE THRESHOLD IS IN RAW COUNTS, FOLDED AT COMPILE TIME. Write the limit
//      in volts or amps where a person reads it, and let `counts_for` turn it
//      into an ADC count before the program runs. The fast path is then an
//      integer compare with no floating point anywhere near it.
//
//   2. DEBOUNCE AND HYSTERESIS ARE PART OF A LIMIT, NOT PATCHED ON LATER. A
//      single noisy sample must not trip a machine, and a limit that clears at
//      the same threshold it trips at will chatter across it. Both need state,
//      so both belong in the limit's shape from the start rather than in a
//      wrapper someone remembers to add.
//
//   3. EVERY LIMIT IS EVALUATED, ALWAYS. When four things go wrong at once you
//      want all four in the fault word, not the first one. `observe_all` uses a
//      pack expansion precisely because it cannot short-circuit — writing this
//      with `||` would record one fault and hide three.
//
// PROVENANCE, including what was missing. The compile-time count folding and
// the comparison-as-policy come from a grid-tied inverter's protection module I
// read; they were the right ideas and they were cheap. That module had NO
// debounce and NO hysteresis: one out-of-range sample latched a fault
// permanently, which on a noisy ADC is a nuisance-trip generator. Adding those
// changes the API's shape, which is why they are here and not in a later patch.
//
// A LIMIT IS NOT A SAFETY SYSTEM. Software sampling at tens of kilohertz is far
// too slow to protect power silicon — tens of microseconds is a long time for a
// short circuit. The real trip belongs in hardware (a comparator driving a PWM
// break input, with no CPU in the path). This is the second layer: the one that
// decides what the machine DOES about a fault, and the one you can test.
//
// No allocation, no exceptions, no floating point at run time, constexpr
// throughout — so a whole protection scheme can be evaluated at compile time in
// a test and cost nothing on the device.
#pragma once

#include <array>
#include <bitset>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace alloy::lib::protect {

/// Which side of the threshold is the fault.
enum class edge : std::uint8_t {
    above,  //< trips when the sample rises past `trip`
    below,  //< trips when the sample falls past `trip`
};

/// A physical value as an ADC count, computed at compile time.
///
///     // 150 V full-scale limit on a divider that maps 250 V to 4095 counts
///     constexpr auto kOverVolt = counts_for<std::uint16_t>(150.0, 250.0, 4095);
///
/// `span` is the physical value the converter reads at `full_scale`. Rounding
/// is to nearest, and the result is clamped into [0, full_scale] so a limit
/// written above the measurable range becomes "never" rather than wrapping into
/// a small number — which is the failure that turns a protection into a trip on
/// every sample.
template <std::unsigned_integral T = std::uint16_t>
[[nodiscard]] constexpr T counts_for(double value, double span, T full_scale) {
    if (span <= 0.0 || value <= 0.0) {
        return 0;
    }
    const double scaled = (value / span) * static_cast<double>(full_scale) + 0.5;
    if (scaled >= static_cast<double>(full_scale)) {
        return full_scale;
    }
    return static_cast<T>(scaled);
}

/// How a limit behaves: where it trips, where it lets go, and how many
/// consecutive samples each takes.
template <std::unsigned_integral T = std::uint16_t>
struct threshold {
    T trip{};
    /// Where the limit stops being tripped. Equal to `trip` means no
    /// hysteresis. For `edge::above` this must be <= trip; for `edge::below`,
    /// >= trip. `valid()` states it so a wrong one is a static_assert and not a
    /// machine that never releases.
    T release{};
    /// Consecutive qualifying samples before the state changes. 1 is "react
    /// immediately"; anything more is debounce measured in samples, so the time
    /// it buys is `count / sample_rate` — a number worth writing in the
    /// comment where you set it.
    std::uint8_t trip_after{1};
    std::uint8_t clear_after{1};

    template <edge Edge>
    [[nodiscard]] constexpr bool valid() const {
        if (trip_after == 0 || clear_after == 0) {
            return false;
        }
        return Edge == edge::above ? release <= trip : release >= trip;
    }
};

/// One measurement against one threshold, with debounce and hysteresis.
///
/// Live state, not latched: `tripped()` follows the signal. Latching is the
/// `fault_latch`'s job, deliberately kept separate — a limit answers "is it bad
/// right now", a latch answers "has it been bad since I last looked", and
/// conflating them is how a system ends up unable to tell a present fault from
/// a remembered one.
template <edge Edge, std::unsigned_integral T = std::uint16_t>
class limit {
public:
    constexpr explicit limit(threshold<T> spec) : spec_(spec) {}

    /// Feed one sample. Returns the live state after it.
    constexpr bool update(T sample) {
        const bool qualifies = Edge == edge::above ? sample > spec_.trip
                                                   : sample < spec_.trip;
        const bool releases = Edge == edge::above ? sample < spec_.release
                                                  : sample > spec_.release;
        if (!tripped_) {
            // Counting toward a trip. A sample that does not qualify resets the
            // run — debounce means CONSECUTIVE, and a counter that only ever
            // increments would trip on scattered noise given enough time.
            count_ = qualifies ? static_cast<std::uint8_t>(count_ + 1) : 0;
            if (count_ >= spec_.trip_after) {
                tripped_ = true;
                count_ = 0;
            }
        } else {
            count_ = releases ? static_cast<std::uint8_t>(count_ + 1) : 0;
            if (count_ >= spec_.clear_after) {
                tripped_ = false;
                count_ = 0;
            }
        }
        return tripped_;
    }

    [[nodiscard]] constexpr bool tripped() const { return tripped_; }

    /// Back to untripped, mid-debounce state discarded.
    constexpr void reset() {
        tripped_ = false;
        count_ = 0;
    }

    /// Start already tripped — a test can construct the state it wants to
    /// exercise instead of feeding samples until the machine gets there.
    constexpr void seed(bool tripped, std::uint8_t count = 0) {
        tripped_ = tripped;
        count_ = count;
    }

    [[nodiscard]] constexpr const threshold<T>& spec() const { return spec_; }

private:
    threshold<T> spec_;
    std::uint8_t count_{0};
    bool tripped_{false};
};

/// A window: fault below `low`, fault above `high`, healthy between.
///
/// Two limits rather than a third comparison kind, because that is what it is —
/// and because the two sides usually want different debounce. An undervoltage
/// that must ride through a sag and an overvoltage that must trip at once are
/// the normal case, not the exotic one.
template <std::unsigned_integral T = std::uint16_t>
class window {
public:
    constexpr window(threshold<T> low, threshold<T> high)
        : low_(low), high_(high) {}

    /// Both sides are updated on every sample — never one or the other. A
    /// `window` that skipped the far side would leave its debounce counter
    /// stale, and the first sample after a long excursion would then trip it.
    constexpr bool update(T sample) {
        const bool under = low_.update(sample);
        const bool over = high_.update(sample);
        return under || over;
    }

    [[nodiscard]] constexpr bool under() const { return low_.tripped(); }
    [[nodiscard]] constexpr bool over() const { return high_.tripped(); }
    [[nodiscard]] constexpr bool tripped() const { return under() || over(); }

    constexpr void reset() {
        low_.reset();
        high_.reset();
    }

private:
    limit<edge::below, T> low_;
    limit<edge::above, T> high_;
};

/// Latched faults: N channels, one bit each, cleared only by an explicit
/// acknowledge.
///
/// A latch is what makes a fault survive long enough to be acted on and
/// reported. The live limit may release a millisecond later; the record of what
/// happened must not.
template <std::size_t N>
class fault_latch {
public:
    /// Optional reaction, run once on the edge where a bit latches.
    ///
    /// A plain function pointer, not std::function: no allocation, no
    /// indirection through a type-erased wrapper, and it fits in a constexpr
    /// array so the whole table can live in flash.
    using reaction = void (*)();

    constexpr fault_latch() = default;
    constexpr explicit fault_latch(std::array<reaction, N> reactions)
        : reactions_(reactions) {}

    /// Record one channel's live state. Returns whether it is latched now.
    constexpr bool observe(std::size_t index, bool live) {
        if (index >= N) {
            return false;
        }
        if (live && !latched_[index]) {
            latched_.set(index);
            if (reactions_[index] != nullptr) {
                reactions_[index]();
            }
        }
        return latched_[index];
    }

    /// Record EVERY channel at once, in argument order.
    ///
    /// The pack expansion is the point: it cannot short-circuit, so four
    /// simultaneous faults produce four set bits. The obvious alternative —
    /// `a() || b() || c()` — records the first and hides the rest, and reads
    /// so naturally that it is the standard way this goes wrong.
    template <std::same_as<bool>... Live>
        requires(sizeof...(Live) == N)
    constexpr bool observe_all(Live... live) {
        std::size_t index = 0;
        (observe(index++, live), ...);
        return any();
    }

    [[nodiscard]] constexpr bool latched(std::size_t index) const {
        return index < N && latched_[index];
    }
    [[nodiscard]] constexpr const std::bitset<N>& faults() const { return latched_; }
    [[nodiscard]] constexpr bool any() const { return latched_.any(); }
    [[nodiscard]] constexpr std::size_t count() const { return latched_.count(); }

    /// Acknowledge. Deliberately all-or-nothing per channel and never
    /// automatic: a fault that clears itself is one nobody has to explain.
    constexpr void clear(std::size_t index) {
        if (index < N) {
            latched_.reset(index);
        }
    }
    constexpr void clear_all() { latched_.reset(); }

    static constexpr std::size_t channels = N;

private:
    std::bitset<N> latched_{};
    std::array<reaction, N> reactions_{};
};

}  // namespace alloy::lib::protect
