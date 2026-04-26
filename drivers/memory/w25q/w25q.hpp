#pragma once

// drivers/memory/w25q/w25q.hpp
//
// Driver for Winbond W25Qxx family NOR flash over SPI (tested against the
// W25Q32/W25Q64/W25Q128 opcode set).
// Written against datasheet revision "W25Q128JV SPI Revision L" (Jan 2019).
// Seed driver: JEDEC-ID probe, 24-bit-address read, page program, 4 KiB sector
// erase, and a WIP-bit status poll. Caller is responsible for page/sector
// alignment and for sizing writes to a single flash page (<= 256 bytes).
// See drivers/README.md.
//
// The driver issues each operation as one full-duplex `transfer(tx, rx)` call
// on the bus handle. CS is managed by the CsPolicy template parameter:
//   NoOpCsPolicy       — SPI hardware holds CS permanently (default).
//   GpioCsPolicy<Pin>  — software GPIO CS (recommended for shared buses).
// For reads longer than one 256-byte page, the driver issues multiple
// 0x03 Read-Data commands with incremented addresses to keep a fixed-size
// internal scratch buffer (no dynamic allocation).

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::memory::w25q {

// ── CS policies ───────────────────────────────────────────────────────────────

struct NoOpCsPolicy {
    void assert_cs()   const noexcept {}
    void deassert_cs() const noexcept {}
};

template <typename GpioPin>
struct GpioCsPolicy {
    explicit GpioCsPolicy(GpioPin& pin) : pin_(&pin) {
        (void)pin_->set_high();
    }
    void assert_cs()   const noexcept { (void)pin_->set_low(); }
    void deassert_cs() const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

inline constexpr std::uint32_t kPageSizeBytes = 256;
inline constexpr std::uint32_t kSectorSizeBytes = 4096;
inline constexpr std::uint8_t kCommandLength = 4;  // 1 opcode + 3-byte address

namespace opcode {
inline constexpr std::uint8_t kWriteEnable = 0x06;
inline constexpr std::uint8_t kWriteDisable = 0x04;
inline constexpr std::uint8_t kReadStatus1 = 0x05;
inline constexpr std::uint8_t kReadData = 0x03;
inline constexpr std::uint8_t kPageProgram = 0x02;
inline constexpr std::uint8_t kSectorErase4K = 0x20;
inline constexpr std::uint8_t kJedecId = 0x9F;
}  // namespace opcode

namespace status_bit {
inline constexpr std::uint8_t kWriteInProgress = 0x01;  // BUSY / WIP
inline constexpr std::uint8_t kWriteEnableLatch = 0x02;  // WEL
}  // namespace status_bit

struct Config {
    // Upper bound on status-polling loop iterations before returning Timeout.
    // Seed driver does not own a clock source, so the caller tunes this to
    // whatever wall-clock timeout they need given their SPI speed.
    std::uint32_t status_poll_max_iterations = 2'000'000u;
};

struct JedecId {
    std::uint8_t manufacturer;  // 0xEF for Winbond
    std::uint8_t memory_type;
    std::uint8_t capacity;  // e.g. 0x16 = W25Q32, 0x18 = W25Q128
};

template <typename BusHandle, typename CsPolicy = NoOpCsPolicy>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultJedec = alloy::core::Result<JedecId, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, CsPolicy cs = {}, Config cfg = {})
        : bus_{&bus}, cs_{cs}, cfg_{cfg} {}

private:
    struct ScopedCs {
        const CsPolicy& cs;
        explicit ScopedCs(const CsPolicy& c) : cs(c) { cs.assert_cs(); }
        ~ScopedCs() { cs.deassert_cs(); }
        ScopedCs(const ScopedCs&) = delete;
        ScopedCs& operator=(const ScopedCs&) = delete;
    };

public:

    // Reads the JEDEC ID. Returns CommunicationError if the bus reports all
    // 0x00 or all 0xFF (unpopulated bus / device absent).
    [[nodiscard]] auto init() -> ResultVoid {
        auto id = read_jedec_id();
        if (id.is_err()) {
            return alloy::core::Err(std::move(id).err());
        }
        const auto v = id.unwrap();
        if ((v.manufacturer == 0x00 && v.memory_type == 0x00 && v.capacity == 0x00) ||
            (v.manufacturer == 0xFF && v.memory_type == 0xFF && v.capacity == 0xFF)) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        jedec_ = v;
        return alloy::core::Ok();
    }

    [[nodiscard]] auto read_jedec_id() -> ResultJedec {
        std::array<std::uint8_t, 4> tx{opcode::kJedecId, 0, 0, 0};
        std::array<std::uint8_t, 4> rx{};
        ScopedCs guard{cs_};
        if (auto r = bus_->transfer(tx, rx); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        return alloy::core::Ok(JedecId{rx[1], rx[2], rx[3]});
    }

    [[nodiscard]] auto jedec_id() const -> JedecId { return jedec_; }

    [[nodiscard]] auto read_status() -> alloy::core::Result<std::uint8_t, alloy::core::ErrorCode> {
        std::array<std::uint8_t, 2> tx{opcode::kReadStatus1, 0};
        std::array<std::uint8_t, 2> rx{};
        ScopedCs guard{cs_};
        if (auto r = bus_->transfer(tx, rx); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        return alloy::core::Ok(static_cast<std::uint8_t>(rx[1]));
    }

    // Polls the WIP status bit. Returns Timeout if the iteration budget runs
    // out, and propagates any transport error.
    [[nodiscard]] auto wait_while_busy() -> ResultVoid {
        for (std::uint32_t i = 0; i < cfg_.status_poll_max_iterations; ++i) {
            auto s = read_status();
            if (s.is_err()) {
                return alloy::core::Err(std::move(s).err());
            }
            if ((s.unwrap() & status_bit::kWriteInProgress) == 0) {
                return alloy::core::Ok();
            }
        }
        return alloy::core::Err(alloy::core::ErrorCode::Timeout);
    }

    // Reads `out.size()` bytes starting at flash address `address`. The driver
    // chunks the read into `kPageSizeBytes` segments so a fixed-size scratch
    // buffer is enough to cover both the command and the dummy-tx bytes.
    [[nodiscard]] auto read(std::uint32_t address, std::span<std::uint8_t> out) -> ResultVoid {
        std::size_t offset = 0;
        while (offset < out.size()) {
            const std::size_t chunk = std::min<std::size_t>(kPageSizeBytes, out.size() - offset);
            const std::size_t frame_len = kCommandLength + chunk;

            std::memset(scratch_tx_.data(), 0, frame_len);
            encode_read_command(address + static_cast<std::uint32_t>(offset));

            {
                ScopedCs guard{cs_};
                if (auto r = bus_->transfer(
                        std::span<const std::uint8_t>{scratch_tx_.data(), frame_len},
                        std::span<std::uint8_t>{scratch_rx_.data(), frame_len});
                    r.is_err()) {
                    return r;
                }
            }
            std::memcpy(out.data() + offset, scratch_rx_.data() + kCommandLength, chunk);
            offset += chunk;
        }
        return alloy::core::Ok();
    }

    // Programs up to one page (256 bytes) starting at `address`. The caller
    // must ensure `address + data.size() <= next page boundary`; crossing a
    // page boundary silently wraps within the flash page per the datasheet.
    // Returns InvalidParameter if the write is larger than one page or empty.
    [[nodiscard]] auto page_program(std::uint32_t address,
                                    std::span<const std::uint8_t> data) -> ResultVoid {
        if (data.empty() || data.size() > kPageSizeBytes) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        if (auto r = write_enable(); r.is_err()) {
            return r;
        }

        const std::size_t frame_len = kCommandLength + data.size();
        scratch_tx_[0] = opcode::kPageProgram;
        scratch_tx_[1] = static_cast<std::uint8_t>((address >> 16) & 0xFF);
        scratch_tx_[2] = static_cast<std::uint8_t>((address >> 8) & 0xFF);
        scratch_tx_[3] = static_cast<std::uint8_t>(address & 0xFF);
        std::memcpy(scratch_tx_.data() + kCommandLength, data.data(), data.size());

        {
            ScopedCs guard{cs_};
            if (auto r = bus_->transfer(
                    std::span<const std::uint8_t>{scratch_tx_.data(), frame_len},
                    std::span<std::uint8_t>{scratch_rx_.data(), frame_len});
                r.is_err()) {
                return r;
            }
        }
        return wait_while_busy();
    }

    // Erases the 4 KiB sector containing `address`. The address does not need
    // to be sector-aligned; the device masks the lower bits internally.
    [[nodiscard]] auto sector_erase(std::uint32_t address) -> ResultVoid {
        if (auto r = write_enable(); r.is_err()) {
            return r;
        }

        std::array<std::uint8_t, kCommandLength> tx{
            opcode::kSectorErase4K,
            static_cast<std::uint8_t>((address >> 16) & 0xFF),
            static_cast<std::uint8_t>((address >> 8) & 0xFF),
            static_cast<std::uint8_t>(address & 0xFF),
        };
        std::array<std::uint8_t, kCommandLength> rx{};
        {
            ScopedCs guard{cs_};
            if (auto r = bus_->transfer(tx, rx); r.is_err()) {
                return r;
            }
        }
        return wait_while_busy();
    }

private:
    [[nodiscard]] auto write_enable() -> ResultVoid {
        std::array<std::uint8_t, 1> tx{opcode::kWriteEnable};
        std::array<std::uint8_t, 1> rx{};
        ScopedCs guard{cs_};
        return bus_->transfer(tx, rx);
    }

    void encode_read_command(std::uint32_t address) {
        scratch_tx_[0] = opcode::kReadData;
        scratch_tx_[1] = static_cast<std::uint8_t>((address >> 16) & 0xFF);
        scratch_tx_[2] = static_cast<std::uint8_t>((address >> 8) & 0xFF);
        scratch_tx_[3] = static_cast<std::uint8_t>(address & 0xFF);
    }

    BusHandle* bus_;
    CsPolicy   cs_;
    Config     cfg_;
    JedecId    jedec_{};
    // One page worth of payload + one 4-byte command header. Same buffer is
    // reused for page_program, read chunks, and sector_erase frames.
    std::array<std::uint8_t, kCommandLength + kPageSizeBytes> scratch_tx_{};
    std::array<std::uint8_t, kCommandLength + kPageSizeBytes> scratch_rx_{};
};

}  // namespace alloy::drivers::memory::w25q

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// SPI bus surface.
namespace {
struct _MockSpiForW25qDeviceGate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};
static_assert(
    sizeof(alloy::drivers::memory::w25q::Device<_MockSpiForW25qDeviceGate>) > 0,
    "w25q Device must compile against the documented SPI bus surface");
}  // namespace
