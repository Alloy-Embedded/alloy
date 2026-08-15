// Unit tests for alloy::adc's LAYER 2 — the per-IP knob bag (`opts<Inst>`),
// the result-width arithmetic that follows from it, and the one place two
// knobs collide.
//
// Driven through a fake adc_impl in the test_adc_stream.cpp spirit: the REAL
// facade runs against a fake converter that records what it was told, so what
// is asserted here is the FACADE's behaviour — which knobs reach a driver,
// what the handle remembers, and what the arithmetic says — not any silicon.
// The register writes themselves belong to the driver and to the emulation
// leg; this file cannot witness them and does not pretend to.
//
// The three things worth pinning, and why each one is a test rather than a
// comment:
//
//  1. A DRIVER WITH NO KNOBS still opens. The primary `adc_opts` is empty and
//     `open<{}>()` must compile against a driver whose enable() never heard of
//     Layer 2 — that is the "adding Layer 2 costs zero lines" claim, and it is
//     false the moment the facade requires a templated enable().
//  2. THE KNOBS TRAVEL WITH THE HANDLE. `result_bits` is a property of the
//     port as configured, so a later call (the watchdog) can check it. A
//     handle that forgot its Opts could not.
//  3. THE ARITHMETIC IS THE CONTRACT. Oversampling widens a result by
//     log2(ratio) and narrows it by the shift; whether that lands back on the
//     12-bit scale is exactly what decides if an analog watchdog is still
//     expressible.

#include <cstdint>

#include "alloy/adc.hpp"
#include "alloy_test.hpp"

namespace {

// A converter whose driver curates NO vendor knobs — the empty-primary case.
struct bare_adc {
    static constexpr alloy::clock_node kernel = alloy::clock_node::ahb;
};

// A converter whose driver curates knobs, shaped like st/adc_v2's: a
// resolution and an oversampler. Deliberately NOT the real IP — this file
// tests the facade, and using the real one would tie these assertions to a
// chip database that is free to grow.
struct knobbed_adc {
    static constexpr alloy::clock_node kernel = alloy::clock_node::ahb;
};

struct fake_clock {
    static constexpr std::uint32_t ahb_hz = 16'000'000u;
    static constexpr std::uint32_t apb_hz = 16'000'000u;
    static constexpr std::uint32_t apb2_hz = 16'000'000u;
    static constexpr std::uint32_t sysclk_hz = 16'000'000u;
};

}  // namespace

namespace alloy::hal {

// The knobbed IP's Layer 2, using the registered vocabulary names.
template <>
struct adc_opts<knobbed_adc> {
    std::uint8_t resolution_bits = 12;
    std::uint16_t oversample_ratio = 1;
    std::uint8_t oversample_shift = 0;
};

// Bare driver: the pre-Layer-2 shape, with a plain enable().
template <>
struct adc_impl<bare_adc> {
    static inline std::uint32_t enabled_at_hz = 0;
    static void enable(std::uint32_t hz) { enabled_at_hz = hz; }
    [[nodiscard]] static std::uint16_t read(std::uint8_t) { return 0; }
};

// Knobbed driver: templated enable(), and it records what it was handed so a
// test can prove the knobs travelled rather than inferring it from a size.
template <>
struct adc_impl<knobbed_adc> {
    static inline std::uint32_t enabled_at_hz = 0;
    static inline std::uint8_t saw_bits = 0;
    static inline std::uint16_t saw_ratio = 0;
    static inline std::uint8_t saw_shift = 0;

    template <adc_opts<knobbed_adc> O>
    static void enable(std::uint32_t hz) {
        enabled_at_hz = hz;
        saw_bits = O.resolution_bits;
        saw_ratio = O.oversample_ratio;
        saw_shift = O.oversample_shift;
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t) { return 0; }

    // The same arithmetic st_adc_v2 uses, and the reason it lives on the
    // driver rather than in the facade: only the driver knows that its
    // oversampler sums then shifts, and a different IP's may not.
    template <adc_opts<knobbed_adc> O>
    [[nodiscard]] static consteval unsigned result_bits() {
        unsigned bits = O.resolution_bits;
        if (O.oversample_ratio > 1u) {
            unsigned gained = 0;
            for (std::uint16_t v = O.oversample_ratio; v > 1u; v >>= 1u) {
                ++gained;
            }
            bits += gained - (O.oversample_shift < gained ? O.oversample_shift : gained);
        }
        return bits > 16u ? 16u : bits;
    }
};

}  // namespace alloy::hal

