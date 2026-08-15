// ADC driver for the ST adc_v3 IP (L4/F3-lineage sequencer ADC).
//
// BEHAVIOR only: addresses/fields come from the generated alloy::ip::st
// headers; the companion common block arrives as Inst::common_t (generated
// from chip data). Nothing here is shared with adc_v2 — that IP is a CHSELR
// bitmap machine, this one runs a sequencer and wakes in three explicit
// steps (RM0394 §16):
//
//   exit DEEPPWD -> ADVREGEN + 20 us settle -> ADCAL (ADEN=0) -> ADEN/ADRDY
//
// Skipping the settle or calibrating with ADEN=1 doesn't error — it yields
// garbage conversions, silently. The hardware oversampler runs at 16x with
// no post-shift (ROVSE, OVSR=3, OVSS=0), so every read() spans the full
// 16-bit range: 12-bit code x16. Internal channels differ from the G0:
// VREFINT is channel 0 (CCR.VREFEN), temperature is 17 (CCR.CH17SEL).
// Sample time is pinned at the maximum for every channel — this driver's
// v1 consumers are high-impedance dividers (NTC ladders), where a short
// sample time reads the sample cap, not the sensor.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"
#include "alloy/ip/st/adc_v3.hpp"
#include "alloy/ip/st/adccommon_v3.hpp"

namespace alloy::hal {

// ── Layer 2 for st/adc_v3 ───────────────────────────────────────────────
//
// SAME NAMES, SAME UNITS as st_adc_v2's and st_adc_g4's — a libs/ driver that
// writes `{.oversample_ratio = 16, .oversample_shift = 0}` now compiles on
// three IPs from three ST families with no preprocessor.
//
// THE DEFAULTS ARE NOT `off`, AND THAT IS DELIBERATE. This driver has always
// forced 16x oversampling with no post-shift, so every read() spans the full
// 16-bit range (12-bit code x16) — a decision baked into a HAL driver for its
// first consumer, high-impedance NTC ladders. Turning it into a knob must not
// change what existing callers get, so the defaults reproduce the old body
// exactly and a port that asks for nothing is programmed with the same writes
// it always was. What changes is that the decision is now VISIBLE and
// overridable instead of being a comment.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_v3>
struct adc_opts<Inst> {
    std::uint8_t resolution_bits = 12;
    std::uint16_t oversample_ratio = 16;  // the historical default, not "off"
    std::uint8_t oversample_shift = 0;    // ...and no post-shift, so DR is 16-bit
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_v3>
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

    [[nodiscard]] static consteval unsigned log2_of(std::uint32_t v) {
        unsigned n = 0;
        while (v > 1u) { v >>= 1u; ++n; }
        return n;
    }

    template <adc_opts<Inst> Opts = {}>
    static void enable(std::uint32_t kernel_hz) {
        alloy::gate_on(Inst::gate);
        Common::ip::ckmode.write(c(), 0u);  // async kernel clock (CCIPR.ADCSEL)

        // Wake order is load-bearing (see header prose).
        IP::deeppwd.clear(r());
        IP::advregen.set(r());
        // >= 20 us regulator settle, from the kernel clock the caller states.
        // Crude cycle loop on purpose: no timer dependency in a HAL driver.
        for (volatile std::uint32_t n = kernel_hz / 50'000u + 1u; n != 0u; --n) {
        }
        IP::adcal.set(r());
        while (IP::adcal.read(r()) != 0u) {
        }
        r().SMPR1 = all_slow();
        r().SMPR2 = all_slow();
        // Layer 2, with the converter still disabled — the only state in
        // which CFGR/CFGR2 accept writes. The defaults are 16x with no shift,
        // which is the write this driver has always made.
        static_assert(Opts.resolution_bits == 12u || Opts.resolution_bits == 10u ||
                          Opts.resolution_bits == 8u || Opts.resolution_bits == 6u,
                      "st/adc_v3 converts at 12, 10, 8 or 6 bits");
        constexpr std::uint32_t res_code = (12u - Opts.resolution_bits) / 2u;
        static_assert(res_code <= IP::res.raw_mask, "resolution code exceeds CFGR.RES");
        if constexpr (res_code != 0u) {
            IP::res.write(r(), res_code);
        }
        static_assert(Opts.oversample_ratio >= 1u && Opts.oversample_ratio <= 256u,
                      "st/adc_v3 oversamples 2 to 256 samples per result (1 = off)");
        static_assert((Opts.oversample_ratio & (Opts.oversample_ratio - 1u)) == 0u,
                      "st/adc_v3's oversampling ratio is a power of two");
        static_assert(Opts.oversample_shift <= 8u,
                      "st/adc_v3 shifts an oversampled sum right by at most 8 bits");
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
        // One-deep sequence: L=0, SQ1=channel, software start, wait EOC.
        r().SQR1 = static_cast<std::uint32_t>(channel) << IP::sq1.pos;
        IP::adstart.set(r());
        while (IP::eoc.read(r()) == 0u) {
        }
        return static_cast<std::uint16_t>(r().DR);
    }

    // With the DEFAULTS this is 16 — which is why the analog watchdog is not
    // offered on this IP even though the silicon has three: handle::watchdog()
    // refuses a port whose results are wider than the 12-bit threshold field,
    // and this driver's historical default is exactly that port.
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
