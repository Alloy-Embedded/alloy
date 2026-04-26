#pragma once

// Opt-in integration between the cooperative scheduler and a UART RX
// interrupt. This header is header-only and pulls in nothing from the HAL
// surface; including it costs nothing for code that does not use it. The
// scheduler / Channel headers remain the foundation -- this file only adds
// a typed convenience wrapper for the canonical UART use case.
//
// What it gives you
// -----------------
//
//   - `UartRxChannel<N>` -- a small wrapper around `Channel<std::byte, N>`
//     with two ISR-side helpers and the same try_pop / wait surface the
//     cooperative scheduler expects on the consumer side.
//
// What it does NOT do
// -------------------
//
//   - It does not reach into the alloy HAL UART. Different vendors expose
//     different "byte ready" mechanisms (status register polling, hardware
//     idle-line interrupt, descriptor-driven interrupt event); we let the
//     application code own the three lines that bridge those into
//     `feed_from_isr(...)`. That keeps the helper portable across STM32G0,
//     SAME70, RP2040, ESP32 (LX6/LX7/RISC-V), and AVR (which itself cannot
//     use the scheduler -- documented limitation).
//
//   - It does not enable the UART RX interrupt. The application code does
//     that inside `board::init()` or wherever it normally wires UARTs up.
//
// Canonical wiring (STM32G0, RXNEIE)
// ----------------------------------
//
//     alloy::tasks::UartRxChannel<32> uart_rx;
//
//     extern "C" void USART2_IRQHandler() {
//         if (USART2->ISR & USART_ISR_RXNE_RXFNE) {
//             const auto byte = static_cast<std::byte>(USART2->RDR);
//             // Discard the bool return: drops are tracked internally.
//             static_cast<void>(uart_rx.feed_from_isr(byte));
//         }
//     }
//
//     auto echo() -> Task {
//         while (true) {
//             while (auto byte = uart_rx.try_pop()) {
//                 board::debug_uart::write(*byte);
//             }
//             co_await uart_rx.wait();
//         }
//     }
//
// The same pattern works on every supported MCU: the RXNE bit is the
// vendor-specific detail; everything below `feed_from_isr` is portable.

#include <cstddef>
#include <cstdint>
#include <optional>

#include "runtime/tasks/channel.hpp"

namespace alloy::tasks {

template <std::size_t Capacity>
class UartRxChannel {
   public:
    static constexpr std::size_t kCapacity = Capacity;

    /// ISR-safe push. Returns false when the ring is full (the byte is
    /// dropped and `drops()` increments). The consumer task is woken on
    /// every successful push.
    [[nodiscard]] auto feed_from_isr(std::byte byte) noexcept -> bool {
        return channel_.try_push(byte);
    }

    /// Convenience overload for callers that have a `std::uint8_t` already
    /// (e.g. straight off a peripheral register cast to integral).
    [[nodiscard]] auto feed_from_isr(std::uint8_t byte) noexcept -> bool {
        return channel_.try_push(std::byte{byte});
    }

    /// Consumer-side non-blocking pop.
    [[nodiscard]] auto try_pop() noexcept -> std::optional<std::byte> {
        return channel_.try_pop();
    }

    /// Consumer-side suspend-until-byte. The canonical loop is
    /// `while (auto b = try_pop()) { ... } co_await wait();` so a burst
    /// of bytes between two scheduler ticks is fully drained before the
    /// task suspends again.
    [[nodiscard]] auto wait() noexcept -> OnEventAwaiter { return channel_.wait(); }

    /// Drops since boot. Producer-only writes; consumer reads for
    /// diagnostics. A steadily-rising counter signals an undersized ring
    /// or a starved consumer.
    [[nodiscard]] auto drops() const noexcept -> std::size_t { return channel_.drops(); }
    [[nodiscard]] auto empty() const noexcept -> bool { return channel_.empty(); }
    [[nodiscard]] auto size() const noexcept -> std::size_t { return channel_.size(); }

   private:
    Channel<std::byte, Capacity> channel_;
};

}  // namespace alloy::tasks
