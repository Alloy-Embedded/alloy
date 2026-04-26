#pragma once

// SMP-safe edge-triggered event. Use this instead of `Event` when the producer
// and consumer run on different CPU cores.
//
// `Event` uses a plain bool + atomic_signal_fence (compiler barrier only),
// which is correct for single-core ISR→task hand-off but not for cross-core
// signalling. `SharedEvent` uses `std::atomic<bool>` with acquire/release
// ordering on every access and is safe across any two cores.
//
// API mirrors `Event`: signal(), consume(), is_signaled().

#include <atomic>

namespace alloy::tasks {

class SharedEvent {
   public:
    constexpr SharedEvent() noexcept = default;

    SharedEvent(const SharedEvent&) = delete;
    auto operator=(const SharedEvent&) -> SharedEvent& = delete;
    SharedEvent(SharedEvent&&) = delete;
    auto operator=(SharedEvent&&) -> SharedEvent& = delete;

    // Signal from any core. The release ordering ensures that any shared state
    // written before signal() is visible to the consumer after consume().
    void signal() noexcept { signaled_.store(true, std::memory_order_release); }

    // Consume the signal. Safe to call from a single consumer core only.
    // Returns true if the event was set and resets it for the next cycle.
    [[nodiscard]] auto consume() noexcept -> bool {
        if (!signaled_.load(std::memory_order_acquire)) return false;
        signaled_.store(false, std::memory_order_release);
        return true;
    }

    [[nodiscard]] auto is_signaled() const noexcept -> bool {
        return signaled_.load(std::memory_order_acquire);
    }

   private:
    std::atomic<bool> signaled_{false};
};

}  // namespace alloy::tasks
