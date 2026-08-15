// ADC driver for the G4's sequencer ADC (st/adc_g4 — 93 parts).
//
// The G4's converter is the L4's with additions: of 19 registers the two share
// 18, and not one of them sits at a different offset (the diff is measured,
// register by register, in alloy-devices registers/st/adc_g4.yaml). So this
// body is st_adc_v3.hpp's, and the two files are near-duplicates on purpose.
//
// WHY DUPLICATED RATHER THAN SHARED, because a reader will ask and the wrong
// answer is "nobody got around to it": codegen emits an IP header only for the
// IPs a board's chip actually instantiates, and includes the driver named
// after the IP. A single body reached from both would have to include both
// `alloy/ip/st/adc_v3.hpp` and `alloy/ip/st/adc_g4.hpp`, and on any given board
// exactly one of those files exists. The tree's `*_body.hpp` split solves that
// where the shared part is large (st_i2c_v2, st_usart_v4); here the whole body
// is forty lines, so a second file costs less than the indirection.
//
// WHERE THE TWO IPs GENUINELY DIVERGE, kept visible rather than smoothed: the
// G4's CFGR.EXTSEL is FIVE bits where the L4's is four, and ALIGN moved from
// bit 5 to bit 15 to make room. This body programs neither field, which is
// what makes the duplication safe — each IP's geometry stays in its own
// generated header, so a literal can never be shared by accident.
//
// The G4-only registers (gain compensation, the three split analog-watchdog
// thresholds TR1/TR2/TR3) are not programmed here. TR1 is curated; the other
// two are not, and neither is any watchdog hook — the G0's watchdog work would
// port to this IP and has not been done.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"
#include "alloy/ip/st/adc_g4.hpp"
#include "alloy/ip/st/adccommon_g4.hpp"

namespace alloy::hal {

// ── Layer 2 for st/adc_g4 ───────────────────────────────────────────────
//
// THE FIELD NAMES ARE THE G0'S ON PURPOSE. `resolution_bits`,
// `oversample_ratio` and `oversample_shift` mean exactly what they mean in
// st_adc_v2's opts, in the same units, so a `libs/` driver written against
// them compiles on both without a preprocessor — that is the Layer-2 naming
// rule's whole payoff, and this is the first place two DIFFERENT IPs from two
// different families make it real.
//
// What this IP does NOT get is the G0's `sample_time` pair and its
// `sample_time_alt_channels` mask: the G0 has two shared tracking times and a
// per-channel selector, the G4 has a genuine time per channel. Same feature,
// different arity, so the same name would be a lie — and enable() pins every
// channel at the longest time for the reason the header gives.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_g4>
struct adc_opts<Inst> {
    std::uint8_t resolution_bits = 12;
    std::uint16_t oversample_ratio = 1;   // 1 = off; otherwise a power of two, 2..256
    std::uint8_t oversample_shift = 0;    // right shift applied to the sum
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_g4>
struct adc_impl<Inst> {
    using IP = typename Inst::ip;
    using Common = typename Inst::common_t;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    static typename Common::ip::regs& c() {
        return *reinterpret_cast<typename Common::ip::regs*>(Common::base);
    }

    // Every channel at the longest sample time: ten 3-bit fields per SMPR
    // word, all 0b111. Built by fold, not literal — the all-ones word is an
    // 8-hex-digit constant the contract gate would (rightly) question.
    static constexpr std::uint32_t all_slow() {
        std::uint32_t v = 0;
        for (unsigned i = 0; i < 10u; ++i) {
            v |= 7u << (3u * i);
        }
        return v;
    }

    // consteval, so it is only ever a compile-time fact about a Layer-2 value.
    [[nodiscard]] static consteval unsigned log2_of(std::uint32_t v) {
        unsigned n = 0;
        while (v > 1u) { v >>= 1u; ++n; }
        return n;
    }

