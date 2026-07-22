// PWM driver for ST 16-bit general-purpose timers (tim_gp16: TIM2/TIM3
// class). Edge-aligned PWM mode 1, 1 MHz timebase tick, channels 1-4.
//
// BEHAVIOR only: bases/gates/fields from generated headers. Advanced
// timers (TIM1) need BDTR.MOE and are not served by this driver.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/pwm/pwm_impl.hpp"
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

    static void enable(std::uint32_t kernel_hz, std::uint32_t freq_hz, unsigned channel) {
        alloy::gate_on(Inst::gate);
        // 1 MHz timer tick; period in ticks caps at 16 bits.
        r().PSC = kernel_hz / 1'000'000u - 1u;
        period_ticks = 1'000'000u / freq_hz;
        r().ARR = period_ticks - 1u;

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