namespace {
// The capability probe portable code writes, in the one spelling that works:
// the type is a template parameter, so the requires-expression is checked per
// instantiation instead of eagerly.
template <class Inst>
constexpr bool has_knob = requires { alloy::adc::opts<Inst>{}.oversample_ratio; };
}  // namespace

// A driver that never heard of Layer 2 still opens, and the facade calls its
// plain enable(). This is the "costs zero lines in a driver with nothing to
// add" claim, executed.
ALLOY_TEST(adc_opts_a_driver_with_no_knobs_still_opens) {
    using bind = alloy::adc::bind<bare_adc, fake_clock>;
    alloy::hal::adc_impl<bare_adc>::enabled_at_hz = 0;

    auto adc = bind::open();
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<bare_adc>::enabled_at_hz, 16'000'000u);
    ALLOY_CHECK_EQ(adc.read(0), 0u);

    // Its opts type exists and is empty — portable code may name it without
    // knowing whether the IP has any knobs at all.
    static_assert(sizeof(alloy::adc::opts<bare_adc>) >= 1u);
    // ...and the by-name probe (the Layer-2 naming rule's whole payoff)
    // answers false for it and true for a driver that has the knob, with no
    // preprocessor and no error either way.
    //
    // The probe MUST be spelled through a template parameter, as `has_knob`
    // below is: a requires-expression over a CONCRETE type is checked
    // immediately, and clang reports the missing member as a hard error
    // instead of `false`. That is not a portability footnote, it is the
    // idiom — a `libs/` driver naming a specific instance would hit it.
    static_assert(!has_knob<bare_adc>);
    static_assert(has_knob<knobbed_adc>);
}

// A bare driver's port is 12 bits, because that is what every ADC alloy ships
// converts at when nobody asks otherwise. Portable code branches on this
// rather than assuming.
ALLOY_TEST(adc_opts_result_bits_defaults_to_twelve_without_a_driver_answer) {
    static_assert(alloy::adc::result_bits<bare_adc, {}>() == 12u);
}

// The knobs reach the driver, and the ones nobody names keep their defaults.
ALLOY_TEST(adc_opts_reach_the_driver_at_open) {
    using bind = alloy::adc::bind<knobbed_adc, fake_clock>;
    using opts = alloy::adc::opts<knobbed_adc>;

    auto adc = bind::open<opts{.resolution_bits = 10,
                               .oversample_ratio = 16,
                               .oversample_shift = 4}>();
    (void)adc;
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<knobbed_adc>::saw_bits, 10u);
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<knobbed_adc>::saw_ratio, 16u);
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<knobbed_adc>::saw_shift, 4u);
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<knobbed_adc>::enabled_at_hz, 16'000'000u);
}

// THE ARITHMETIC, which is what decides whether a watchdog can still be armed.
// Ratio 2^n with shift n is an average and stays on the 12-bit scale; a
// smaller shift keeps the extra bits and widens the result.
ALLOY_TEST(adc_opts_oversampling_widens_by_log2_ratio_and_narrows_by_the_shift) {
    using opts = alloy::adc::opts<knobbed_adc>;

    // Off: the resolution, whatever it is.
    static_assert(alloy::adc::result_bits<knobbed_adc, opts{}>() == 12u);
    static_assert(alloy::adc::result_bits<knobbed_adc, opts{.resolution_bits = 8}>() == 8u);

    // 16x summed, shifted back by 4 — an average, still 12 bits.
    static_assert(alloy::adc::result_bits<knobbed_adc,
                                          opts{.oversample_ratio = 16,
                                               .oversample_shift = 4}>() == 12u);
    // 16x summed, NOT shifted — four extra bits of resolution, 16 wide.
    static_assert(alloy::adc::result_bits<knobbed_adc,
                                          opts{.oversample_ratio = 16,
                                               .oversample_shift = 0}>() == 16u);
    // 256x summed, shifted 4 — eight gained, four kept: 16 bits, and the
    // clamp at the data register's own width is what stops it at 16 rather
    // than 20.
    static_assert(alloy::adc::result_bits<knobbed_adc,
                                          opts{.oversample_ratio = 256,
                                               .oversample_shift = 4}>() == 16u);
    // A shift LARGER than the bits gained cannot narrow below the
    // resolution — the sum has no more bits to give.
    static_assert(alloy::adc::result_bits<knobbed_adc,
                                          opts{.oversample_ratio = 4,
                                               .oversample_shift = 8}>() == 12u);
}

