// Hardware test-doubles — plain structs that satisfy the same alloy concepts
// the silicon drivers do, so generic code (logger, scheduler, filters, a user
// service) runs unchanged against them on the host. This is what makes app
// logic testable off-target: swap the real peripheral for a fake that records
// what happened.

#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/concepts.hpp"
#include "alloy/hal/can/can_impl.hpp"

namespace alloy::test {

// Satisfies OutputPin — records level and toggle count. State is `mutable`
// because OutputPin (like the real stateless gpio::output) drives through const
// methods; a recording double keeps its bookkeeping mutable to match.
struct fake_pin {
    mutable bool level{false};
    mutable std::uint32_t toggles{0};
    void set_high() const { level = true; }
    void set_low() const { level = false; }
    void toggle() const {
        level = !level;
        ++toggles;
    }
    void on() const { set_high(); }
    void off() const { set_low(); }
};
static_assert(alloy::OutputPin<fake_pin>);

// Satisfies ByteStream (ByteSink + ByteSource): captures everything written,
// replays a preloaded rx buffer.
template <std::size_t Cap = 512>
struct fake_uart {
    std::uint8_t tx[Cap]{};
    std::size_t tx_len{0};
    const std::uint8_t* rx{nullptr};
    std::size_t rx_len{0};
    std::size_t rx_pos{0};

    void write(std::uint8_t b) {
        if (tx_len < Cap) {
            tx[tx_len++] = b;
        }
    }
    void write(const char* z) {
        while (*z != '\0') {
            write(static_cast<std::uint8_t>(*z++));
        }
    }
    bool read(std::uint8_t& b) {
        if (rx_pos < rx_len) {
            b = rx[rx_pos++];
            return true;
        }
        return false;
    }

    void clear() {
        tx_len = 0;
        rx_pos = 0;
    }

    // Does the captured tx stream contain `needle`?
    [[nodiscard]] bool tx_contains(const char* needle) const {
        for (std::size_t i = 0; i < tx_len; ++i) {
            std::size_t j = 0;
            while (needle[j] != '\0' && i + j < tx_len &&
                   tx[i + j] == static_cast<std::uint8_t>(needle[j])) {
                ++j;
            }
            if (needle[j] == '\0') {
                return true;
            }
        }
        return false;
    }
};
static_assert(alloy::ByteSink<fake_uart<>>);
static_assert(alloy::ByteSource<fake_uart<>>);

// A settable monotonic clock for the software timer / scheduler.
struct fake_clock {
    std::uint32_t now{0};
    [[nodiscard]] std::uint32_t operator()() const { return now; }
    void advance(std::uint32_t d) { now += d; }
};

// Satisfies CanBus, in software: a one-deep loopback with the SAME acceptance
// filtering the silicon does. Its point is not to fake a controller — it is to
// show that `alloy::can`'s filter surface is implementable by something that
// is not an M_CAN, which is the only evidence available on the host that the
// surface is portable rather than a description of one vendor's registers.
//
// It is also the executable statement of what a filter MEANS: `matches()` is
// the same predicate the FDCAN's classic filter element evaluates in hardware,
// so a host test can pin the semantics the example depends on.
template <std::size_t Slots = 8>
struct fake_can {
    mutable alloy::hal::can_filter filters[Slots]{};
    mutable std::size_t filter_count{0};   // 0 with accept_unmatched = accept all
    mutable bool accept_unmatched{true};
    mutable alloy::can_frame pending{};
    mutable bool has_pending{false};
    mutable std::uint32_t dropped{0};

    void enable() const {
        filter_count = 0;
        accept_unmatched = true;
        has_pending = false;
        dropped = 0;
    }

    [[nodiscard]] bool accepts(std::uint32_t id) const {
        for (std::size_t i = 0; i < filter_count; ++i) {
            if (filters[i].matches(id)) {
                return true;
            }
        }
        return accept_unmatched;
    }

    [[nodiscard]] bool send(const alloy::can_frame& f) const {
        if (!accepts(f.id)) {
            ++dropped;  // filtered on the way back in, exactly as loopback does
            return true;
        }
        pending = f;
        has_pending = true;
        return true;
    }
    [[nodiscard]] bool receive(alloy::can_frame& f) const {
        if (!has_pending) {
            return false;
        }
        f = pending;
        has_pending = false;
        return true;
    }

    void accept_all() const {
        filter_count = 0;
        accept_unmatched = true;
    }
    template <class... F>
    void accept_only(F... fs) const {
        static_assert(sizeof...(F) <= Slots, "fake_can has fewer slots than that");
        const alloy::hal::can_filter list[] = {fs...};
        filter_count = sizeof...(F);
        for (std::size_t i = 0; i < filter_count; ++i) {
            filters[i] = list[i];
        }
        accept_unmatched = false;
    }
};
static_assert(alloy::CanBus<fake_can<>>);

}  // namespace alloy::test
