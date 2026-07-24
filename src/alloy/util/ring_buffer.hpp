// Fixed-capacity ring/FIFO — the most-reached-for embedded primitive (UART RX
// staging, event queues, sample capture). std::array-backed, zero heap,
// capacity fixed at compile time.
//
// Genuinely lock-free single-producer / single-consumer: push() only ever
// writes head_ (and its own slot), pop() only ever writes tail_. Each side
// owns one index and only reads the other's, so no shared read-modify-write
// exists to race. Correctness across an ISR/foreground boundary needs the two
// index writes to be *ordered* against the slot access, not merely word-atomic:
//   - push() writes the slot, then publishes head_ with release;
//   - pop() reads head_ with acquire before touching the slot it points to.
// The release→acquire pair is what makes the consumer observe a fully-written
// slot, and it stops the compiler from reordering the slot store past the index
// publish or hoisting an index load out of a poll loop (`while (rb.empty()) {}`)
// — the miscompilations plain std::size_t indices would permit at -O2/LTO. On
// the single-core 32-bit targets alloy ships, these lower to ordinary loads and
// stores (no DMB needed for ordinary memory), so the guarantee is ~free.
//
// clear() resets both indices and is NOT concurrency-safe — call it only when
// the ring is quiescent (before wiring up the ISR).

#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace alloy {

template <class T, std::size_t Capacity>
class ring_buffer {
    static_assert(Capacity >= 1, "ring_buffer needs capacity >= 1");
    // Deliberately no is_always_lock_free assert: that trait covers the whole
    // type including read-modify-write ops, which are NOT lock-free on ARMv6-M
    // (Cortex-M0+, e.g. STM32G0 — no LDREX/STREX). This ring uses ONLY load()
    // and store(), which lower to a single word LDR/STR (+DMB) on every 32-bit
    // target alloy ships — lock-free with no libatomic dependency.
    static constexpr std::size_t slots = Capacity + 1;  // one reserved slot
    std::array<T, slots> buf_{};
    std::atomic<std::size_t> head_{0};  // producer-owned: next slot to write
    std::atomic<std::size_t> tail_{0};  // consumer-owned: next slot to read

public:
    using value_type = T;

    [[nodiscard]] constexpr std::size_t capacity() const { return Capacity; }
    [[nodiscard]] std::size_t size() const {
        const std::size_t h = head_.load(std::memory_order_acquire);
        const std::size_t t = tail_.load(std::memory_order_acquire);
        return (h + slots - t) % slots;
    }
    [[nodiscard]] bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool full() const {
        return advance(head_.load(std::memory_order_acquire)) ==
               tail_.load(std::memory_order_acquire);
    }

    // Not concurrency-safe: only call when no producer/consumer is active.
    void clear() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    // Enqueue (producer side); returns false (value dropped) when full.
    bool push(const T& v) {
        const std::size_t head = head_.load(std::memory_order_relaxed);  // we own it
        const std::size_t next = advance(head);
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;  // full — consumer has not freed the reserved slot
        }
        buf_[head] = v;
        head_.store(next, std::memory_order_release);  // publishes the slot write
        return true;
    }

    // Dequeue into `out` (consumer side); returns false when empty.
    bool pop(T& out) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);  // we own it
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;  // empty
        }
        out = buf_[tail];
        tail_.store(advance(tail), std::memory_order_release);  // frees the slot
        return true;
    }

    // Oldest element without removing it; undefined when empty.
    [[nodiscard]] const T& front() const {
        return buf_[tail_.load(std::memory_order_acquire)];
    }

private:
    [[nodiscard]] static std::size_t advance(std::size_t i) {
        return (i + 1 == slots) ? std::size_t{0} : i + 1;
    }
};

}  // namespace alloy
