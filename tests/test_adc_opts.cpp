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
