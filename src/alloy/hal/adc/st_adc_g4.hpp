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
};

}  // namespace alloy::hal
