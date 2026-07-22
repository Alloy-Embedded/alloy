// UART driver for the Microchip SAM full USART (usart_v1: E70-era), async 8N1.
//
// BEHAVIOR only: every base/bit comes from the generated IP header and
// instance descriptor. The MR write is explicit about MODE/USCLKS/CHRL/PAR —
// the old ecosystem drove this IP through the plain-UART model and silently
// got 5-bit characters (CHRL=0); the field masks below make that mistake
// impossible to repeat quietly.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/microchip/usart_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::usart_v1>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t baud) {
        alloy::gate_on(Inst::gate);
        // CR is write-only: whole-value command writes composed from field masks.
        r().CR = IP::rstrx.mask | IP::rsttx.mask | IP::rxdis.mask |
                 IP::txdis.mask | IP::rststa.mask;
        // MR: normal mode, MCK, 8-bit, no parity (reset value is 0, RMW is safe).
        IP::mode.write(r(), 0u);
        IP::usclks.write(r(), 0u);
        IP::chrl.write(r(), 3u);
        IP::par.write(r(), 4u);
        // Fixed 16x oversampling: CD = round(kernel / (16 * baud)).
        IP::cd.write(r(), (kernel_hz + 8u * baud) / (16u * baud));
        r().CR = IP::rxen.mask | IP::txen.mask;
    }

    static void write(std::uint8_t byte) {
        while (IP::txrdy.read(r()) == 0u) {
        }
        r().THR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxrdy.read(r()) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().RHR);
        return true;
    }

    static void flush() {
        while (IP::txempty.read(r()) == 0u) {
        }
    }
};

}  // namespace alloy::hal
