// alloy::lib::bus::watch<T> — a latest-value cell, for the half of
// "telemetry" that does not want a queue: current temperature, current mode,
// last reading. A depth-1 subscriber with drop-newest would hand back the
// OLDEST unread value — exactly wrong for that case. watch overwrites:
// get() always sees the newest set(), and updates() counts every write.
//
// The cell is independent of topics (usable as a plain ISR→thread mailbox).
// watch_route<T> optionally links a cell into topic<T>, so publishes feed it
// alongside queued subscribers.
//
// Reads and writes copy T under the irq mask — same trade as publish: keep T
// small, this is the slow plane. No awaitable in this phase: get() is poll;
// code that wants "wake me on change" uses subscriber<T, 1> and accepts
// queue semantics.

#pragma once

#include <cstdint>
#include <type_traits>

#include "alloy/arch/irq.hpp"
#include "bus/topic.hpp"

namespace alloy::lib::bus {

template <class T>
class watch {
    static_assert(std::is_trivially_copyable_v<T>,
                  "watch copies T under an irq mask");

public:
    watch() = default;
    watch(const watch&) = delete;
    watch& operator=(const watch&) = delete;

    // Overwrite the cell. ISR-safe.
    void set(const T& v) noexcept {
        const arch::irq_state s = arch::irq_save();
        value_ = v;
        written_ = true;
        ++updates_;
        arch::irq_restore(s);
    }

    // Copy out the newest value. False until the first set() — a default-
    // constructed T pretending to be a reading would be a lie, not an API.
    [[nodiscard]] bool get(T& out) const noexcept {
        const arch::irq_state s = arch::irq_save();
        const bool w = written_;
        if (w) {
            out = value_;
        }
        arch::irq_restore(s);
        return w;
    }

    // Cumulative write count (advisory read, same caveat as missed()).
    [[nodiscard]] std::uint32_t updates() const noexcept { return updates_; }

private:
    T value_{};
    bool written_ = false;
    std::uint32_t updates_ = 0;
};

// Link a watch cell into topic<T>: every publish(T) also lands in the cell.
// Same lifetime obligation as subscriber: the route unlinks in its
// destructor, and the watch it references must outlive it.
template <class T>
class watch_route {
public:
    explicit watch_route(watch<T>& w) noexcept : w_(w) {
        node_.deliver = &deliver_thunk;
        node_.owner = this;
        detail::topic<T>::link(node_);
    }
    ~watch_route() { detail::topic<T>::unlink(node_); }
    watch_route(const watch_route&) = delete;
    watch_route& operator=(const watch_route&) = delete;
    watch_route(watch<T>&&) = delete;  // a temporary cell would dangle

private:
    static bool deliver_thunk(void* owner, const void* msg) noexcept {
        auto& self = *static_cast<watch_route*>(owner);
        self.w_.set(*static_cast<const T*>(msg));  // nested mask — fine
        return true;  // a cell overwrites; it cannot drop
    }

    detail::node node_{};
    watch<T>& w_;
};

}  // namespace alloy::lib::bus
