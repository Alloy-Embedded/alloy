// ADC driver for the ST single-ADC-with-CHSELR IP (adc_v2: F0/G0 style).
//
// Blocking single conversions, 12-bit right-aligned, slowest sample time
// (required for the internal vref/temp channels and safe everywhere).
// Bring-up follows RM0444: voltage regulator on, calibrate, enable
// internal sources, ADEN + ADRDY.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/admit.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"
#include "alloy/ip/st/adc_v2.hpp"

namespace alloy::hal {

// The eight tracking times this IP offers, named as the manual names them.
// Half-cycles are real, which is why this is an enum and not a number: the
// values are 1.5 … 160.5 ADC clock cycles and no integer unit holds them
// without inventing one.
enum class st_adc_v2_sample_time : std::uint8_t {
    cycles1_5 = 0,
    cycles3_5 = 1,
    cycles7_5 = 2,
    cycles12_5 = 3,
    cycles19_5 = 4,
    cycles39_5 = 5,
    cycles79_5 = 6,
    cycles160_5 = 7,
};

// The hardware events that can start a conversion on this IP. The encoding is
// generated (CFGR1.EXTSEL), and these names are that data's names — an event
// a given chip does not route is still a legal encoding, so this enum is the
// IP's vocabulary and not a promise about any one part's timers.
enum class st_adc_v2_trigger : std::uint8_t {
    tim1_trgo2 = 0,
    tim1_cc4 = 1,
    tim2_trgo = 2,
    tim3_trgo = 3,
    tim15_trgo = 4,
    tim6_trgo = 5,
    tim4_trgo = 6,
    exti_line11 = 7,
};

enum class st_adc_v2_trigger_edge : std::uint8_t {
    software = 0,  // no hardware trigger; ADSTART starts a conversion
    rising = 1,
    falling = 2,
    both = 3,
};

// ── Layer 2 for st/adc_v2 ───────────────────────────────────────────────
//
// Every member below is a field this driver PROGRAMS, and the values are
// checked against generated data at compile time (see configure()). A knob
// this IP lacks is not a member — CFGR2 has no injected oversampling here,
// so there is no `oversample_injected`, and asking for one names the member.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_v2>
struct adc_opts<Inst> {
    // 12, 10, 8 or 6. Fewer bits is a shorter conversion; the result stays
    // right-aligned at the chosen width.
    std::uint8_t resolution_bits = 12;

    // Hardware oversampling: sum `oversample_ratio` conversions, shift the sum
    // right by `oversample_shift`, put that in the data register. 1 = off.
    //
    // Ratio 2^n with shift n is a plain AVERAGE and keeps the 12-bit scale.
    // A smaller shift keeps the extra bits, which WIDENS the result past 12
    // bits — legal, useful, and the reason the analog watchdog and this knob
    // interact (see alloy/adc.hpp).
    std::uint16_t oversample_ratio = 1;
    std::uint8_t oversample_shift = 0;

    // TWO shared tracking times, not one per channel: SMPR carries SMP1 and
    // SMP2, and SMPSEL picks between them PER CHANNEL. So the honest surface
    // is a pair plus an assignment — a field called "the sampling time of
    // channel n" would be a lie on this silicon.
    st_adc_v2_sample_time sample_time = st_adc_v2_sample_time::cycles160_5;
    st_adc_v2_sample_time sample_time_alt = st_adc_v2_sample_time::cycles1_5;
    std::uint32_t sample_time_alt_channels = 0;

    // What starts a conversion. `software` (the default) leaves the port
    // exactly as it was before this knob existed: ADSTART and nothing else.
    st_adc_v2_trigger trigger = st_adc_v2_trigger::tim1_trgo2;
    st_adc_v2_trigger_edge trigger_edge = st_adc_v2_trigger_edge::software;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::adc_v2>
struct adc_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void spin(std::uint32_t iterations) {
        for (volatile std::uint32_t i = 0; i < iterations; i = i + 1u) {
        }
    }

    // consteval, so it is only ever a compile-time fact about a Layer-2 value.
    [[nodiscard]] static consteval unsigned log2_of(std::uint32_t v) {
        unsigned n = 0;
        while (v > 1u) {
            v >>= 1u;
            ++n;
        }
        return n;
    }

