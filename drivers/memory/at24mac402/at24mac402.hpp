#pragma once

// drivers/memory/at24mac402/at24mac402.hpp
//
// Driver for Microchip AT24MAC402 I2C EEPROM with pre-programmed EUI-48 and
// 128-bit serial number.
// Written against datasheet revision DS20005427A (Microchip, 2015).
// Seed driver: paged write + arbitrary read of the 2 Kbit (256-byte) EEPROM
// block, plus read accessors for the factory EUI-48 and 128-bit serial number
// held in the "protected" sub-address block. See drivers/README.md.
//
// The part responds on two logical I2C addresses:
//   * EEPROM block: base 0x50 | (A2 A1 A0)
//   * Protected block (EUI + serial): base 0x58 | (A2 A1 A0)
// The caller supplies both so the driver does not assume a board wiring.
// Write operations respect the 16-byte page boundary of the AT24MAC402 — each
// page-crossing write is split into per-page transactions internally.

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::memory::at24mac402 {

inline constexpr std::uint16_t kEepromBaseAddress = 0x50;
inline constexpr std::uint16_t kProtectedBaseAddress = 0x58;
inline constexpr std::uint16_t kEepromSizeBytes = 256;   // 2 Kbit
inline constexpr std::uint16_t kPageSizeBytes = 16;
inline constexpr std::uint8_t kEui48InternalAddress = 0x9A;  // §4.3 serial layout
inline constexpr std::uint8_t kEui64InternalAddress = 0x98;
inline constexpr std::uint8_t kSerialNumberInternalAddress = 0x80;
inline constexpr std::uint8_t kSerialNumberLengthBytes = 16;

struct Config {
    std::uint16_t eeprom_address = kEepromBaseAddress;
    std::uint16_t protected_address = kProtectedBaseAddress;
};

template <typename BusHandle>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    // Probes both sub-addresses with a zero-length read (implemented as a
    // one-byte read at offset 0x00) so the caller learns up-front whether both
    // logical devices respond.
    [[nodiscard]] auto init() -> ResultVoid {
        std::array<std::uint8_t, 1> probe{};
        if (auto r = read_at(cfg_.eeprom_address, 0x00, probe); r.is_err()) {
            return r;
        }
        return read_at(cfg_.protected_address, kEui48InternalAddress, probe);
    }

    // Reads `out.size()` bytes starting at EEPROM byte offset `offset`.
    // Out-of-range reads return InvalidParameter. Reads that span to the end
    // of the array are permitted.
    [[nodiscard]] auto read(std::uint8_t offset, std::span<std::uint8_t> out) -> ResultVoid {
        if (out.empty()) {
            return alloy::core::Ok();
        }
        if (static_cast<std::size_t>(offset) + out.size() > kEepromSizeBytes) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        return read_at(cfg_.eeprom_address, offset, out);
    }

    // Writes `data.size()` bytes starting at EEPROM byte offset `offset`,
    // splitting at every 16-byte page boundary so the part's internal page
    // latch never wraps. Out-of-range writes return InvalidParameter. The
    // caller is responsible for the write-cycle delay between successive
    // `write()` calls (datasheet t_WR ~ 5 ms).
    [[nodiscard]] auto write(std::uint8_t offset, std::span<const std::uint8_t> data)
        -> ResultVoid {
        if (data.empty()) {
            return alloy::core::Ok();
        }
        if (static_cast<std::size_t>(offset) + data.size() > kEepromSizeBytes) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        std::size_t written = 0;
        std::uint8_t cursor = offset;
        while (written < data.size()) {
            const std::uint8_t page_remaining =
                static_cast<std::uint8_t>(kPageSizeBytes - (cursor % kPageSizeBytes));
            const std::size_t chunk =
                std::min<std::size_t>(page_remaining, data.size() - written);

            std::array<std::uint8_t, 1 + kPageSizeBytes> frame{};
            frame[0] = cursor;
            std::memcpy(frame.data() + 1, data.data() + written, chunk);

            // ACK polling: during the internal t_WR (~5 ms) the part NACKs
            // every address byte. Retry the page write until ACK returns or
            // the poll budget is exhausted. Each failed attempt is a
            // START + addr + NACK which takes ~100 us at 100 kHz, so
            // kAckPollRetries * 100 us covers the datasheet worst case.
            constexpr std::size_t kAckPollRetries = 100;  // ~10 ms budget
            std::size_t retries = 0;
            while (true) {
                auto r = bus_->write(cfg_.eeprom_address,
                                     std::span<const std::uint8_t>{frame.data(), chunk + 1});
                if (r.is_ok()) {
                    break;
                }
                if (r.error() != alloy::core::ErrorCode::I2cNack ||
                    ++retries >= kAckPollRetries) {
                    return r;
                }
            }
            written += chunk;
            cursor = static_cast<std::uint8_t>(cursor + chunk);
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto read_eui48(std::array<std::uint8_t, 6>& out) -> ResultVoid {
        return read_at(cfg_.protected_address, kEui48InternalAddress, out);
    }

    [[nodiscard]] auto read_eui64(std::array<std::uint8_t, 8>& out) -> ResultVoid {
        return read_at(cfg_.protected_address, kEui64InternalAddress, out);
    }

    [[nodiscard]] auto read_serial_number(std::array<std::uint8_t, kSerialNumberLengthBytes>& out)
        -> ResultVoid {
        return read_at(cfg_.protected_address, kSerialNumberInternalAddress, out);
    }

private:
    [[nodiscard]] auto read_at(std::uint16_t address, std::uint8_t internal,
                               std::span<std::uint8_t> out) -> ResultVoid {
        std::array<std::uint8_t, 1> tx{internal};
        return bus_->write_read(address, tx, out);
    }

    BusHandle* bus_;
    Config cfg_;
};

}  // namespace alloy::drivers::memory::at24mac402
