// PWM driver for ST 16-bit general-purpose timers (tim_gp16: TIM2/TIM3
// class). PWM mode 1 on channels 1-4, edge- or centre-aligned, with a
// prescaler DERIVED from the requested carrier rather than a fixed tick —
// see st_tim_timebase.hpp for why that is not a detail.
//
// Optionally drives the timer's trigger output (CR2.MMS), so a converter can
// be started by the counter instead of by the CPU. The facade refuses that
// request on a block whose curated data reports no trigger output.
//
// BEHAVIOR only: bases/gates/fields from generated headers. Advanced
// timers (TIM1) need BDTR.MOE and are not served by this driver — a
// complementary bridge is alloy::bridge, a different personality.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/pwm/pwm_impl.hpp"
#include "alloy/hal/pwm/st_tim_timebase.hpp"
#include "alloy/ip/st/tim_gp16.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::tim_gp16>
struct pwm_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static inline std::uint32_t period_ticks = 0;

    //: The counter's span, from the CURATED field width rather than from the
    //: constant 65536 — a narrower ARR needs no edit here.
    static constexpr std::uint32_t max_period_ticks = IP::arr.wide_raw_mask + 1u;

    //: Layer 1's alignment/trigger in this silicon's vocabulary.
    static constexpr std::uint32_t mms_bits(alloy::hal::pwm_trigger t) {
        switch (t) {
            case alloy::hal::pwm_trigger::on_update:
                return static_cast<std::uint32_t>(IP::cr2::mms_update);
            case alloy::hal::pwm_trigger::on_compare:
                // OCxREF of THIS channel would need the channel ordinal; the
                // block-scoped answer that works for any of them is OC1REF,
                // and callers who want another channel's compare should say
                // so through a channel-scoped API that does not exist yet.
                return static_cast<std::uint32_t>(IP::cr2::mms_oc1ref);
            case alloy::hal::pwm_trigger::none:
                break;
        }
        return 0u;
    }

    static void enable(std::uint32_t kernel_hz, alloy::hal::pwm_config c, unsigned channel) {
        const std::uint32_t freq_hz = c.freq_hz;
        alloy::gate_on(Inst::gate);
        // CR2 on a general-purpose timer carries no output-idle state — that
        // is an advanced-timer register — so this is an ordinary write with
        // no safety argument attached, unlike the bridge's.
        r().CR2 = mms_bits(c.trigger);
        // The prescaler is DERIVED, not fixed. This used to pin a 1 MHz tick,
        // which at 20 kHz leaves fifty counts of duty — under six bits, and
        // invisible from an API that still takes a 16-bit number. Now PSC is
        // the smallest that makes the period fit, so the caller gets every
        // count the silicon has (3200 at 64 MHz / 20 kHz).
        //: Centre-aligned counts BOTH WAYS, so one period is 2*ARR ticks and
        //: the reload is half the edge-aligned one for the same carrier —
        //: which also halves the duty steps, the trade the caller made.
        const bool center = c.align == alloy::hal::pwm_alignment::center;
        const detail::tim_timebase tb = detail::tim_timebase_for(
            kernel_hz, center ? freq_hz * 2u : freq_hz, max_period_ticks);
        r().PSC = tb.psc;
        period_ticks = tb.arr + 1u;
        r().ARR = tb.arr;
        if (center) {
            IP::cms.write(r(), 1u);  // centre-aligned mode 1
        }

        constexpr std::uint32_t kPwmMode1 = 6u;
        switch (channel) {
            case 1:
                IP::oc1m.write(r(), kPwmMode1);
                IP::oc1pe.set(r());
                IP::cc1e.set(r());
                break;
            case 2:
                IP::oc2m.write(r(), kPwmMode1);
                IP::oc2pe.set(r());
                IP::cc2e.set(r());
                break;
            case 3:
                IP::oc3m.write(r(), kPwmMode1);
                IP::oc3pe.set(r());
                IP::cc3e.set(r());
                break;
            case 4:
                IP::oc4m.write(r(), kPwmMode1);
                IP::oc4pe.set(r());
                IP::cc4e.set(r());
                break;
            default:
                return;
        }
        IP::arpe.set(r());
        r().EGR = IP::ug.mask;  // latch PSC/ARR
        IP::cen.set(r());
    }

    static void set_duty(unsigned channel, std::uint16_t duty) {
        const std::uint32_t ccr =
            (static_cast<std::uint32_t>(duty) * period_ticks) / 65535u;
        switch (channel) {
            case 1: r().CCR1 = ccr; break;
            case 2: r().CCR2 = ccr; break;
            case 3: r().CCR3 = ccr; break;
            case 4: r().CCR4 = ccr; break;
            default: break;
        }
    }

    // --- DMA hooks: waveform streaming on the UPDATE request. Items
    // written by DMA are RAW compare values (0..period_ticks()-1), not
    // normalized duties — scale on the memory side. ---
    static void dma_update_begin() { IP::ude.set(r()); }
    static void dma_update_end() { IP::ude.clear(r()); }
    // (period_ticks — the raw tick count per period — already exists above
    // as the driver's static state, set by enable().)

    [[nodiscard]] static std::uintptr_t duty_addr(unsigned channel) {
        switch (channel) {
            case 1: return reinterpret_cast<std::uintptr_t>(&r().CCR1);
            case 2: return reinterpret_cast<std::uintptr_t>(&r().CCR2);
            case 3: return reinterpret_cast<std::uintptr_t>(&r().CCR3);
            default: return reinterpret_cast<std::uintptr_t>(&r().CCR4);
        }
    }
};

}  // namespace alloy::hal
