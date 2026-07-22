// UART driver for the ST usart_v4 IP (USART with PRESC/FIFO — G0/G4/H7/L5 era).
//
// BEHAVIOR only: every address, offset and field position comes from the
// generated alloy::ip::st::usart_v4 header and the generated instance
// descriptor. Blocking byte I/O for the walking skeleton; IRQ/DMA later.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/usart_v4.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v4>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t baud) {
        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());
        // OVER8=0, PRESC=0: BRR = kernel/baud, rounded to nearest.
        r().BRR = (kernel_hz + baud / 2u) / baud;
        IP::te.set(r());
        IP::re.set(r());
        IP::ue.set(r());
    }

    static void write(std::uint8_t byte) {
        while (IP::txe.read(r()) == 0u) {
        }
        r().TDR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxne.read(r()) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().RDR);
        return true;
    }

    static void flush() {
        while (IP::tc.read(r()) == 0u) {
        }
    }
};

}  // namespace alloy::hal
