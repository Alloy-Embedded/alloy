#pragma once

// Single-producer, single-consumer ring buffer with payload, designed for the
// canonical embedded ISR -> task hand-off:
//
//     ISR side: enqueue a freshly-arrived value (UART byte, ADC sample, etc.)
//     and signal so a sleeping task wakes.
//
//     Task side: drain everything that's queued, then suspend until the next
//     signal.
//
// Design notes:
//
// - Lock-free for the producer (an ISR cannot block).
// - No RMW atomic instructions. Cortex-M0+ has no hardware ldrex/strex; we
//   refuse to drag libatomic in for what is a single-core ISR-vs-main-loop
//   boundary. Each side writes only its own cursor (Lamport ring); a
//   compiler-only `atomic_signal_fence` enforces the ordering between the
//   payload write and the cursor advance. Cross-core signalling needs a real
//   atomic primitive -- tracked as a follow-up.
//
// - Capacity must be a power of two so the modulo on each push/pop turns into
//   a bitwise AND. The static_assert keeps misuse out at compile time.
//
// - `T` must be trivially copyable. The canonical payloads (`std::byte`, scalar
//   samples, small POD structs) all satisfy this. Non-trivial types would
//   require copy/move calls inside an ISR -- we forbid that path on purpose.
//
// - The drop counter is producer-only writes / consumer reads. It increments
//   when `try_push` finds the ring full and discards the value. Production
//   code should sample it periodically; a steadily-rising drop counter means
//   either the consumer task is starved or the ring is undersized.
//
// Canonical consumer loop:
//
//     while (true) {
//         while (auto value = rx_channel.try_pop()) {
//             process(*value);
//         }
//         co_await rx_channel.wait();
//     }

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

#include "runtime/tasks/event.hpp"
#include "runtime/tasks/scheduler.hpp"  // OnEventAwaiter

namespace alloy::tasks {

template <typename T, std::size_t Capacity>
class Channel {
    static_assert(Capacity > 0, "Channel capacity must be > 0");
    static_assert(std::has_single_bit(Capacity),
                  "Channel capacity must be a power of two so the cursor wrap is a bitwise AND");
    static_assert(std::is_trivially_copyable_v<T>,
                  "Channel T must be trivially copyable so ISR-side push does no work");

    static constexpr std::size_t kMask = Capacity - 1u;

   public:
    using value_type = T;
    static constexpr std::size_t kCapacity = Capacity;

    Channel() = default;

    Channel(const Channel&) = delete;
    auto operator=(const Channel&) -> Channel& = delete;
    Channel(Channel&&) = delete;
    auto operator=(Channel&&) -> Channel& = delete;

    /// ISR-safe push. Returns true if the value was queued, false if the ring
    /// was full (the value is dropped and the drop counter increments). The
    /// signal event is fired on every successful push so a sleeping consumer
    /// task wakes on the next scheduler tick.
    [[nodiscard]] auto try_push(T value) noexcept -> bool {
        const auto h = head_;
        const auto next_head = (h + 1u) & kMask;
        if (next_head == tail_) {
            // Full. Don't lose count of how often that happens.
            ++drops_;
            return false;
        }
        storage_[h] = value;
        // Ensure the element write is visible before the head advance.
        // Single-core: compiler barrier is sufficient (ISR boundary, no OoO).
        // SMP: full memory fence to order across cores.
#if ALLOY_SINGLE_CORE
        std::atomic_signal_fence(std::memory_order_release);
#else
        std::atomic_thread_fence(std::memory_order_release);
#endif
        head_ = next_head;
        ready_.signal();
        return true;
    }

    /// Consumer-side non-blocking pop. Returns nullopt when the ring is empty.
    [[nodiscard]] auto try_pop() noexcept -> std::optional<T> {
        if (head_ == tail_) return std::nullopt;
#if ALLOY_SINGLE_CORE
        std::atomic_signal_fence(std::memory_order_acquire);
#else
        std::atomic_thread_fence(std::memory_order_acquire);
#endif
        const auto value = storage_[tail_];
        tail_ = (tail_ + 1u) & kMask;
        return value;
    }

    /// Suspend until the producer signals at least once. The canonical pattern
    /// is `while (auto v = try_pop()) { ... } co_await wait();` -- drain
    /// everything ready, then suspend for the next signal. Spurious wake-ups
    /// do not occur in the single-producer / single-consumer model when the
    /// consumer always drains before waiting.
    [[nodiscard]] auto wait() noexcept -> OnEventAwaiter { return OnEventAwaiter{ready_}; }

    /// Producer-only writes; consumer reads for diagnostics.
    [[nodiscard]] auto drops() const noexcept -> std::size_t { return drops_; }

    /// Test/diagnostic helpers.
    [[nodiscard]] auto empty() const noexcept -> bool { return head_ == tail_; }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return (head_ - tail_) & kMask;
    }

   private:
    std::array<T, Capacity> storage_{};
    // Producer-only writes
    std::size_t head_ = 0;
    std::size_t drops_ = 0;
    // Consumer-only writes
    std::size_t tail_ = 0;
    // Signalled by the producer after a push; consumed by the awaiter the
    // consumer task suspends on.
    Event ready_{};
};

}  // namespace alloy::tasks
