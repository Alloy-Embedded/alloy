// alloy library testkit — serial-wire doubles for protocol libraries.
//
// mock_serial satisfies alloy::ByteStream exactly as a real uart handle does,
// so a protocol client templated on that concept links against it unchanged:
// queue the bytes the peer would send, run the client, inspect what it wrote.
// virtual_clock stands in for alloy::uptime_us() — tests advance time by the
// microsecond to walk a framer through t3.5 windows deterministically.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::testkit {

inline constexpr std::size_t kWireCap = 512;  // two full RTU ADUs

// A scriptable serial port. read() yields queued peer bytes one at a time
// (false when drained — the same "nothing waiting" contract the uart handle
// has); write() captures everything the code under test transmits.
struct mock_serial {
    std::uint8_t rx[kWireCap]{};
    std::size_t rx_len = 0;
    std::size_t rx_pos = 0;

    std::uint8_t tx[kWireCap]{};
    std::size_t tx_len = 0;

    void queue_rx(std::span<const std::uint8_t> data) {
        for (std::uint8_t b : data) {
            if (rx_len < kWireCap) {
                rx[rx_len++] = b;
            }
        }
    }
    void reset() { rx_len = rx_pos = tx_len = 0; }

    void write(std::uint8_t b) {
        if (tx_len < kWireCap) {
            tx[tx_len++] = b;
        }
    }
    void write(const char* zstr) {
        while (*zstr != '\0') {
            write(static_cast<std::uint8_t>(*zstr++));
        }
    }
    [[nodiscard]] bool read(std::uint8_t& b) {
        if (rx_pos >= rx_len) {
            return false;
        }
        b = rx[rx_pos++];
        return true;
    }
};
static_assert(alloy::ByteStream<mock_serial>);

// A hand-cranked microsecond clock. Deliberately allowed to wrap: framer
// tests drive it across the 2^32 µs boundary to pin wrap-safe delta math.
struct virtual_clock {
    std::uint32_t now_us = 0;
    void advance_us(std::uint32_t d) { now_us += d; }
};

}  // namespace alloy::testkit
