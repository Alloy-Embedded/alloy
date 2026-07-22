// Generic AT24-family I2C EEPROM driver (1-byte word address parts up to
// 256 bytes: 24C02..24C16 class, incl. the EEPROM block of AT24MAC402).
//
// Constrained ONLY on the I2cBus concept — no chip, no family, no board.
// Datasheet behavior honored (Microchip DS20005427): random read is a
// word-address write + repeated-START read (= bus.write_read, one
// transaction); page writes must not cross the page boundary (the address
// counter wraps WITHIN the page and clobbers earlier bytes), so write()
// splits at every boundary; during the internal write cycle (t_WR <= 5 ms)
// the device NACKs its own address — completion is detected by ACK-polling.
//
// AT24MAC402/602 identity block: a SECOND logical device (0x58 | A2A1A0)
// holds a factory 128-bit serial at word 0x80 and the EUI-48 at word 0x9A
// (read-only; 0x98 is EUI-64 on the MAC602 ONLY — deliberately not exposed
// here to avoid reading undefined bytes on a MAC402).

#pragma once

#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::drivers {

template <class Bus>
    requires alloy::I2cBus<Bus>
class at24 {
public:
    // 16 is both the real maximum of the 1-byte-address AT24 class (24C16)
    // and a frame every current backend can transport (the ESP32 command
    // engine caps a write at 30 data bytes and a write_read read at 15).
    static constexpr std::uint8_t kMaxPage = 16;

    constexpr at24(const Bus& bus, std::uint8_t dev_addr, std::uint8_t page_size)
        : bus_(bus), dev_addr_(dev_addr), page_(page_size) {}
    at24(const Bus&&, std::uint8_t, std::uint8_t) = delete;  // no dangling temporaries

    [[nodiscard]] bool read(std::uint8_t mem_addr, std::span<std::uint8_t> out) const {
        if (out.size() > 256u - mem_addr) {
            return false;  // would wrap past the top of the array
        }
        // Chunked random reads (each carries its own word address —
        // semantically identical per datasheet) so no backend transfer cap
        // is ever hit.
        std::size_t done = 0;
        while (done < out.size()) {
            std::size_t n = out.size() - done;
            if (n > 8) {
                n = 8;
            }
            const std::uint8_t wa[1] = {static_cast<std::uint8_t>(mem_addr + done)};
            if (!bus_.write_read(dev_addr_, wa, out.subspan(done, n))) {
                return false;
            }
            done += n;
        }
        return true;
    }

    // Any length/alignment; splits at page boundaries and ACK-polls the
    // write cycle after each page. False on NACK, timeout, out-of-range
    // span or page > buffer.
    [[nodiscard]] bool write(std::uint8_t mem_addr, std::span<const std::uint8_t> data) const {
        if (page_ == 0 || page_ > kMaxPage || data.size() > 256u - mem_addr) {
            return false;
        }
        std::size_t done = 0;
        while (done < data.size()) {
            const std::uint8_t addr = mem_addr + static_cast<std::uint8_t>(done);
            std::size_t n = page_ - (addr % page_);  // room left in this page
            if (n > data.size() - done) {
                n = data.size() - done;
            }
            std::uint8_t frame[1 + kMaxPage];
            frame[0] = addr;
            for (std::size_t i = 0; i < n; ++i) {
                frame[1 + i] = data[done + i];
            }
            if (!bus_.write(dev_addr_, std::span<const std::uint8_t>{frame, 1 + n})) {
                return false;
            }
            if (!wait_ready()) {
                return false;
            }
            done += n;
        }
        return true;
    }

    // ACK-poll until the internal write cycle ends (t_WR <= 5 ms; the bound
    // is generous because a poll round-trip is itself ~0.1 ms at 100 kHz).
    [[nodiscard]] bool wait_ready() const {
        for (unsigned i = 0; i < 2000; ++i) {
            if (bus_.write(dev_addr_, {})) {
                return true;
            }
        }
        return false;
    }

private:
    const Bus& bus_;
    std::uint8_t dev_addr_;
    std::uint8_t page_;
};

// Identity block of AT24MAC402: EUI-48 at word 0x9A, 16-byte serial at 0x80.
template <class Bus>
    requires alloy::I2cBus<Bus>
[[nodiscard]] bool at24mac_read_eui48(const Bus& bus, std::uint8_t id_dev_addr,
                                      std::span<std::uint8_t> out6) {
    const std::uint8_t wa[1] = {0x9A};  // MAC402 EUI-48 (0x98 is MAC602-only)
    return out6.size() == 6 && bus.write_read(id_dev_addr, wa, out6);
}

template <class Bus>
    requires alloy::I2cBus<Bus>
[[nodiscard]] bool at24mac_read_serial(const Bus& bus, std::uint8_t id_dev_addr,
                                       std::span<std::uint8_t> out16) {
    if (out16.size() != 16) {
        return false;
    }
    // Two 8-byte reads: stays inside every backend's bounded-transfer caps
    // (the ESP32 command engine limits a write_read read phase to 15 bytes).
    const std::uint8_t lo[1] = {0x80};
    const std::uint8_t hi[1] = {0x88};
    return bus.write_read(id_dev_addr, lo, out16.subspan(0, 8)) &&
           bus.write_read(id_dev_addr, hi, out16.subspan(8, 8));
}

}  // namespace alloy::drivers