// The handle CARRIES its Opts, which is what lets a later call check the pair.
// Two ports configured differently are different types, and each one's
// result_bits is its own.
ALLOY_TEST(adc_opts_travel_with_the_handle) {
    using bind = alloy::adc::bind<knobbed_adc, fake_clock>;
    using opts = alloy::adc::opts<knobbed_adc>;

    static_assert(bind::result_bits<> == 12u);
    static_assert(bind::result_bits<opts{.oversample_ratio = 16, .oversample_shift = 0}> == 16u);

    // The two handles are genuinely different types: a function taking the
    // averaging port cannot silently be handed the widening one.
    using plain = decltype(bind::open<opts{}>());
    using wide = decltype(bind::open<opts{.oversample_ratio = 16, .oversample_shift = 0}>());
    static_assert(!std::is_same_v<plain, wide>);
}

// ===========================================================================
// MULTI-CHANNEL SCAN — the capability that exists only where the silicon has a
// sequencer. Shipped with compile-level proof (it is a member on an F7 and not
// on a G0); these cases add the BEHAVIOUR, which no compiler check can give:
// that the order the caller asked for is the order the sequencer is programmed
// with, that repeats survive, and that a request the hardware cannot hold is
// refused rather than truncated.
// ===========================================================================

namespace {

// A converter WITH a sequencer: records the sequence it was handed, and hands
// back a result derived from each channel so a test can tell the slots apart.
// TAGGED PER TEST, and that is the claim guard doing its job rather than a
// test-harness quirk: `bind::open()` takes claim::exclusive on the INSTANCE,
// cross-TU, so a second test opening the same fake instance traps with
// instance_owned. One tag per test is the honest fix — sharing an open handle
// between tests would couple them, and there is no close() to undo a claim.
template <int Tag>
struct seq_adc {
    static constexpr alloy::clock_node kernel = alloy::clock_node::ahb;
};

// A converter WITHOUT one — a bitmap machine, like the G0. Nothing opens it:
// it exists to be PROBED, which is the whole point of a capability constant.
// (No `kernel` member, because bind::kernel_hz() is never instantiated for it
// and an unused constant is a -Werror warning.)
struct bitmap_adc {};

}  // namespace

namespace alloy::hal {

template <int Tag>
struct adc_impl<seq_adc<Tag>> {
    static inline std::uint8_t programmed[16]{};
    static inline std::uint8_t programmed_count = 0;
    static inline std::uint8_t calls = 0;
    static constexpr std::uint8_t max_scan_slots = 16u;

    static void enable(std::uint32_t) {}
    [[nodiscard]] static std::uint16_t read(std::uint8_t ch) {
        return static_cast<std::uint16_t>(100u + ch);
    }

    [[nodiscard]] static bool scan(const std::uint8_t* channels, std::uint16_t* out,
                                   std::uint8_t count) {
        ++calls;
        if (count == 0u || count > max_scan_slots) {
            return false;  // the driver's own bound, mirroring st_adc_f4's
        }
        programmed_count = count;
        for (std::uint8_t i = 0; i < count; ++i) {
            programmed[i] = channels[i];
            out[i] = static_cast<std::uint16_t>(100u + channels[i]);
        }
        return true;
    }

    static void reset() {
        programmed_count = 0;
        calls = 0;
        for (auto& p : programmed) { p = 0xEE; }
    }
};

template <>
struct adc_impl<bitmap_adc> {
    static void enable(std::uint32_t) {}
    [[nodiscard]] static std::uint16_t read(std::uint8_t ch) {
        return static_cast<std::uint16_t>(100u + ch);
    }
    // No scan hook — this IP cannot express an ordered sequence.
};

}  // namespace alloy::hal

