// GPIO driver for the Microchip SAM PIO controller (pio_v1: E70/S70/V71).
//
// BEHAVIOR only: bases, gates and the mux_unlock come from generated
// descriptors. PIO uses write-only SET/CLEAR register pairs — masks are
// written whole, never RMW'd. Peripheral function select: two rw registers
// ABCDSR1/2, af bit0 -> ABCDSR1, af bit1 -> ABCDSR2 (A=0 B=1 C=2 D=3).

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/ip/microchip/pio_v1.hpp"

namespace alloy::hal {

template <class Pin>
    requires std::same_as<typename Pin::port_t::ip, alloy::ip::microchip::pio_v1>
struct pin_impl<Pin> {
    using Port = typename Pin::port_t;
    using IP = typename Port::ip;
    static constexpr std::uint32_t bit = 1u << Pin::index;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Port::base);
    }

    static void make_output() {
        alloy::gate_on(Port::gate);
        r().PER = bit;  // pin -> PIO control
        r().OER = bit;  // output enable
    }

    static void make_input() {
        alloy::gate_on(Port::gate);
        r().PER = bit;
        r().ODR = bit;  // output disable
    }

    static void make_input_pullup() {
        make_input();
        r().PPDDR = bit;  // pull-down off first (mutually exclusive)
        r().PUER = bit;
    }

    static void make_af(std::uint8_t af) {
        alloy::gate_on(Port::gate);
        if constexpr (requires { Pin::mux_unlock; }) {
            alloy::gate_on(Pin::mux_unlock);  // release from system function (JTAG etc.)
        }
        if (af & 0b01u) {
            r().ABCDSR1 = r().ABCDSR1 | bit;
        } else {
            r().ABCDSR1 = r().ABCDSR1 & ~bit;
        }
        if (af & 0b10u) {
            r().ABCDSR2 = r().ABCDSR2 | bit;
        } else {
            r().ABCDSR2 = r().ABCDSR2 & ~bit;
        }
        r().PDR = bit;  // hand the pin to the peripheral
    }

    static void set_high() { r().SODR = bit; }
    static void set_low() { r().CODR = bit; }

    static void toggle() {
        if (r().ODSR & bit) {
            set_low();
        } else {
            set_high();
        }
    }

    [[nodiscard]] static bool read() { return (r().PDSR & bit) != 0u; }
};

}  // namespace alloy::hal
