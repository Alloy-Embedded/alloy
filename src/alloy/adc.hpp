// User-facing ADC: blocking single conversions, raw 12-bit results.
//
//   auto adc = board::adc::open();
//   std::uint16_t raw = adc.read(board::adc_vref_channel);
//
// Internal channels (VREFINT, temperature) come from chip data via
// generated board constants, so probes stay portable with zero wiring.
//
// ── and the ANALOG WATCHDOG, which is not a knob on open() ────────────────
//
//   auto wd = adc.watchdog<0>({.channel = 3, .low = 1000, .high = 3000});
//   ...
//   if (wd.tripped()) { wd.clear(); }
//
// It exists N times inside one converter, each with its own enable, its own
// window and its own flag, so it gets its own handle indexed against the
// generated count `Inst::feat::analog_watchdogs` — `watchdog<3>()` on an ADC
// with three of them is a compile error with both numbers in it, and an ADC
// whose data records no count at all is a compile error naming the instance
// rather than a silent zero.
//
// THREE THINGS THE SILICON MADE TRUE THAT A GENERIC "SUB-RESOURCE" MODEL DOES
// NOT PREDICT, all three visible in this header's shape:
//
//  1. The guarded CHANNEL is part of ARMING, not a later call. On ST's adc_v2
//     watchdog 1 keeps its channel in the same register as its enable
//     (CFGR1.AWD1CH next to AWD1EN), and ST's own LL driver documents both as
//     writable only with the ADC DISABLED. So there is no `guard(channel)`
//     that a running port could honour, and `watchdog_config` carries the
//     channel next to the window.
//  2. Arming therefore CYCLES THE PORT — the driver stops any conversion,
//     disables the ADC, programs the watchdog and re-enables it. A sub-resource
//     whose lifetime was supposed to be independent of the port's turns out to
//     interrupt it.
//  3. The N of them are NOT interchangeable. Watchdog 1 guards one channel or
//     all of them; 2 and 3 guard an arbitrary channel SET. Layer 1 here is the
//     intersection — one channel — because that is the only shape all three
//     can honour, and a portable field that only works at index 0 would be
//     exactly the lie this surface exists to remove.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/types.hpp"
#include "alloy/dma.hpp"
#include "alloy/hal/adc/adc_impl.hpp"

