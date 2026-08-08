// GPIO driver for the Renesas port_g14 IP (RL78/G14 I/O ports).
//
// BEHAVIOR only: every address and offset comes from the generated
// alloy::ip::renesas::port_g14 header and the generated port descriptor.
//
// Two things here are not true of any other GPIO driver in this tree, and both
// change what callers can assume:
//
//   * there is no BSRR. On ST and RP2040 a pin write is one store to a
//     set/clear register, which is atomic by construction. RL78 has to
//     read-modify-write the port latch, so a write from an ISR racing a write
//     from the main loop can lose one. Where that matters the RL78 has SET1 /
//     CLR1 — single-bit instructions the compiler emits from exactly the shape
//     used below, which is why these are written as `|=` and `&=` rather than
//     through a helper that would hide it.
//   * the port spans two address windows. Direction and data live in the SFR
//     area; pull-up, open-drain and the analogue/digital switch live in the
//     2nd SFR area ~64 KB away. The generated descriptor carries both bases.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/ip/renesas/port_g14.hpp"

namespace alloy::hal {

template <class Pin>
    requires std::same_as<typename Pin::port_t::ip, alloy::ip::renesas::port_g14>
struct pin_impl<Pin> {
    using Port = typename Pin::port_t;
    using IP = typename Port::ip;
    static constexpr unsigned index = Pin::index;
    static_assert(index < 8, "renesas/port_g14 ports are 8 pins wide");

    static constexpr std::uint8_t bit = static_cast<std::uint8_t>(std::uint32_t{1} << index);

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Port::base);
    }
    //: The second window. A port that did not declare it is a data error, not
    //: something to paper over at runtime — the static_assert says so at the
    //: point of use rather than letting `base_sfr2` default to zero.
    static typename IP::regs_sfr2& r2() {
        static_assert(Port::base_sfr2 != 0,
                      "this port declares no sfr2 window; the chip data is "
                      "incomplete for renesas/port_g14");
        return *reinterpret_cast<typename IP::regs_sfr2*>(Port::base_sfr2);
    }

    // PMC is the analogue/digital switch and its RESET VALUE is 1 on every pin
    // that shares with the ADC — the pin starts OUTSIDE the digital domain, so
    // a driver that only sets the direction gets nothing. Clearing it first is
    // the step that is easy to leave out and impossible to see.
    static void make_digital() { r2().PMC = static_cast<std::uint8_t>(r2().PMC & ~bit); }

    static void make_output() {
        make_digital();
        // PM: 0 = output. Reset is 0xFF, so every pin is an input until told.
        r().PM = static_cast<std::uint8_t>(r().PM & ~bit);
    }

    static void make_input() {
        make_digital();
        r().PM = static_cast<std::uint8_t>(r().PM | bit);
    }

    static void make_input_pullup() {
        make_input();
        r2().PU = static_cast<std::uint8_t>(r2().PU | bit);
    }

    // Open drain, for a shared line. The internal pull-up is weak; a real bus
    // wants external ones.
    static void make_output_od() {
        make_output();
        r2().POM = static_cast<std::uint8_t>(r2().POM | bit);
    }

    static void set_high() { r().P = static_cast<std::uint8_t>(r().P | bit); }
    static void set_low() { r().P = static_cast<std::uint8_t>(r().P & ~bit); }

    static void toggle() { r().P = static_cast<std::uint8_t>(r().P ^ bit); }

    [[nodiscard]] static bool read() { return (r().P & bit) != 0u; }

    // --- port-level primitives for gpio::bus ---------------------------------
    //
    // NOT atomic, unlike every other backend's. ST and RP2040 land set and
    // clear in one store; here it is a read, a mask and a write, so an
    // interrupt that touches this port in between is lost. A bus driven from
    // both an ISR and the main loop needs a critical section around it, and
    // saying that here is more useful than a helper that pretends otherwise.
    static void write_masked(std::uint32_t set_bits, std::uint32_t clear_bits) {
        const std::uint8_t cur = r().P;
        r().P = static_cast<std::uint8_t>(
            (cur & ~static_cast<std::uint8_t>(clear_bits)) |
            static_cast<std::uint8_t>(set_bits));
    }

    [[nodiscard]] static std::uint32_t read_port() {
        return static_cast<std::uint32_t>(r().P);
    }
};

}  // namespace alloy::hal
