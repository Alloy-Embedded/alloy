#pragma once

// Single-producer, single-consumer ring buffer with full acquire/release
// memory ordering on all index loads and stores.
//
// Unlike `Channel<T,N>` (which uses compiler-only signal fences for single-core
// ISR↔task hand-off), `CrossCoreChannel<T,N>` is safe across two CPU cores.
// Use it whenever producer and consumer run on different cores.
//
// Constraints mirror Channel:
//   - Capacity must be a power of two.
//   - T must be trivially copyable.
//   - Single producer, single consumer — one core owns each cursor.
//
// Drop counter semantics: increments on full-ring try_push (value discarded).

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace alloy::tasks {

template <typename T, std::size_t Capacity>
class CrossCoreChannel {
    static_assert(Capacity > 0, "Capacity must be > 0");
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

    static constexpr std::size_t kMask = Capacity - 1u;

   public:
    using value_type = T;
    static constexpr std::size_t kCapacity = Capacity;

    CrossCoreChannel() = default;
    CrossCoreChannel(const CrossCoreChannel&) = delete;
    auto operator=(const CrossCoreChannel&) -> CrossCoreChannel& = delete;
    CrossCoreChannel(CrossCoreChannel&&) = delete;
    auto operator=(CrossCoreChannel&&) -> CrossCoreChannel& = delete;

    // Producer side. Returns false and increments drop counter when the ring is
    // full. The head_ store uses memory_order_release so the payload write is
    // visible to the consumer before the index advance is.
    [[nodiscard]] auto try_push(T value) noexcept -> bool {
        const auto h = head_.load(std::memory_order_relaxed);
        const auto next_head = (h + 1u) & kMask;
        if (next_head == tail_.load(std::memory_order_acquire)) {
            ++drops_;
            return false;
        }
        storage_[h] = value;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // Consumer side. Returns nullopt when the ring is empty.
    // The head_ load uses memory_order_acquire to synchronise with the producer's
    // release store, guaranteeing the payload is fully visible before we read it.
    [[nodiscard]] auto try_pop() noexcept -> std::optional<T> {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_relaxed);
        if (h == t) return std::nullopt;
        const T value = storage_[t];
        tail_.store((t + 1u) & kMask, std::memory_order_release);
        return value;
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_relaxed);
        return (h - t) & kMask;
    }

    [[nodiscard]] auto drops() const noexcept -> std::size_t { return drops_; }

   private:
    std::array<T, Capacity> storage_{};

    // Separate cache lines prevent false sharing between producer and consumer.
    alignas(64) std::atomic<std::size_t> head_{0};
    std::size_t drops_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

}  // namespace alloy::tasks
