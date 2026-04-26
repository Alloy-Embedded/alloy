#pragma once

// Opt-in TX-side counterpart to `UartRxChannel`. Same single-producer /
// single-consumer pattern, mirrored:
//
//   Task   -> try_send(byte)              (producer; the application loop)
//   ISR    -> pop_for_isr()               (consumer; UART TX-ready handler)
//   Task   <- co_await wait_space() / wait_drained()
//
// Why a separate type instead of reusing `Channel`
// -----------------------------------------------
//
// `Channel<T, N>` signals its internal event on every push (producer side)
// because that is the right wake-up edge for RX (ISR -> task). On TX the
// edge points the other way: the consumer (the ISR) signals when it makes
// room in the queue, and the producer (the task) waits on that. Mirroring
// the directionality cleanly is easier than overloading Channel's signal
// semantics, so the TX type owns its own ring and event.
//
// The "kick" callback
// -------------------
//
// On most MCUs the UART's TXE/TC interrupt fires only when the data
// register is empty AND the interrupt is enabled. When the application
// pushes the first byte into an empty queue, the ISR is typically off; if
// nothing wakes it, it never fires and the bytes never go out. The
// canonical fix is "enable the TX-empty interrupt the moment the queue
// becomes non-empty"; UartTxChannel calls a user-supplied `kick` callback
// for exactly that. The callback is invoked from the task context (NOT
// from inside any ISR), so it can use whatever the application uses to
// enable interrupts.
//
// Set the callback once during init:
//
//     alloy::tasks::UartTxChannel<32> uart_tx;
//
//     void board_init() {
//         uart_tx.set_kick_callback([] {
//             USART2->CR1 |= USART_CR1_TXEIE;   // enable TXE interrupt
//         });
//     }
//
//     extern "C" void USART2_IRQHandler() {
//         if (USART2->ISR & USART_ISR_TXE_TXFNF) {
//             if (auto byte = uart_tx.pop_for_isr()) {
//                 USART2->TDR = static_cast<uint32_t>(*byte);
//             } else {
//                 // queue drained; ISR turns itself off until the next kick
//                 USART2->CR1 &= ~USART_CR1_TXEIE;
//             }
//         }
//     }

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "runtime/tasks/event.hpp"
#include "runtime/tasks/scheduler.hpp"  // OnEventAwaiter

namespace alloy::tasks {

template <std::size_t Capacity>
class UartTxChannel {
    static_assert(Capacity > 0, "Capacity must be > 0");
    static_assert(std::has_single_bit(Capacity),
                  "Capacity must be a power of two so the cursor wrap is a bitwise AND");

    static constexpr std::size_t kMask = Capacity - 1u;

   public:
    using KickFn = void (*)();
    static constexpr std::size_t kCapacity = Capacity;

    UartTxChannel() = default;

    UartTxChannel(const UartTxChannel&) = delete;
    auto operator=(const UartTxChannel&) -> UartTxChannel& = delete;
    UartTxChannel(UartTxChannel&&) = delete;
    auto operator=(UartTxChannel&&) -> UartTxChannel& = delete;

    /// Install the "kick" callback the channel calls when a try_send
    /// transitions the queue from empty to non-empty. Typical body:
    /// "enable the UART TXE interrupt".
    void set_kick_callback(KickFn fn) noexcept { kick_ = fn; }

    /// Task-side push. Returns false if the ring is full (the byte is
    /// dropped and the drop counter increments). On the empty -> non-empty
    /// transition the kick callback is invoked from the task context so
    /// the application can re-enable the TX-empty interrupt.
    [[nodiscard]] auto try_send(std::byte byte) noexcept -> bool {
        const auto h = head_;
        const auto next_head = (h + 1u) & kMask;
        if (next_head == tail_) {
            ++drops_;
            return false;
        }
        const bool was_empty = (h == tail_);
        storage_[h] = byte;
        std::atomic_signal_fence(std::memory_order_release);
        head_ = next_head;
        if (was_empty && kick_ != nullptr) kick_();
        return true;
    }

    /// Convenience overload for callers that have an integer byte in hand.
    [[nodiscard]] auto try_send(std::uint8_t byte) noexcept -> bool {
        return try_send(std::byte{byte});
    }

    /// ISR-side pop. Returns nullopt when the queue is drained. After a
    /// successful pop the popped event is signalled so any task awaiting
    /// `wait_space` or `wait_drained` resumes on the next scheduler tick.
    [[nodiscard]] auto pop_for_isr() noexcept -> std::optional<std::byte> {
        if (head_ == tail_) return std::nullopt;
        std::atomic_signal_fence(std::memory_order_acquire);
        const auto value = storage_[tail_];
        tail_ = (tail_ + 1u) & kMask;
        popped_.signal();
        return value;
    }

    /// Suspend until at least one ISR pop happens. Use in the canonical
    /// back-pressure loop:
    ///
    ///     while (!tx.try_send(byte)) co_await tx.wait_space();
    [[nodiscard]] auto wait_space() noexcept -> OnEventAwaiter {
        return OnEventAwaiter{popped_};
    }

    /// Suspend until the next ISR pop. Use in a drain-to-empty loop:
    ///
    ///     while (!tx.empty()) co_await tx.wait_drained();
    ///
    /// Implemented on the same `popped_` event because both waits are
    /// satisfied by an ISR pop; the loop predicate disambiguates.
    [[nodiscard]] auto wait_drained() noexcept -> OnEventAwaiter {
        return OnEventAwaiter{popped_};
    }

    [[nodiscard]] auto drops() const noexcept -> std::size_t { return drops_; }
    [[nodiscard]] auto empty() const noexcept -> bool { return head_ == tail_; }
    [[nodiscard]] auto full() const noexcept -> bool {
        return ((head_ + 1u) & kMask) == tail_;
    }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return (head_ - tail_) & kMask;
    }

   private:
    std::array<std::byte, Capacity> storage_{};
    // Task-only writes
    std::size_t head_ = 0;
    std::size_t drops_ = 0;
    KickFn kick_ = nullptr;
    // ISR-only writes
    std::size_t tail_ = 0;
    Event popped_{};
};

}  // namespace alloy::tasks
