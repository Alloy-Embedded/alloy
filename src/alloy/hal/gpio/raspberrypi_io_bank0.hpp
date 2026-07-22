// GPIO driver for the RP2040 (io_bank0 mux + SIO fast I/O + pads).
//
// BEHAVIOR only: bases, gates and companion blocks come from generated
// descriptors (io_bank0's instance carries sio_t/pads_t companions).
// SIO gives atomic OUT_SET/OUT_CLR/OUT_XOR — toggle is a single write.
// Pads reset to 0x56 (IE=1, OD=0), so outputs and UART work at defaults;
// pads are only touched for pulls.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/ip/raspberrypi/io_bank0.hpp"
#include "alloy/ip/raspberrypi/pads_bank0.hpp"
#include "alloy/ip/raspberrypi/sio.hpp"

namespace alloy::hal {

template <class Pin>
    requires std::same_as<typename Pin::port_t::ip, alloy::ip::raspberrypi::io_bank0>
struct pin_impl<Pin> {
    using Bank = typename Pin::port_t;
    using IP = typename Bank::ip;
    using Sio = typename Bank::sio_t;
    using Pads = typename Bank::pads_t;
    static constexpr unsigned index = Pin::index;
    static constexpr std::uint32_t bit = 1u << index;
    static_assert(index < IP::GPIO_CTRL_count, "RP2040 bank0 has 30 GPIOs");

    static constexpr std::uint8_t kFuncselSio = 5;  // datasheet §2.19.2 F5 = SIO

    static typename Sio::ip::regs& sio() {
        return *reinterpret_cast<typename Sio::ip::regs*>(Sio::base);
    }
    static rw32& ctrl() {
        return alloy::reg_at(Bank::base, IP::GPIO_CTRL_offset, IP::GPIO_CTRL_stride, index);
    }

    static void release_resets() {
        alloy::gate_on(Bank::gate);
        alloy::gate_on(Pads::gate);
    }

    static void make_output() {
        release_resets();
        IP::funcsel.write(ctrl(), kFuncselSio);
        sio().GPIO_OE_SET = bit;
    }

    static void make_input() {
        release_resets();
        IP::funcsel.write(ctrl(), kFuncselSio);
        sio().GPIO_OE_CLR = bit;
    }

    static void make_input_pullup() {
        make_input();
        auto& pad = alloy::reg_at(Pads::base, Pads::ip::GPIO_offset,
                                  Pads::ip::GPIO_stride, index);
        Pads::ip::pde.write(pad, 0u);
        Pads::ip::pue.write(pad, 1u);
    }

    static void make_af(std::uint8_t funcsel) {
        release_resets();
        IP::funcsel.write(ctrl(), funcsel);
    }

    static void set_high() { sio().GPIO_OUT_SET = bit; }
    static void set_low() { sio().GPIO_OUT_CLR = bit; }
    static void toggle() { sio().GPIO_OUT_XOR = bit; }

    [[nodiscard]] static bool read() { return (sio().GPIO_IN & bit) != 0u; }
};

}  // namespace alloy::hal
