// ADC driver for the ST single-ADC-with-CHSELR IP (adc_v2: F0/G0 style).
//
// Blocking single conversions, 12-bit right-aligned, slowest sample time
// (required for the internal vref/temp channels and safe everywhere).
// Bring-up follows RM0444: voltage regulator on, calibrate, enable
// internal sources, ADEN + ADRDY.

#pragma once

#include <concepts>
#include <cstdint>

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
        alloy::gate_on(Inst::gate);
        IP::ckmode.write(r(), 1u);  // PCLK/2 synchronous
        IP::advregen.set(r());
        spin(kernel_hz / 50'000u);  // >= 20 us regulator start-up
        IP::adcal.set(r());
        while (IP::adcal.read(r()) != 0u) {
        }
        IP::vrefen.set(r());
        IP::tsen.set(r());
        spin(kernel_hz / 50'000u);  // temp sensor start-up
        IP::smp1.write(r(), 7u);    // 160.5 cycles — required for vref/temp
        r().ISR = IP::adrdy.mask;   // w1c
        IP::aden.set(r());
        while (IP::adrdy.read(r()) == 0u) {
        }
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t channel) {
        r().CHSELR = 1u << channel;
        while (IP::ccrdy.read(r()) == 0u) {
        }
        r().ISR = IP::eoc.mask | IP::ccrdy.mask;  // clear stale flags
        IP::adstart.set(r());
        while (IP::eoc.read(r()) == 0u) {
        }
        return static_cast<std::uint16_t>(r().DR);
    }
};

}  // namespace alloy::hal