    template <adc_opts<Inst> Opts = {}>
    static void enable(std::uint32_t kernel_hz) {
        alloy::gate_on(Inst::gate);
        // Asynchronous kernel clock — the RCC's dedicated ADC mux
        // (CCIPR.ADC12SEL / ADC345SEL). Written explicitly rather than
        // trusted: the reset value happens to be this, and a driver that
        // relies on a reset value it did not choose breaks the day something
        // else touches the register first.
        Common::ip::ckmode.write(c(), 0u);

        // THE WAKE ORDER IS LOAD-BEARING and it is the reason this IP needs a
        // driver at all: the converter's reset state is DEEP POWER DOWN.
        //   leave DEEPPWD -> ADVREGEN + settle -> ADCAL with ADEN low -> ADEN
        // Skipping the settle, or calibrating with ADEN already set, does not
        // error. It yields garbage conversions, silently — which is the worst
        // failure shape an ADC has.
        IP::deeppwd.clear(r());
        IP::advregen.set(r());
        // >= 20 us regulator settle, from the kernel rate the caller states.
        // Crude cycle loop on purpose: no timer dependency in a HAL driver.
        for (volatile std::uint32_t n = kernel_hz / 50'000u + 1u; n != 0u; --n) {
        }
        IP::adcal.set(r());
        while (IP::adcal.read(r()) != 0u) {
        }
        r().SMPR1 = all_slow();
        r().SMPR2 = all_slow();

        // --- Layer 2, programmed with the converter still disabled, which is
        // the only state in which CFGR/CFGR2 are writable at all.
        static_assert(Opts.resolution_bits == 12u || Opts.resolution_bits == 10u ||
                          Opts.resolution_bits == 8u || Opts.resolution_bits == 6u,
                      "st/adc_g4 converts at 12, 10, 8 or 6 bits");
        constexpr std::uint32_t res_code = (12u - Opts.resolution_bits) / 2u;
        static_assert(res_code <= IP::res.raw_mask, "resolution code exceeds CFGR.RES");
        if constexpr (res_code != 0u) {
            IP::res.write(r(), res_code);
        }

        static_assert(Opts.oversample_ratio >= 1u && Opts.oversample_ratio <= 256u,
                      "st/adc_g4 oversamples 2 to 256 samples per result (1 = off)");
        static_assert((Opts.oversample_ratio & (Opts.oversample_ratio - 1u)) == 0u,
                      "st/adc_g4's oversampling ratio is a power of two");
        static_assert(Opts.oversample_shift <= 8u,
                      "st/adc_g4 shifts an oversampled sum right by at most 8 bits");
        if constexpr (Opts.oversample_ratio > 1u) {
            IP::ovsr.write(r(), log2_of(Opts.oversample_ratio) - 1u);
            IP::ovss.write(r(), Opts.oversample_shift);
            IP::rovse.set(r());
        }

        r().ISR = IP::adrdy.mask;  // w1c any stale ready flag
        IP::aden.set(r());
        while (IP::adrdy.read(r()) == 0u) {
        }
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t channel) {
        // One-deep sequence: L = 0, SQ1 = channel, software start, wait EOC.
        r().SQR1 = static_cast<std::uint32_t>(channel) << IP::sq1.pos;
        IP::adstart.set(r());
        while (IP::eoc.read(r()) == 0u) {
        }
        return static_cast<std::uint16_t>(r().DR);
    }

    // What a result is worth after Layer 2: the resolution, widened by the
    // bits oversampling gains and narrowed by the shift, clamped at the data
    // register's own width. Identical arithmetic to st_adc_v2's, because the
    // oversampler works the same way — sum then shift.
    template <adc_opts<Inst> Opts>
    [[nodiscard]] static consteval unsigned result_bits() {
        unsigned bits = Opts.resolution_bits;
        if (Opts.oversample_ratio > 1u) {
            const unsigned gained = log2_of(Opts.oversample_ratio);
            bits = bits + gained -
                   (Opts.oversample_shift < gained ? Opts.oversample_shift : gained);
        }
        return bits > 16u ? 16u : bits;
    }
};

}  // namespace alloy::hal