    // Bring-up, with the Layer-2 knobs programmed in the one window where
    // CFGR1/CFGR2/SMPR are writable: after the regulator and the calibration,
    // before ADEN. The default `Opts{}` reproduces the pre-Layer-2 body
    // exactly — 12 bits, no oversampling, SMP1 = 160.5 cycles, software start
    // — so a port that asks for nothing is programmed with the same writes it
    // always was.
    template <adc_opts<Inst> Opts = {}>
    static void enable(std::uint32_t kernel_hz) {
        using cr = typename IP::cr;
        using isr = typename IP::isr;
        alloy::gate_on(Inst::gate);
        IP::ckmode.write(r(), 1u);  // PCLK/2 synchronous
        IP::advregen.set(r());
        spin(kernel_hz / 50'000u);  // >= 20 us regulator start-up
        IP::adcal.set(r());
        while ((r().CR & cr::adcal) != 0u) {
        }
        IP::vrefen.set(r());
        IP::tsen.set(r());
        spin(kernel_hz / 50'000u);  // temp sensor start-up
        // SMP1 lands here as 160.5 cycles (required for vref/temp) unless the
        // caller named a tracking time; configure() writes the same field.
        configure<Opts>();
        r().ISR = IP::adrdy.mask;  // w1c
        IP::aden.set(r());
        while ((r().ISR & isr::adrdy) == 0u) {
        }
    }

    // ── Layer 2: program the vendor knobs ───────────────────────────────
    //
    // Called from bind::open() with the port DISABLED and before ADEN, which
    // is the only state in which CFGR1/CFGR2 are writable at all (RM0444: the
    // configuration registers are write-protected while ADSTART is set, and
    // ST's LL driver documents the whole group as "ADC must be disabled").
    // Doing it here rather than through a later reconfigure() is why this
    // driver needs no stop/disable/re-enable cycle for a knob.
    //
    // Every check below is a static_assert against GENERATED data, so none of
    // them can drift from the silicon, and all of them fire at compile time
    // because `O` is a template argument.
    template <adc_opts<Inst> O>
    static void configure() {
        // --- resolution -------------------------------------------------
        static_assert(O.resolution_bits == 12u || O.resolution_bits == 10u ||
                          O.resolution_bits == 8u || O.resolution_bits == 6u,
                      "st/adc_v2 converts at 12, 10, 8 or 6 bits");
        constexpr std::uint32_t res_code = (12u - O.resolution_bits) / 2u;
        static_assert(res_code <= IP::res.raw_mask, "resolution code exceeds CFGR1.RES");
        if constexpr (res_code != 0u) {
            IP::res.write(r(), res_code);
        }

        // --- oversampling -----------------------------------------------
        // The ratio is a POWER OF TWO by construction of OVSR, so a value
        // that is not one cannot be encoded; saying so at compile time is
        // better than rounding a caller's 100 down to 64 in silence.
        static_assert(O.oversample_ratio >= 1u && O.oversample_ratio <= 256u,
                      "st/adc_v2 oversamples 2 to 256 samples per result (1 = off)");
        static_assert((O.oversample_ratio & (O.oversample_ratio - 1u)) == 0u,
                      "st/adc_v2's oversampling ratio is a power of two");
        static_assert(O.oversample_shift <= 8u,
                      "st/adc_v2 shifts an oversampled sum right by at most 8 bits");
        if constexpr (O.oversample_ratio > 1u) {
            IP::ovsr.write(r(), log2_of(O.oversample_ratio) - 1u);
            IP::ovss.write(r(), O.oversample_shift);
            IP::ovse.set(r());
        }

        // --- tracking time ----------------------------------------------
        // SMP1 is written unconditionally: enable() already needs the slowest
        // time for the internal vref/temp channels, and this replaces that
        // write rather than fighting it.
        IP::smp1.write(r(), static_cast<std::uint32_t>(O.sample_time));
        if constexpr (O.sample_time_alt_channels != 0u) {
            IP::smp2.write(r(), static_cast<std::uint32_t>(O.sample_time_alt));
            // SMPSEL is one bit per channel, generated as a repeat field, so
            // the whole assignment is one write of the caller's mask.
            IP::smpsel.write(r(), O.sample_time_alt_channels);
        }

        // --- hardware trigger -------------------------------------------
        if constexpr (O.trigger_edge != st_adc_v2_trigger_edge::software) {
            IP::extsel.write(r(), static_cast<std::uint32_t>(O.trigger));
            IP::exten.write(r(), static_cast<std::uint32_t>(O.trigger_edge));
        }
    }

    // What the data register's results are worth after configure<O>(): the
    // number of significant bits. The facade needs it to decide whether an
    // analog watchdog window can still be expressed in a 12-bit threshold
    // field, and a caller needs it to know what read() means.
    //
    // Oversampling sums `ratio` samples (log2(ratio) extra bits) and shifts
    // the sum right by `shift`, so the width is resolution + log2(ratio) -
    // shift, never below the resolution and never above 16 (the data
    // register's own width).
    template <adc_opts<Inst> O>
    [[nodiscard]] static consteval unsigned result_bits() {
        unsigned bits = O.resolution_bits;
        if (O.oversample_ratio > 1u) {
            const unsigned gained = log2_of(O.oversample_ratio);
            bits = bits + gained - (O.oversample_shift < gained ? O.oversample_shift : gained);
        }
        return bits > 16u ? 16u : bits;
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t channel) {
        using isr = typename IP::isr;
        r().CHSELR = std::uint32_t{1} << channel;
        while ((r().ISR & isr::ccrdy) == 0u) {
        }
        r().ISR = isr::eoc | isr::ccrdy;  // clear stale flags
        IP::adstart.set(r());
        while ((r().ISR & isr::eoc) == 0u) {
        }
        return static_cast<std::uint16_t>(r().DR);
    }

    // --- DMA burst hooks (RM0444: DMAEN/DMACFG/CONT only writable while
    // ADSTART=0). begin() configures but does NOT start — the caller arms
    // the DMA channel first, then kick() starts conversions. ---
    static void dma_burst_begin(std::uint8_t channel) {
        using isr = typename IP::isr;
        r().CHSELR = std::uint32_t{1} << channel;
        while ((r().ISR & isr::ccrdy) == 0u) {
        }
        // Flag hygiene (w1c): stale CCRDY breaks the NEXT channel-select
        // handshake; stale EOC/EOS/OVR from a previous burst BLOCK the
        // ADC's DMA requests entirely (RM0444 overrun management) and
        // dma.wait() would spin forever.
        r().ISR = isr::ccrdy | isr::eoc | isr::eos | isr::ovr;
        IP::dmaen.set(r());
        IP::cont.set(r());
        IP::ovrmod.set(r());  // overwrite on overrun keeps the stream honest
    }

    static void dma_burst_kick() { IP::adstart.set(r()); }

    static void dma_burst_end() {
        using cr = typename IP::cr;
        IP::adstp.set(r());
        while ((r().CR & cr::adstart) != 0u) {
        }
        IP::cont.clear(r());
        IP::dmaen.clear(r());
        IP::ovrmod.clear(r());
    }

    [[nodiscard]] static std::uintptr_t dr_addr() {
        return reinterpret_cast<std::uintptr_t>(&r().DR);
    }

    // ── analog watchdogs ────────────────────────────────────────────────
    //
    // THREE of them on this IP, in TWO shapes, which is why every hook below
    // branches on the ordinal at COMPILE time and none of them takes it as an
    // argument. Watchdog 1's enable and its single-channel selection are
    // fields of CFGR1; watchdogs 2 and 3 own a 19-bit channel bitmask register
    // each, and a non-zero mask IS the enable — there is no AWD2EN bit to set.
    // The three threshold registers are AWD1TR, AWD2TR and AWD3TR at 0x20,
    // 0x24 and 0x2C, which is not a stride: 0x28 in the middle of them is
    // CHSELR. So the register map offers no array to index and a runtime
    // ordinal could not be expressed without a switch over three unrelated
    // members — the ordinal is a template parameter because the silicon says
    // so, not for elegance.

    template <unsigned N>
    static void awd_arm(const alloy::hal::adc_watchdog_config& cfg) {
        static_assert(N < 3u, "st/adc_v2 has three analog watchdogs");
        // The thresholds are 12-bit fields; a wider value would be truncated
        // and the watchdog would guard a window nobody asked for. The bound is
        // the GENERATED field width, so it cannot drift from the silicon.
        const bool fits = cfg.low <= IP::lt1.raw_mask && cfg.high <= IP::ht1.raw_mask;
        if (__builtin_constant_p(fits) && !fits) {
            alloy::core::admit::adc_watchdog_threshold();
        }
        if (!fits) {
            alloy::trap<alloy::trap_code::impossible_config>();
        }
        const bool was_enabled = enabled();
        disable_for_reconfig();
        if constexpr (N == 0u) {
            IP::lt1.write(r(), cfg.low);
            IP::ht1.write(r(), cfg.high);
            IP::awd1ch.write(r(), cfg.channel);
            IP::awd1sgl.set(r());  // this one channel, not every converted one
            IP::awd1en.set(r());
        } else if constexpr (N == 1u) {
            IP::lt2.write(r(), cfg.low);
            IP::ht2.write(r(), cfg.high);
            IP::awd2ch.write(r(), std::uint32_t{1} << cfg.channel);
        } else {
            IP::lt3.write(r(), cfg.low);
            IP::ht3.write(r(), cfg.high);
            IP::awd3ch.write(r(), std::uint32_t{1} << cfg.channel);
        }
        if (was_enabled) {
            re_enable();
        }
    }

    template <unsigned N>
    static void awd_disarm() {
        static_assert(N < 3u, "st/adc_v2 has three analog watchdogs");
        const bool was_enabled = enabled();
        disable_for_reconfig();
        if constexpr (N == 0u) {
            IP::awd1en.clear(r());
        } else if constexpr (N == 1u) {
            IP::awd2ch.write(r(), 0u);
        } else {
            IP::awd3ch.write(r(), 0u);
        }
        if (was_enabled) {
            re_enable();
        }
    }

    template <unsigned N>
    [[nodiscard]] static bool awd_tripped() {
        static_assert(N < 3u, "st/adc_v2 has three analog watchdogs");
        using isr = typename IP::isr;
        if constexpr (N == 0u) {
            return (r().ISR & isr::awd1) != 0u;
        } else if constexpr (N == 1u) {
            return (r().ISR & isr::awd2) != 0u;
        } else {
            return (r().ISR & isr::awd3) != 0u;
        }
    }

    template <unsigned N>
    static void awd_clear() {
        static_assert(N < 3u, "st/adc_v2 has three analog watchdogs");
        // w1c, and ONLY this bit: ISR is a write-one-to-clear register, so a
        // read-modify-write would acknowledge every other pending flag too.
        if constexpr (N == 0u) {
            r().ISR = IP::awd1.mask;
        } else if constexpr (N == 1u) {
            r().ISR = IP::awd2.mask;
        } else {
            r().ISR = IP::awd3.mask;
        }
    }

private:
    [[nodiscard]] static bool enabled() {
        using cr = typename IP::cr;
        return (r().CR & cr::aden) != 0u;
    }

    // ST's own LL driver (stm32g0xx_ll_adc.h, LL_ADC_SetAnalogWDMonitChannels)
    // documents the monitored-channel write as "conditioned to ADC state: ADC
    // must be disabled" — for CFGR1's AWD1CH/AWD1SGL/AWD1EN and for the
    // AWD2CR/AWD3CR masks alike. So arming a watchdog on a live port is not a
    // register poke, it is a stop / disable / program / re-enable cycle. The
    // caller's conversions resume afterwards; nothing else is reprogrammed,
    // and the factory calibration in CALFACT survives a disable.
    static void disable_for_reconfig() {
        using cr = typename IP::cr;
        if (!enabled()) {
            return;
        }
        if ((r().CR & cr::adstart) != 0u) {
            IP::adstp.set(r());
            while ((r().CR & cr::adstart) != 0u) {
            }
        }
        IP::addis.set(r());
        while ((r().CR & cr::aden) != 0u) {
        }
    }

    static void re_enable() {
        using isr = typename IP::isr;
        r().ISR = IP::adrdy.mask;  // w1c the stale ready flag
        IP::aden.set(r());
        while ((r().ISR & isr::adrdy) == 0u) {
        }
    }
};

}  // namespace alloy::hal
