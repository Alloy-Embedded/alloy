// ADC driver for ST's classic multi-ADC (st/adc_f4 — the F4 and F7 lineage,
// 236 parts). Blocking single conversions, right-aligned raw counts.
//
// WHAT MAKES THIS DIFFERENT FROM st_adc_v2 (the G0), because the two look
// alike from the facade and are not alike underneath:
//
//  1. THERE IS NO CHANNEL BITMAP. Conversions are a SEQUENCE, always: SQR1.L
//     says how many, and SQR3.SQ1…SQ6 / SQR2 / SQR1 name the channels in
//     order. A single conversion is a sequence of length one, so `read()`
//     writes L = 0 and puts the channel in SQR3's first slot. There is no
//     CCRDY handshake to wait on either — the sequence registers take effect
//     when the next conversion starts.
//
//  2. SAMPLING TIME IS GENUINELY PER CHANNEL. SMPR2 holds channels 0..9 and
//     SMPR1 holds 10..18, three bits each. That is a different shape from the
//     G0's two shared times plus a selector, and it is why this driver's
//     Layer 2 has a plain `sample_time` while the G0's has a pair and a mask.
//     Same feature, same field name, honestly different arity — which is the
//     naming rule working, not breaking.
//
//  3. THE CLOCK PRESCALER LIVES IN A COMPANION. ADC1/2/3 share one common
//     block, at a different distance from each of them, so it is a separate
//     peripheral reached through `Inst::common_t` (chip data:
//     `companions: {common: adc123_common}`). Bring-up MUST write it — see
//     the note on the divider in enable().
//
//  4. FLAGS CLEAR BY WRITING ZERO. SR on this IP is not the G0's
//     write-one-to-clear register. Clearing EOC means writing a word with
//     that bit low, and every other flag preserved.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"
#include "alloy/ip/st/adc_f4.hpp"

namespace alloy::hal {

// The eight tracking times this IP offers, named as the manual names them.
enum class st_adc_f4_sample_time : std::uint8_t {
    cycles3 = 0,
    cycles15 = 1,
    cycles28 = 2,
    cycles56 = 3,
    cycles84 = 4,
    cycles112 = 5,
    cycles144 = 6,
    cycles480 = 7,
};

// The common block's prescaler. See enable() for why the DEFAULT is the
// slowest one and why that is a decision rather than laziness.
enum class st_adc_f4_clock_divider : std::uint8_t {
    div2 = 0,
    div4 = 1,
    div6 = 2,
    div8 = 3,
};

// ── Layer 2 for st/adc_f4 ───────────────────────────────────────────────
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_f4>
struct adc_opts<Inst> {
    // 12, 10, 8 or 6.
    std::uint8_t resolution_bits = 12;

    // Applied to whichever channel read() converts. Per channel in the
    // silicon; this driver programs the one it is about to convert, which is
    // the only channel a blocking read has.
    st_adc_f4_sample_time sample_time = st_adc_f4_sample_time::cycles480;

    // Divider from PCLK2 to the converter. Default is the slowest — read
    // enable() before raising it.
    st_adc_f4_clock_divider clock_divider = st_adc_f4_clock_divider::div8;

    // NO `oversample_ratio` MEMBER, and its absence is the capability answer:
    // this lineage's ADC has no hardware oversampler at all (there is no
    // CFGR2 on it). A program that probes `requires { Opts{}.oversample_ratio; }`
    // gets false here and true on the G0, with no preprocessor — which is what
    // the Layer-2 naming rule is for.
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_f4>
struct adc_impl<Inst> {
    using IP = typename Inst::ip;
    // The shared common block's own register map, reached the way every
    // companion is (see st_fdcan_v1.hpp's `ram_t`).
    using CIP = typename Inst::common_t::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    static typename CIP::regs& c() {
        return *reinterpret_cast<typename CIP::regs*>(Inst::common_t::base);
    }

    static void spin(std::uint32_t iterations) {
        for (volatile std::uint32_t i = 0; i < iterations; i = i + 1u) {
        }
    }

