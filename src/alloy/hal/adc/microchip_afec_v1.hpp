// ADC driver for the SAM AFEC (E70-era), blocking single conversions.
//
// Silicon quirks honored: MR.ONE must be written 1; per-channel COCR
// offset DAC must sit mid-scale (0x200) or readings are shifted; CSELR
// selects the channel for both COCR and CDR. AFEC clock kept <= 40 MHz
// via MR.PRESCAL.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/adc/adc_impl.hpp"
#include "alloy/ip/microchip/afec_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::afec_v1>
struct adc_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz) {
        alloy::gate_on(Inst::gate);
        r().CR = IP::swrst.mask;
        // fAFE = kernel/(PRESCAL+1), capped at 40 MHz.
        std::uint32_t prescal = kernel_hz / 40'000'000u;
        r().MR = (prescal << IP::prescal.pos) | (4u << IP::startup.pos) |
                 IP::one.mask | (2u << IP::tracktim.pos) | (2u << IP::transfer.pos);
        r().EMR = IP::tag.mask;
        r().ACR = 1u << IP::ibctl.pos;
        r().CGR = 0;      // gain 1 on all channels
        r().DIFFR = 0;    // single-ended
        r().TEMPMR = 0;   // temp sensor sampled like any channel
    }

    [[nodiscard]] static std::uint16_t read(std::uint8_t channel) {
        // Mid-scale offset compensation for this channel (required).
        r().CSELR = channel;
        r().COCR = 0x200u;
        r().CHER = 1u << channel;
        r().CR = IP::start.mask;
        while ((r().ISR & (1u << channel)) == 0u) {
        }
        r().CSELR = channel;
        const auto data = static_cast<std::uint16_t>(r().CDR & 0xFFFu);
        r().CHDR = 1u << channel;
        return data;
    }
};

}  // namespace alloy::hal