namespace alloy::adc {

struct config {};

// Layer 1 for one analog watchdog: the values every driver that HAS a watchdog
// can program, in units the caller already has.
//
// `low`/`high` are RAW COUNTS on the same scale `read()` returns — inclusive
// bounds, a sample strictly outside them trips.
//
// THAT SENTENCE USED TO CARRY A CAVEAT THAT IS NOW OBSOLETE, and it is
// rewritten rather than annotated because a stale caveat is worse than none.
// It said the scale question could not arise, "no alloy adc driver turns
// oversampling on, and adc_v2's oversampling fields are deliberately not
// curated". Both halves stopped being true when Layer 2 landed: the fields are
// curated and `opts::oversample_ratio` turns the oversampler on. What keeps
// the equivalence honest now is `handle::watchdog()`, which refuses to arm at
// all when the configured `result_bits` exceed the 12-bit threshold field —
// so within this facade the two units still cannot diverge, but the reason is
// a refusal you can read rather than an absence you had to trust.
//
// Declared in hal/adc/adc_impl.hpp — the shared-vocabulary header both this
// facade and every driver include — and named here, so a user never types
// `hal`.
using watchdog_config = hal::adc_watchdog_config;

// Layer 2 — the vendor knobs this converter's IP actually has, as a
// compile-time value. Empty on a driver that curates none, so
// `open<Role::opts{}>()` compiles everywhere and costs nothing.
//
//   auto adc = board::adc::open<board::adc::opts{
//                  .resolution_bits  = 12,
//                  .oversample_ratio = 16,   // average 16 samples…
//                  .oversample_shift = 4,    // …back onto the 12-bit scale
//              }>();
//
// Spell the type at the call site (`Role::opts{…}`, not `open<{…}>()`): six
// characters buy a diagnostic that names the member instead of "template
// argument deduction/substitution failed". See the Layer-2 section of
// docs/reference/peripheral-surface.md.
template <class Inst>
using opts = hal::adc_opts<Inst>;

// The channel number a board emits for an internal source its chip data does
// not map — see hal/adc/adc_impl.hpp for why the symbol has to exist at all
// and why it carries an impossible value instead of being absent.
using hal::adc_channel_none;
inline constexpr std::uint8_t channel_none = hal::adc_channel_none;

namespace detail {
// Same two mechanisms as alloy/core/admit.hpp's other four: the named trap is
// the guarantee, and the compile error is a bonus that fires only when the
// optimizer propagates the constant (measured matrix in admit.hpp — a debug
// build does not get it). Spelled out here rather than behind a helper for
// the reason admit.hpp gives — a wrapper folds on GCC and not on clang.
inline void admit_channel(std::uint8_t channel) {
    const bool ok = channel != channel_none;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::adc_channel();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

inline void admit_window(std::uint16_t low, std::uint16_t high) {
    const bool ok = low <= high;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::adc_watchdog_window();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}
}  // namespace detail

// One analog watchdog of one converter. Move-only and obtained from the port's
// handle, so the ordinal is claimed exactly once per image
// (claim::sub_exclusive) and a second claimant traps instead of silently
// re-pointing the first one's window.
template <class Inst, unsigned N>
class watchdog {
public:
    watchdog(const watchdog&) = delete;
    watchdog& operator=(const watchdog&) = delete;
    watchdog(watchdog&&) noexcept = default;
    watchdog& operator=(watchdog&&) noexcept = default;

    // Has a conversion fallen outside the window since the last clear()?
    // Sticky in the silicon, so it survives however long the caller waits.
    [[nodiscard]] bool tripped() const {
        return hal::adc_impl<Inst>::template awd_tripped<N>();
    }

    // Acknowledge the trip. Nothing else clears it.
    void clear() const { hal::adc_impl<Inst>::template awd_clear<N>(); }

    // Point this watchdog at a different channel or window. One operation, not
    // a `set_window` and a `guard`: the channel and the enable share a register
    // whose write conditions are the strictest of the two, so splitting them
    // would offer a cheap call that is not cheap.
    void rearm(watchdog_config cfg) const {
        detail::admit_channel(cfg.channel);
        detail::admit_window(cfg.low, cfg.high);
        hal::adc_impl<Inst>::template awd_arm<N>(cfg);
    }