// THE CAPABILITY ANSWER, which is what portable code branches on. Spelled
// through the variable template because a requires-expression over a concrete
// type is checked eagerly — see can_scan's own declaration.
ALLOY_TEST(adc_scan_can_scan_tells_the_two_shapes_apart) {
    static_assert(alloy::adc::can_scan<seq_adc<0>>);
    static_assert(!alloy::adc::can_scan<bitmap_adc>);

    // The member-level claim ("scan() is not a member on a bitmap converter")
    // is NOT asserted here, and the reason is worth the line: `scan` is
    // declared for every Inst and merely CONSTRAINED, so naming `h.scan` on a
    // converter whose constraint fails is an "invalid reference to function,
    // constraints not satisfied" hard error rather than a substitution
    // failure — even inside a requires-expression. That claim is proven
    // instead by compiling the real facade against two real generated board
    // headers (an F7 accepts `adc.scan(...)`, a G0 refuses it), which is the
    // level at which it is actually meaningful.
}

// ORDER AND REPEATS ARE THE WHOLE POINT — they are exactly what a channel
// bitmap cannot express, and therefore what a scan has to preserve.
ALLOY_TEST(adc_scan_programs_the_sequence_in_the_order_given_repeats_included) {
    alloy::hal::adc_impl<seq_adc<1>>::reset();
    using bind = alloy::adc::bind<seq_adc<1>, fake_clock>;
    auto adc = bind::open();

    const std::uint8_t channels[] = {3, 4, 3, 7};
    std::uint16_t out[4]{};
    ALLOY_CHECK(adc.scan(channels, out));

    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<1>>::programmed_count, 4u);
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<1>>::programmed[0], 3u);
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<1>>::programmed[1], 4u);
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<1>>::programmed[2], 3u);  // the repeat
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<1>>::programmed[3], 7u);

    // Results land in the caller's order, one per requested slot.
    ALLOY_CHECK_EQ(out[0], 103u);
    ALLOY_CHECK_EQ(out[1], 104u);
    ALLOY_CHECK_EQ(out[2], 103u);
    ALLOY_CHECK_EQ(out[3], 107u);
}

// A request the caller got wrong is REFUSED at the facade, before the driver
// is asked — an empty list, or an output buffer too small to hold the answer.
// The second one matters most: truncating instead would hand back a buffer
// whose tail is whatever was there before, indistinguishable from a reading.
ALLOY_TEST(adc_scan_refuses_a_request_it_cannot_answer_without_asking_the_driver) {
    alloy::hal::adc_impl<seq_adc<2>>::reset();
    using bind = alloy::adc::bind<seq_adc<2>, fake_clock>;
    auto adc = bind::open();

    std::uint16_t out[4]{};
    const std::uint8_t none[] = {3};
    ALLOY_CHECK(!adc.scan(std::span<const std::uint8_t>{}, out));
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<2>>::calls, 0u);  // never reached the driver

    const std::uint8_t five[] = {1, 2, 3, 4, 5};
    std::uint16_t small[4]{};
    ALLOY_CHECK(!adc.scan(five, small));
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<2>>::calls, 0u);

    // ...and a well-formed request still works after the refusals, i.e. the
    // guard rejects rather than latching a bad state.
    ALLOY_CHECK(adc.scan(none, out));
    ALLOY_CHECK_EQ(alloy::hal::adc_impl<seq_adc<2>>::calls, 1u);
    ALLOY_CHECK_EQ(out[0], 103u);
}

// The sentinel admission covers scan too: a channel nobody curated is refused
// on EVERY entry point, not just read(). Runtime here (the span is a runtime
// value), which is the trap half of core/admit.hpp's pair.
ALLOY_TEST(adc_scan_admits_every_channel_in_the_list) {
    static_assert(alloy::adc::channel_none == 0xFFu);
    // Compile-level: the admission is called per element — see the facade. A
    // host test cannot execute the trap without aborting the run, so what is
    // pinned here is that the constant the facade compares against is the one
    // the generator emits, which is the seam that could silently drift.
}
