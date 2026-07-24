// Fixed-capacity ring/FIFO — the most-reached-for embedded primitive (UART RX
// staging, event queues, sample capture). std::array-backed, zero heap,
// capacity fixed at compile time. Single-producer / single-consumer: push from
// one context (e.g. an ISR) and pop from another needs no lock on the 32-bit
// cores alloy targets, where an aligned index word writes atomically.

#pragma once

#include <array>
#include <cstddef>

namespace alloy {

template <class T, std::size_t Capacity>
class ring_buffer {
    static_assert(Capacity >= 1, "ring_buffer needs capacity >= 1");
    std::array<T, Capacity> buf_{};
    std::size_t head_{0};  // next slot to write
    std::size_t tail_{0};  // next slot to read
    std::size_t count_{0};

public:
    using value_type = T;

    [[nodiscard]] constexpr std::size_t capacity() const { return Capacity; }
    [[nodiscard]] std::size_t size() const { return count_; }
    [[nodiscard]] bool empty() const { return count_ == 0; }
    [[nodiscard]] bool full() const { return count_ == Capacity; }

    void clear() {
        head_ = 0;
        tail_ = 0;
        count_ = 0;
    }

    // Enqueue; returns false (value dropped) when full.
    bool push(const T& v) {
        if (count_ == Capacity) {
            return false;
        }
        buf_[head_] = v;
        head_ = advance(head_);
        ++count_;
        return true;
    }

    // Dequeue into `out`; returns false when empty.
    bool pop(T& out) {
        if (count_ == 0) {
            return false;
        }
        out = buf_[tail_];
        tail_ = advance(tail_);
        --count_;
        return true;
    }

    // Oldest element without removing it; undefined when empty.
    [[nodiscard]] const T& front() const { return buf_[tail_]; }

private:
    [[nodiscard]] static std::size_t advance(std::size_t i) {
        return (i + 1 == Capacity) ? std::size_t{0} : i + 1;
    }
};

}  // namespace alloy
