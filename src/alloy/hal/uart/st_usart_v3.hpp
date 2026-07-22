// UART driver for the ST usart_v3 IP (ISR/ICR era WITHOUT PRESC — F7/L4).
//
// The v4 driver's exact access pattern applies (same offsets and bit
// positions for the bring-up subset); this is a separate driver because the
// rule is one driver per IP version — v3 has no PRESC/FIFO to grow into.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/usart_v3.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v3>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t baud) {
        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());
        // OVER8=0: BRR = kernel/baud, rounded to nearest.
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
