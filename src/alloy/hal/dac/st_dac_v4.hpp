// DAC channel-1 output driver for the STM32 DAC "v4" (G0/G4/L5-era).
//
// BEHAVIOR only: register overlay + gate come from generated data. RM0444 §17:
// enable the channel (CR.EN1); with the trigger off (CR.TEN1=0, reset default)
// a 12-bit code written to DHR12R1 transfers to the output register (DOR1 ->
// DAC_OUT1 pin) automatically after one APB cycle. The output buffer +
// external-pin routing are the MCR reset default, so no MCR write is needed;
// the pin (PA4) resets to analog, so the GPIO needs no setup either.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/dac/dac_impl.hpp"
#include "alloy/ip/st/dac_v4.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::dac_v4>
struct dac_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable() {
        alloy::gate_on(Inst::gate);
        IP::en1.set(r());  // CR.EN1 = 1 (TEN1 stays 0 -> DHR loads DOR directly)
    }

    static void write(std::uint16_t code) {
        r().DHR12R1 = code & 0xFFFu;  // 12-bit right-aligned -> DOR1 -> DAC_OUT1
    }
};

}  // namespace alloy::hal
