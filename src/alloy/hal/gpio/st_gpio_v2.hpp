// GPIO driver for the ST gpio_v2 IP (STM32G0/G4/L4/F7/H7-era GPIO block).
//
// BEHAVIOR only: every address, offset and field position comes from the
// generated alloy::ip::st::gpio_v2 header and the generated port descriptor.
// This one driver serves every chip whose data tags its GPIO ports gpio_v2.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/ip/st/gpio_v2.hpp"

namespace alloy::hal {

template <class Pin>
    requires std::same_as<typename Pin::port_t::ip, alloy::ip::st::gpio_v2>
struct pin_impl<Pin> {
    using Port = typename Pin::port_t;
    using IP = typename Port::ip;
    static constexpr unsigned index = Pin::index;
    static_assert(index < 16, "st/gpio_v2 ports have 16 pins");

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Port::base);
    }

    static void make_output() {
        alloy::gate_on(Port::gate);
        IP::template moder<index>.write(r(), 0b01);  // general-purpose output
    }

    static void make_input() {
        alloy::gate_on(Port::gate);
        IP::template moder<index>.write(r(), 0b00);
    }

    static void make_input_pullup() {
        make_input();
        IP::template pupdr<index>.write(r(), 0b01);
    }

    static void make_af(std::uint8_t af) {
        alloy::gate_on(Port::gate);
        if constexpr (index < 8) {
            IP::template afrl<index>.write(r(), af);
        } else {
            IP::template afrh<index - 8>.write(r(), af);
        }
        IP::template moder<index>.write(r(), 0b10);  // alternate function
    }

    // AF with open-drain + weak internal pull-up (I2C pads). The ~40 kΩ
    // internal pull is enough for short-wire scanning; real buses want
    // external pull-ups.
    static void make_af_od(std::uint8_t af) {
        make_af(af);
        IP::template ot<index>.write(r(), 1u);
        IP::template pupdr<index>.write(r(), 0b01);
    }

    static void set_high() { r().BSRR = 1u << index; }
    static void set_low() { r().BSRR = 1u << (index + 16u); }

    static void toggle() {
        if ((r().ODR >> index) & 1u) {
            set_low();
        } else {
            set_high();
        }
    }

    [[nodiscard]] static bool read() { return ((r().IDR >> index) & 1u) != 0u; }
};

}  // namespace alloy::hal