    // Stop guarding. The ordinal stays claimed — this handle IS the resource
    // and alloy has no close(); re-point it with rearm() instead.
    void disarm() const { hal::adc_impl<Inst>::template awd_disarm<N>(); }

private:
    template <class I, class R, opts<I> O>
    friend class handle;
    watchdog() = default;
};

// What the converter+route pair must offer before `adc.ring()` exists on it:
// the driver's DMA-burst hooks (begin/kick/end + the data-register address)
// AND a ring-capable controller behind the route (circular mode, half events,
// live remaining-count — dma::ring_capable). Checked as ONE concept so the
// facade's requires-gate names the whole contract, and checked inside a
// requires-expression so a board whose binder carries no route (ConvRoute =
// void) fails the probe instead of the build.
template <class Inst, class Route>
concept stream_capable = requires {
    hal::adc_impl<Inst>::dma_burst_begin(std::uint8_t{});
    hal::adc_impl<Inst>::dma_burst_kick();
    hal::adc_impl<Inst>::dma_burst_end();
    hal::adc_impl<Inst>::dr_addr();
    typename Route::controller;
    requires dma::ring_capable<typename Route::controller, Route::channel>;
};

// One continuously-converting channel streamed into a caller-owned ring by
// DMA — the design's anchor 2.1 (docs/design/dma-streams.md §2.1). The CPU
// never touches the sampling path: the converter is programmed to free-run
// (CONT + DMAEN — RM-derived; Renode tags CONT unimplemented, so emulation
// paces conversions through the leg's trigger shim and only silicon can
// witness the free-run itself), every result is moved by the channel the
// board assigned to `adc.conv`, and the consumer takes hardware-stable
// halves off the ring.
//
// This wrapper exists for the §4 TEARDOWN ORDER, which the bare ring cannot
// provide alone: the peripheral's request stream must stop BEFORE the channel
// does (a request landing mid-teardown on a disabled channel is how overrun
// flags get stuck). C++ member-destruction order encodes it — this class's
// destructor BODY (ADC side: ADSTP, DMAEN/CONT off — dma_burst_end) runs
// first, then the `ring_` member's destructor stops the channel and releases
// the claim. NOT movable, NOT copyable (the ring's ISRs hold a pointer into
// it); guaranteed copy elision covers the factory-return spelling
// `auto ring = adc.ring(samples);` — constructed in place, never moved.
template <class Inst, class Route>
class stream {
public:
    stream(const stream&) = delete;
    stream& operator=(const stream&) = delete;
    stream(stream&&) = delete;
    stream& operator=(stream&&) = delete;

    ~stream() { hal::adc_impl<Inst>::dma_burst_end(); }

    // The control-loop discipline, forwarded from the ring (see alloy/dma.hpp
    // for the full contract): block until a half is hardware-stable, hand it
    // over, count every half the consumer failed to collect.
    [[nodiscard]] std::span<const std::uint16_t> take() { return ring_.take(); }
    [[nodiscard]] std::uint32_t missed() const { return ring_.missed(); }
    [[nodiscard]] bool pending() const { return ring_.pending(); }

    // ISR-context boundary hook — what alloy::async::ring_waiter registers.
    void on_boundary(void (*fn)(void*), void* ctx = nullptr) {
        ring_.on_boundary(fn, ctx);
    }

private:
    template <class I, class R, opts<I> O>
    friend class handle;

    // The ADC must be CONFIGURED for the stream before the channel arms
    // (RM0444: DMAEN/CONT only writable while ADSTART=0, and the first request
    // must not fire into an unarmed channel). This helper sequences that into
    // the member-init list: begin() selects the channel and raises DMAEN/CONT,
    // the ring constructor then claims + programs + starts the DMA channel,
    // and the constructor BODY kicks ADSTART last — §4's arm order, encoded.
    static Route begun(std::uint8_t channel) {
        hal::adc_impl<Inst>::dma_burst_begin(channel);
        return Route{};
    }

    template <std::size_t N>
    explicit stream(std::uint8_t channel, dma::ring_storage<std::uint16_t, N>& storage)
        : ring_(begun(channel), hal::adc_impl<Inst>::dr_addr(), storage) {
        hal::adc_impl<Inst>::dma_burst_kick();
    }

