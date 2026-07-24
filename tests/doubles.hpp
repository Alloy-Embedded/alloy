// Hardware test-doubles — plain structs that satisfy the same alloy concepts
// the silicon drivers do, so generic code (logger, scheduler, filters, a user
// service) runs unchanged against them on the host. This is what makes app
// logic testable off-target: swap the real peripheral for a fake that records
// what happened.

#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/concepts.hpp"

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

}  // namespace alloy::test