    template <adc_opts<Inst> Opts = {}>
    static void enable(std::uint32_t kernel_hz) {
        alloy::gate_on(Inst::gate);

        // THE PRESCALER, and why the default is the slowest divider.
        //
        // ADCPRE's reset value divides PCLK2 by two. On a fast part in this
        // lineage that leaves the converter clocked above its rated maximum,
        // so leaving the reset value alone is not a neutral choice — it is a
        // converter running out of spec, which reads as plausible-but-wrong
        // counts rather than as a failure.
        //
        // Choosing the RIGHT divider needs the part's maximum ADC clock, and
        // that is a datasheet electrical characteristic this driver has no
        // served source for. So it does the one thing that is correct without
        // that number: the SLOWEST divider, which is legal on every part in
        // the lineage at every PCLK2 it can run at. The cost is conversion
        // time, and a product that has read its own datasheet raises it
        // through Layer 2 (`clock_divider`) with the fact in hand.
        //
        // Inventing a limit here would be the exact failure the register data
        // refuses: a plausible number nobody checked, in the one place that
        // decides whether readings mean anything.
        CIP::adcpre.write(c(), static_cast<std::uint32_t>(Opts.clock_divider));

        static_assert(Opts.resolution_bits == 12u || Opts.resolution_bits == 10u ||
                          Opts.resolution_bits == 8u || Opts.resolution_bits == 6u,
                      "st/adc_f4 converts at 12, 10, 8 or 6 bits");
        constexpr std::uint32_t res_code = (12u - Opts.resolution_bits) / 2u;
        static_assert(res_code <= IP::res.raw_mask, "resolution code exceeds CR1.RES");
        IP::res.write(r(), res_code);

        // One conversion per start, of one channel: sequence length 1 (L = 0),
        // EOC raised per conversion, no scan, no continuous mode.
        IP::l.write(r(), 0u);
        IP::scan.clear(r());
        IP::cont.clear(r());
        IP::eocs.set(r());

        // ADON, then the analog block's stabilisation time. The manual gives
        // it as a microsecond-class delay (tSTAB); this spins for ~20 us at
        // the kernel rate, the same conservative shape st_adc_v2 uses for its
        // regulator, and errs long because the cost is one-time.
        IP::adon.set(r());
        spin(kernel_hz / 50'000u);
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t channel) {
        return read_with<adc_opts<Inst>{}>(channel);
    }

    // The sampling time belongs to the CHANNEL, so it is programmed here
    // rather than once at open: a caller that reads two channels with one
    // port gets each one's own tracking time, which is what the silicon
    // offers and what a shared write would throw away.
    template <adc_opts<Inst> Opts>
    [[nodiscard]] static std::uint16_t read_with(std::uint8_t channel) {
        using sr = typename IP::sr;
        const auto smp = static_cast<std::uint32_t>(Opts.sample_time);
        if (channel < 10u) {
            // SMPR2 holds channels 0..9, three bits each, low channel first.
            const std::uint32_t shift = static_cast<std::uint32_t>(channel) * 3u;
            r().SMPR2 = (r().SMPR2 & ~(std::uint32_t{7} << shift)) | (smp << shift);
        } else {
            const std::uint32_t shift = (static_cast<std::uint32_t>(channel) - 10u) * 3u;
            r().SMPR1 = (r().SMPR1 & ~(std::uint32_t{7} << shift)) | (smp << shift);
        }

        // Sequence of one: this channel in slot 1. L is already 0 from
        // enable(), and nothing else writes it. The slot accessor is indexed
        // because the field REPEATS six times across SQR3 — slot 1 is index 0.
        IP::template sq1_6<0>.write(r(), channel);

        // SR clears by writing ZERO on this IP — not write-one-to-clear. So a
        // stale EOC is cleared by writing the register with that bit low,
        // which is what this assignment does; the other flags are read-only
        // status a write cannot set.
        r().SR = 0u;

        IP::swstart.set(r());
        while ((r().SR & sr::eoc) == 0u) {
        }
        // Reading DR clears EOC in the silicon as well; the explicit clear
        // above is what makes the WAIT above honest on the next call.
        return static_cast<std::uint16_t>(r().DR);
    }

    // ── MULTI-CHANNEL SCAN ──────────────────────────────────────────────
    //
    // The feature this IP has and the G0 does not. A scan is what the SQR
    // registers were always for: SQR1.L says how many conversions, and the
    // slots name the channels IN ORDER — repeats allowed, which a channel
    // BITMAP cannot express at all. That is why this hook lives here and not
    // on adc_v2: the G0's ordered-sequence view sits at the same address as
    // its bitmap and cannot be curated (see alloy-devices adc_v2.yaml), so a
    // scan there would silently become "every selected channel once, in
    // hardware order" — a different operation wearing the same name.
    //
    // Blocking, one conversion at a time. EOCS was set to EACH_CONVERSION at
    // enable(), so EOC rises per slot rather than once per sequence, and DR
    // is read between slots — which is what makes this work without DMA. A
    // caller that wants the sequence moved for it wants ring(), which this IP
    // does not offer yet.
    //
    // The sequence is programmed and then TORN DOWN: SCAN and L go back to
    // the single-conversion state on the way out, so an interleaved read()
    // cannot inherit a sequence it never asked for. Costlier than leaving it
    // armed, and the alternative is a read() whose meaning depends on what
    // ran before it.
    static constexpr std::uint8_t max_scan_slots = 16u;

    [[nodiscard]] static bool scan(const std::uint8_t* channels, std::uint16_t* out,
                                   std::uint8_t count) {
        using sr = typename IP::sr;
        if (count == 0u || count > max_scan_slots) {
            return false;
        }

        // Slot n of the sequence, across three registers whose order is not
        // the obvious one: SQR3 carries slots 1..6, SQR2 carries 7..12 and
        // SQR1 carries 13..16 alongside the length. Indexed accessors, so the
        // generated stride does the arithmetic rather than a literal here.
        for (std::uint8_t i = 0; i < count; ++i) {
            const std::uint8_t ch = channels[i];
            if (i < 6u) {
                write_slot<0>(i, ch);
            } else if (i < 12u) {
                write_slot<1>(static_cast<std::uint8_t>(i - 6u), ch);
            } else {
                write_slot<2>(static_cast<std::uint8_t>(i - 12u), ch);
            }
        }
        IP::l.write(r(), static_cast<std::uint32_t>(count - 1u));
        IP::scan.set(r());

        r().SR = 0u;  // clears by writing zero on this IP, not w1c
        IP::swstart.set(r());
        for (std::uint8_t i = 0; i < count; ++i) {
            while ((r().SR & sr::eoc) == 0u) {
            }
            out[i] = static_cast<std::uint16_t>(r().DR);  // reading DR clears EOC
        }

        IP::scan.clear(r());
        IP::l.write(r(), 0u);
        return true;
    }

  private:
    // One slot write, with the register chosen at COMPILE time and the slot
    // index at run time — the generated accessors are indexed templates, so
    // the index has to be constant; this unrolls the six (or four) cases of
    // one register rather than the sixteen of all three.
    template <unsigned Reg>
    static void write_slot(std::uint8_t idx, std::uint8_t ch) {
        const auto v = static_cast<std::uint32_t>(ch);
        if constexpr (Reg == 0u) {
            switch (idx) {
                case 0: IP::template sq1_6<0>.write(r(), v); break;
                case 1: IP::template sq1_6<1>.write(r(), v); break;
                case 2: IP::template sq1_6<2>.write(r(), v); break;
                case 3: IP::template sq1_6<3>.write(r(), v); break;
                case 4: IP::template sq1_6<4>.write(r(), v); break;
                default: IP::template sq1_6<5>.write(r(), v); break;
            }
        } else if constexpr (Reg == 1u) {
            switch (idx) {
                case 0: IP::template sq7_12<0>.write(r(), v); break;
                case 1: IP::template sq7_12<1>.write(r(), v); break;
                case 2: IP::template sq7_12<2>.write(r(), v); break;
                case 3: IP::template sq7_12<3>.write(r(), v); break;
                case 4: IP::template sq7_12<4>.write(r(), v); break;
                default: IP::template sq7_12<5>.write(r(), v); break;
            }
        } else {
            switch (idx) {
                case 0: IP::template sq13_16<0>.write(r(), v); break;
                case 1: IP::template sq13_16<1>.write(r(), v); break;
                case 2: IP::template sq13_16<2>.write(r(), v); break;
                default: IP::template sq13_16<3>.write(r(), v); break;
            }
        }
    }

  public:
    // What a result is worth: this IP has no oversampler, so it is exactly
    // the configured resolution.
    template <adc_opts<Inst> Opts>
    [[nodiscard]] static consteval unsigned result_bits() {
        return Opts.resolution_bits;
    }
};

}  // namespace alloy::hal