    dma::ring<std::uint16_t, Route> ring_;
};

namespace detail {
// 0 when this converter has no analog watchdog alloy can reach — either the
// driver has no hooks for one, or the chip database records no count. Both are
// "not reachable from here", and a board's generic code needs one answer.
template <class Inst>
consteval unsigned watchdog_count() {
    if constexpr (requires {
                      Inst::feat::analog_watchdogs;
                      hal::adc_impl<Inst>::template awd_arm<0>(watchdog_config{});
                  }) {
        return Inst::feat::analog_watchdogs;
    } else {
        return 0u;
    }
}
}  // namespace detail

template <class Inst>
inline constexpr unsigned watchdog_count = detail::watchdog_count<Inst>();

// Does this converter have a SEQUENCER, and therefore handle::scan()?
//
// A variable template rather than a bare requires-expression at the call site,
// and that is not style — it is the only spelling that works. A
// requires-expression over a CONCRETE type is checked eagerly, so
// `if constexpr (requires { adc.scan(c, v); })` on a converter without the
// member is a hard compile error instead of `false`. Measured on the very pair
// this constant exists to tell apart: the naive form compiles on an F7 and
// fails on a G0 with "no matching function for call to ... scan". Routing the
// probe through a template parameter makes the check per-instantiation, which
// is what portable code needs:
//
//   [&]<class Bind>(Bind*) {
//       auto adc = Bind::open();
//       if constexpr (alloy::adc::can_scan<typename Bind::instance>) {
//           adc.scan(channels, out);
//       } else {
//           for (…) out[i] = adc.read(channels[i]);   // one at a time
//       }
//   }(static_cast<board::adc*>(nullptr));
//
// AND THE LAMBDA IS NOT CEREMONY EITHER — this was measured too. Outside a
// template, a DISCARDED if-constexpr branch is still name-looked-up and
// type-checked, so writing the branch straight into main() fails on the very
// board that lacks the member, `can_scan` or not. The generic lambda is what
// makes the branch dependent; examples/analog_probe uses the same shape for
// the same reason. This is the identical trap that forces a board with no
// internal-channel map to emit `adc_vref_channel` rather than omit it.
//
// `watchdog_count<Inst>` above is the same shape for the same reason.
template <class Inst>
inline constexpr bool can_scan = requires(const std::uint8_t* c, std::uint16_t* o) {
    hal::adc_impl<Inst>::scan(c, o, std::uint8_t{});
};

// What a converter's results are worth, in significant bits, after Layer 2 has
// been applied. 12 for a driver that curates no knobs — every ADC alloy ships
// converts at 12 bits by default, and a driver that can change it says so by
// providing result_bits<O>().
//
// This is not decoration: it is what decides whether an analog watchdog is
// still expressible, below.
template <class Inst, opts<Inst> O>
consteval unsigned result_bits() {
    if constexpr (requires { hal::adc_impl<Inst>::template result_bits<O>(); }) {
        return hal::adc_impl<Inst>::template result_bits<O>();
    } else {
        return 12u;
    }
}

// ConvRoute is the board's `adc.conv` DMA assignment, attached to the binder
// by the generator (docs/design/dma-streams.md §1) — `void` when the board
// assigned none, which constrains ring() away so a portable program's
// `requires` probe reports the missing route at compile time.
//
// Opts travels with the handle because one Layer-2 value changes what a LATER
// call means: oversampling moves the scale that `read()` returns and that the
// analog watchdog compares against. A handle that forgot how its port was
// configured could not check the pair, which is the whole reason watchdog()
// can refuse below.
template <class Inst, class ConvRoute = void, opts<Inst> Opts = {}>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    // Blocking single conversion of the given channel (raw counts).
    [[nodiscard]] std::uint16_t read(std::uint8_t channel) const {
        detail::admit_channel(channel);
        return hal::adc_impl<Inst>::read(channel);
    }

