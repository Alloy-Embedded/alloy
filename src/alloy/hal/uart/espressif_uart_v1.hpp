// UART driver for the classic ESP32 in ROM-configured mode.
//
// The boot ROM + 2nd-stage bootloader leave UART0 at 115200 8N1 with pins
// routed via IO_MUX — hardware-validated in the old ecosystem with exactly
// this FIFO/STATUS-only access pattern. enable() therefore programs
// nothing; full CLKDIV/CONF0 bring-up arrives with matrix routing support.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/espressif/uart_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::espressif::uart_v1>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t, std::uint32_t) {
        // ROM-configured (115200 8N1); nothing to program in v1.
    }

    static void write(std::uint8_t byte) {
        // TX FIFO is 128 deep; leave headroom (validated threshold: 126).
        while (IP::txfifo_cnt.read(r()) >= 126u) {
        }
        r().FIFO = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxfifo_cnt.read(r()) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().FIFO);
        return true;
    }

    static void flush() {
        while (IP::txfifo_cnt.read(r()) != 0u) {
        }
    }
};

}  // namespace alloy::hal
