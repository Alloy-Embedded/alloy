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
        // 16x oversample, no shift: 12-bit codes fill the 16-bit range.
        IP::ovsr.write(r(), 3u);
        IP::ovss.write(r(), 0u);
        IP::rovse.set(r());

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
};

}  // namespace alloy::hal
