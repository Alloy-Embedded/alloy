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
        IP::smp1.write(r(), 7u);    // 160.5 cycles — required for vref/temp
        r().ISR = IP::adrdy.mask;   // w1c
        IP::aden.set(r());
        while ((r().ISR & isr::adrdy) == 0u) {
        }
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
