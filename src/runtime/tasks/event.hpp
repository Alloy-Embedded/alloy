#pragma once

// Single-producer, single-consumer signal event for ISR -> task wake-up.
//
// Design constraints:
//
// - Lock-free for the producer (an ISR cannot block).
// - No RMW atomic instructions, because Cortex-M0+ has no hardware
//   `ldrex/strex` and emitting `__atomic_*_4` libcalls would drag libatomic
//   into bare-metal builds for no real benefit. A single-byte flag plus a
//   compiler-only fence (`atomic_signal_fence`) is sufficient when the
//   producer (ISR) and the consumer (cooperative scheduler) live on the same
//   CPU core. For cross-core signalling on multi-core MCUs, a real atomic
//   primitive is needed -- tracked as a follow-up.
//
// - "Signal" is edge-triggered. Calling `signal()` twice between consumes
//   coalesces; awaiters cannot count signals, only observe that one happened.
//   That matches the canonical use case (a UART byte arrived; wake the
//   processor) and keeps the type's RAM footprint at one byte.
//
// Usage:
//
//     // shared between ISR and task
//     alloy::tasks::Event uart_byte_ready;
//
//     extern "C" void USART2_IRQHandler() {
//         uart_byte_ready.signal();
//     }
//
//     auto echo_task() -> Task {
//         while (true) {
//             co_await alloy::tasks::on(uart_byte_ready);
//             auto byte = uart::read_byte();
//             uart::write_byte(byte);
//         }
//     }

#include <atomic>

namespace alloy::tasks {

class Event {
   public:
    constexpr Event() noexcept = default;

    Event(const Event&) = delete;
    auto operator=(const Event&) -> Event& = delete;
    Event(Event&&) = delete;
    auto operator=(Event&&) -> Event& = delete;

    /// Signal that the event happened. ISR-safe. Idempotent: signalling twice
    /// before the consumer sees the event is the same as signalling once.
    void signal() noexcept {
        // Compiler-only fence: stops the compiler from reordering a write of
        // shared state (e.g. a UART byte buffer) past this point. The CPU
        // boundary handles the architectural ordering on single-core Cortex-M
        // because the ISR is just a function call from the CPU's view.
        std::atomic_signal_fence(std::memory_order_release);
        signaled_ = true;
    }

    /// Consume the signal. Returns true if the event was set; resets it to
    /// the unsignaled state so the same task can wait on it again later.
    /// Called only by the scheduler.
    [[nodiscard]] auto consume() noexcept -> bool {
        if (!signaled_) return false;
        signaled_ = false;
        std::atomic_signal_fence(std::memory_order_acquire);
        return true;
    }

    /// Inspect without consuming. Useful for tests.
    [[nodiscard]] auto is_signaled() const noexcept -> bool { return signaled_; }

   private:
    // Plain bool is fine because reads/writes are size-aligned single bytes
    // on every supported arch and we never RMW it. The atomic_signal_fence
    // covers compiler reordering; on multi-core targets a future change
    // promotes this to std::atomic<bool> with proper memory ordering.
    bool signaled_ = false;
};

}  // namespace alloy::tasks
