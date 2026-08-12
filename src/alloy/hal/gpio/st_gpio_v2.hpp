// GPIO driver for the ST gpio_v2 IP (STM32G0/G4/L4/F7/H7-era GPIO block).
//
// BEHAVIOR only: every address, offset and field position comes from the
// generated alloy::ip::st::gpio_v2 header and the generated port descriptor.
// This one driver serves every chip whose data tags its GPIO ports gpio_v2.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/exti/exti_impl.hpp"
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

    static void set_high() { r().BSRR = std::uint32_t{1} << index; }
    static void set_low() { r().BSRR = std::uint32_t{1} << (index + 16u); }

    static void toggle() {
        if ((r().ODR >> index) & 1u) {
            set_low();
        } else {
            set_high();
        }
    }

    [[nodiscard]] static bool read() { return ((r().IDR >> index) & 1u) != 0u; }

    // --- port-level primitives for gpio::bus: touch several pins on THIS pin's
    // port at once. write_masked is one BSRR store (atomic, glitch-free — the
    // set and clear happen in the same cycle). ---
    static void write_masked(std::uint32_t set_bits, std::uint32_t clear_bits) {
        r().BSRR = set_bits | (clear_bits << 16u);
    }
    [[nodiscard]] static std::uint32_t read_port() { return r().IDR; }

    // --- pin interrupts ------------------------------------------------------
    //
    // The pin-interrupt block is a SEPARATE peripheral on ST, so this driver
    // reaches it through the port's `exti` COMPANION — the same shape dma_v1
    // uses for its DMAMUX, and for the same reason: which controller serves a
    // port is a chip fact, not something a GPIO driver may assume. Its
    // specialization (hal/exti/st_exti_g0.hpp) is auto-included by codegen for
    // any chip whose EXTI is curated; only exti_impl's primary template is
    // included here, so a chip with no EXTI still compiles.
    //
    // Every method below is `requires`-gated on the companion AND on
    // port_index. A chip whose EXTI is not curated has neither, so these
    // methods DO NOT EXIST there and gpio::input's own gates report that
    // truthfully instead of failing deep inside a body.
    //
    // NOTE the pad is left in whatever mode it is already in. EXTI samples the
    // input path, and re-running make_input() here would quietly undo a
    // pull-up, an open-drain setting, or an alternate function the caller
    // deliberately chose. Only the port clock is forced on, because a gated
    // port drives nothing into the controller.
    static void enable_edge_irq(pin_edge e, void (*fn)(void*), void* ctx)
        requires requires { typename Port::exti_t; Port::port_index; }
    {
        alloy::gate_on(Port::gate);
        exti_impl<typename Port::exti_t>::template arm<Port::port_index, index>(e, fn, ctx);
    }

    static void disable_edge_irq()
        requires requires { typename Port::exti_t; Port::port_index; }
    {
        exti_impl<typename Port::exti_t>::template disarm<Port::port_index, index>();
    }

    [[nodiscard]] static std::uint32_t edge_count()
        requires requires { typename Port::exti_t; Port::port_index; }
    {
        return exti_impl<typename Port::exti_t>::template edges<index>();
    }

    [[nodiscard]] static bool take_edge()
        requires requires { typename Port::exti_t; Port::port_index; }
    {
        return exti_impl<typename Port::exti_t>::template take_edge<index>();
    }
};

}  // namespace alloy::hal
