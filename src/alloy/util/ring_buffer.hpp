// Fixed-capacity ring/FIFO — the most-reached-for embedded primitive (UART RX
// staging, event queues, sample capture). std::array-backed, zero heap,
// capacity fixed at compile time.
//
// Genuinely lock-free single-producer / single-consumer: push() only ever
// writes head_ (and its own slot), pop() only ever writes tail_. Each side
// reads the OTHER'S index as a single aligned word — atomic on the 32-bit
// cores alloy targets — so an ISR producer and a foreground consumer need no
// lock. (Emptiness/fullness derive from head_ vs tail_, never a shared counter
// that both sides would have to read-modify-write.) One slot is reserved to
// tell "full" from "empty", so the array holds Capacity + 1 elements.
//
// clear() resets both indices and is NOT concurrency-safe — call it only when
// the ring is quiescent (before wiring up the ISR).

#pragma once

#include <array>
#include <cstddef>

namespace alloy {

template <class T, std::size_t Capacity>
class ring_buffer {
    static_assert(Capacity >= 1, "ring_buffer needs capacity >= 1");
    static constexpr std::size_t slots = Capacity + 1;  // one reserved slot
    std::array<T, slots> buf_{};
    std::size_t head_{0};  // producer-owned: next slot to write
    std::size_t tail_{0};  // consumer-owned: next slot to read

public:
    using value_type = T;

    [[nodiscard]] constexpr std::size_t capacity() const { return Capacity; }
    [[nodiscard]] std::size_t size() const { return (head_ + slots - tail_) % slots; }
    [[nodiscard]] bool empty() const { return head_ == tail_; }
    [[nodiscard]] bool full() const { return advance(head_) == tail_; }

    // Not concurrency-safe: only call when no producer/consumer is active.
    void clear() {
        head_ = 0;
        tail_ = 0;
    }

    // Enqueue (producer side); returns false (value dropped) when full.
    bool push(const T& v) {
        const std::size_t next = advance(head_);
        if (next == tail_) {
            return false;
        }
        buf_[head_] = v;
        head_ = next;
        return true;
    }

    // Dequeue into `out` (consumer side); returns false when empty.
    bool pop(T& out) {
        if (head_ == tail_) {
            return false;
        }
        out = buf_[tail_];
        tail_ = advance(tail_);
        return true;
    }

    // Oldest element without removing it; undefined when empty.
    [[nodiscard]] const T& front() const { return buf_[tail_]; }

private:
    [[nodiscard]] static std::size_t advance(std::size_t i) {
        return (i + 1 == slots) ? std::size_t{0} : i + 1;
    }
};

}  // namespace alloy
