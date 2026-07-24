// alloy library testkit — scriptable bus doubles for testing a driver off-target.
//
// mock_i2c / mock_spi / mock_delay satisfy the same alloy concepts a real
// peripheral does (alloy::I2cBus, alloy::SpiBus, alloy::DelayNs), so a driver
// templated on those concepts links against them unchanged. Queue the bytes the
// device would return, run the driver, and inspect what it wrote — no hardware,
// no heap, all in a host unit test.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::testkit {

inline constexpr std::size_t kBufCap = 256;

// One captured write() transaction, for asserting an intermediate write in a
// multi-write sequence (not just the most recent one).
struct write_record {
    std::uint8_t addr = 0;
    std::uint8_t data[kBufCap]{};
    std::size_t len = 0;
};

// A scriptable I2C controller. Concept methods are const (I2cBus checks const
// T&), so the recording/replay state is mutable.
struct mock_i2c {
    // Bytes the next read()/write_read() read-phase returns, FIFO.
    mutable std::uint8_t rx[kBufCap]{};
    mutable std::size_t rx_len = 0;
    mutable std::size_t rx_pos = 0;

    // The most recent write() payload and address, for assertions.
    mutable std::uint8_t last_write[kBufCap]{};
    mutable std::size_t last_write_len = 0;
    mutable std::uint8_t last_addr = 0;
    mutable std::size_t write_count = 0;

    // Full history of writes (oldest first, capped) so a test can assert an
    // earlier write — e.g. the config byte a driver sends before its data read.
    static constexpr std::size_t kMaxWrites = 16;
    mutable write_record writes[kMaxWrites]{};

    bool fail = false;  // set true to simulate a NACK / bus error

    // Queue device response bytes (returned by later reads, in order).
    void queue_read(std::span<const std::uint8_t> data) {
        for (std::uint8_t b : data) {
            if (rx_len < kBufCap) {
                rx[rx_len++] = b;
            }
        }
    }
    void reset() {
        rx_len = rx_pos = last_write_len = write_count = 0;
    }

    bool write(std::uint8_t addr, std::span<const std::uint8_t> wr) const {
        if (fail) {
            return false;
        }
        last_addr = addr;
        last_write_len = 0;
        write_record* rec = write_count < kMaxWrites ? &writes[write_count] : nullptr;
        if (rec != nullptr) {
            rec->addr = addr;
            rec->len = 0;
        }
        for (std::uint8_t b : wr) {
            if (last_write_len < kBufCap) {
                last_write[last_write_len++] = b;
            }
            if (rec != nullptr && rec->len < kBufCap) {
                rec->data[rec->len++] = b;
            }
        }
        ++write_count;
        return true;
    }

    bool read(std::uint8_t addr, std::span<std::uint8_t> rd) const {
        if (fail) {
            return false;
        }
        last_addr = addr;
        for (std::uint8_t& b : rd) {
            b = (rx_pos < rx_len) ? rx[rx_pos++] : 0u;
        }
        return true;
    }

    bool write_read(std::uint8_t addr, std::span<const std::uint8_t> wr,
                    std::span<std::uint8_t> rd) const {
        return write(addr, wr) && read(addr, rd);
    }
};
static_assert(alloy::I2cBus<mock_i2c>);

// A scriptable SPI controller. transfer() overwrites the caller buffer with
// queued response bytes; write() records; xfer() returns one queued byte.
struct mock_spi {
    mutable std::uint8_t rx[kBufCap]{};
    mutable std::size_t rx_len = 0;
    mutable std::size_t rx_pos = 0;
    mutable std::uint8_t last_write[kBufCap]{};
    mutable std::size_t last_write_len = 0;

    void queue_read(std::span<const std::uint8_t> data) {
        for (std::uint8_t b : data) {
            if (rx_len < kBufCap) {
                rx[rx_len++] = b;
            }
        }
    }
    void reset() { rx_len = rx_pos = last_write_len = 0; }

    std::uint8_t xfer(std::uint8_t byte) const {
        if (last_write_len < kBufCap) {
            last_write[last_write_len++] = byte;
        }
        return (rx_pos < rx_len) ? rx[rx_pos++] : 0u;
    }
    void write(std::span<const std::uint8_t> cbuf) const {
        for (std::uint8_t b : cbuf) {
            (void)xfer(b);
        }
    }
    void transfer(std::span<std::uint8_t> buf) const {
        for (std::uint8_t& b : buf) {
            b = xfer(b);
        }
    }
};
static_assert(alloy::SpiBus<mock_spi>);

// A no-op delay that counts requested nanoseconds so a test can assert timing.
struct mock_delay {
    mutable std::uint64_t total_ns = 0;
    void delay_ns(std::uint32_t ns) { total_ns += ns; }
};
static_assert(alloy::DelayNs<mock_delay>);

// A pin double (mirrors tests/doubles.hpp fake_pin) for drivers needing a CS/DC.
struct mock_pin {
    mutable bool level = false;
    mutable unsigned toggles = 0;
    void set_high() const { level = true; }
    void set_low() const { level = false; }
    void toggle() const {
        level = !level;
        ++toggles;
    }
};
static_assert(alloy::OutputPin<mock_pin>);

}  // namespace alloy::testkit