    // MULTI-CHANNEL SCAN: convert a list of channels, in the order given,
    // into the caller's buffer. Blocking, one conversion per element.
    //
    //   const std::uint8_t chans[] = {3, 4, 3};
    //   std::uint16_t v[3];
    //   if (adc.scan(chans, v)) { … }
    //
    // ONLY EXISTS WHERE THE SILICON HAS A SEQUENCER, and that is the point of
    // the requires-clause rather than a runtime "unsupported" return. A
    // converter driven by a channel BITMAP cannot honour this signature: a
    // bitmap has no order and no repeats, so `{3, 4, 3}` is not a thing it can
    // be asked for. Offering the method everywhere and failing at run time on
    // half the fleet would be exactly the lie this surface removes — on those
    // parts `adc.scan(...)` is a compile error naming the member, and portable
    // code probes for it:
    //
    //   if constexpr (alloy::adc::can_scan<Inst>) { … } else { … }
    //
    // Returns false if the request cannot be programmed — an empty list, or
    // more channels than the sequencer has slots. That IS a runtime answer
    // because the length is a runtime value; a caller with a constant-sized
    // array gets the bound checked by the span conversion instead.
    [[nodiscard]] bool scan(std::span<const std::uint8_t> channels,
                            std::span<std::uint16_t> out) const
        requires requires(const std::uint8_t* c, std::uint16_t* o) {
            hal::adc_impl<Inst>::scan(c, o, std::uint8_t{});
        }
    {
        if (channels.empty() || out.size() < channels.size() ||
            channels.size() > 0xFFu) {
            return false;
        }
        for (const std::uint8_t ch : channels) {
            detail::admit_channel(ch);
        }
        return hal::adc_impl<Inst>::scan(channels.data(), out.data(),
                                         static_cast<std::uint8_t>(channels.size()));
    }

    // DMA burst: N continuous conversions of one channel straight to RAM.
    // Blocking (returns when the buffer is full); only exists where the
    // driver has DMA hooks AND the chip data carries the request ID.
    template <class Chan>
    [[nodiscard]] bool read_burst(const Chan& dma, std::uint8_t channel,
                                  std::span<std::uint16_t> out) const
        requires requires {
            hal::adc_impl<Inst>::dma_burst_begin(channel);
            Inst::dmareq_conv;
        }
    {
        detail::admit_channel(channel);
        hal::adc_impl<Inst>::dma_burst_begin(channel);
        dma.start_p2m_u16(hal::adc_impl<Inst>::dr_addr(), out, Inst::dmareq_conv);
        hal::adc_impl<Inst>::dma_burst_kick();
        const bool ok = dma.wait();
        dma.stop();
        hal::adc_impl<Inst>::dma_burst_end();
        return ok;
    }

    // DMA ring: continuous conversions of one channel streamed into the
    // caller's ring buffer, consumed as hardware-stable halves — the design's
    // anchor 2.1, on the BOARD-ASSIGNED route (board.json "dma": "adc.conv").
    // Claims the assigned channel, programs a circular p2m transfer of 16-bit
    // items, arms the half + full interrupts, starts the converter's
    // continuous conversion. Compile error (this method is constrained away)
    // if the board assigned no route, the driver has no DMA hooks, or the
    // routed controller cannot do circular + half events (today: SAME70
    // XDMAC) — never a link error or a runtime surprise.
    //
    // `channel` is the ANALOG input to convert. It defaults to 0 so the
    // anchor's spelling `adc.ring(samples)` is real; any other input is named
    // explicitly, the same units read() takes.
    template <std::size_t N>
    [[nodiscard]] adc::stream<Inst, ConvRoute> ring(
        dma::ring_storage<std::uint16_t, N>& storage, std::uint8_t channel = 0u) const
        requires(!std::is_void_v<ConvRoute>) && stream_capable<Inst, ConvRoute>
    {
        detail::admit_channel(channel);
        return adc::stream<Inst, ConvRoute>(channel, storage);
    }

    // The explicit-route overload — the documented escape hatch for
    // hand-wired projects (design §1): same stream, on a route the caller
    // spelled out instead of the board's assignment.
    template <class Route, std::size_t N>
    [[nodiscard]] adc::stream<Inst, Route> ring(
        Route, dma::ring_storage<std::uint16_t, N>& storage,
        std::uint8_t channel = 0u) const
        requires stream_capable<Inst, Route>
    {
        detail::admit_channel(channel);
        return adc::stream<Inst, Route>(channel, storage);
    }

    // Arm one of this converter's analog watchdogs. Only offered where the
    // driver has the hooks — on an ADC without them the call is not a member,
    // not a member that quietly does nothing.
    template <unsigned N>
    [[nodiscard]] adc::watchdog<Inst, N> watchdog(watchdog_config cfg) const
        requires requires { hal::adc_impl<Inst>::template awd_arm<N>(cfg); }
    {
        // Both numbers land in the diagnostic: GCC prints the comparison, and
        // the count came from the chip database, so the bound cannot drift
        // from the silicon the way a hand-written constant can.
        static_assert(N < Inst::feat::analog_watchdogs,
                      "this ADC does not have that many analog watchdogs");
        // THE ONE PLACE TWO LAYER-2 KNOBS COLLIDE, and it is refused rather
        // than papered over. The threshold registers are 12 bits wide — that
        // is generated data, not a guess — so a converter configured to
        // produce WIDER results than that has no way to express a window over
        // its own full range: every threshold a caller could write is a value
        // the converter passes through on its way to somewhere higher.
        //
        // What this deliberately does NOT do is scale the window by the
        // oversampler's shift and carry on. Whether the silicon compares the
        // pre-shift sum or the post-shift result against AWDnTR is a
        // reference-manual fact, and inventing an answer to it would put a
        // plausible wrong number in a guard whose entire job is to be right.
        // Refusing is the honest half of that pair, and it is a compile error
        // naming the combination rather than a runtime surprise.
        static_assert(adc::result_bits<Inst, Opts>() <= 12u,
                      "this ADC is configured to produce results wider than 12 bits "
                      "(oversampling with a shift smaller than log2 of the ratio), and "
                      "the analog watchdog's thresholds are a 12-bit field — raise "
                      "oversample_shift to keep the 12-bit scale, or drop the watchdog");
        // Per INSTANCE and ORDINAL: two claimants of ONE watchdog would fight
        // over one window, and the second would win silently.
        alloy::claim::sub_exclusive<Inst, N, alloy::claim::personality::adc>();
        detail::admit_channel(cfg.channel);
        detail::admit_window(cfg.low, cfg.high);
        hal::adc_impl<Inst>::template awd_arm<N>(cfg);
        return adc::watchdog<Inst, N>{};
    }

private:
    template <class, class, class>
    friend struct bind;
    handle() = default;
};

// ConvRoute: the board's `adc.conv` DMA assignment (a generated
// alloy::dma::route), or void when the board assigns none — see handle.
template <class Inst, class Clock, class ConvRoute = void>
struct bind {
    using instance = Inst;

    // How many analog watchdogs portable code may ask this port for. 0 means
    // "not reachable here" and is what board-generic code branches on, so a
    // program that wants one does not have to name a chip to find out.
    static constexpr unsigned watchdogs = adc::watchdog_count<Inst>;

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    // Layer 2 travels as a template argument, defaulted, so the empty case is
    // the call every existing site already writes. The knobs are programmed
    // INSIDE enable(), between the regulator coming up and ADEN — the only
    // window in which this IP's configuration registers are writable — which
    // is why there is no reconfigure() here and no stop/re-enable cycle.
    template <opts<Inst> Opts = {}>
    static handle<Inst, ConvRoute, Opts> open(config = {}) {
        // Per INSTANCE, cross-TU (alloy/core/claim.hpp), not per binder type.
        alloy::claim::exclusive<Inst, alloy::claim::personality::adc>();
        if constexpr (requires { hal::adc_impl<Inst>::template enable<Opts>(0u); }) {
            hal::adc_impl<Inst>::template enable<Opts>(kernel_hz());
        } else {
            hal::adc_impl<Inst>::enable(kernel_hz());
        }
        return handle<Inst, ConvRoute, Opts>{};
    }

    // What one conversion is worth on this port, in significant bits, given
    // the knobs. Portable code that scales a reading branches on this instead
    // of assuming 12 — and it is a constant, so the branch costs nothing.
    template <opts<Inst> Opts = {}>
    static constexpr unsigned result_bits = adc::result_bits<Inst, Opts>();
};

}  // namespace alloy::adc
