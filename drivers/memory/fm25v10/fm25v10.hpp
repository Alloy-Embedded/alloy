#pragma once

// drivers/memory/fm25v10/fm25v10.hpp
//
// Driver for Cypress (Infineon) FM25V10 1-Mbit (128 KiB) Ferroelectric RAM
// (FRAM) over SPI.
// Written against datasheet FM25V10 Rev 1.0 (Cypress Semiconductor, 2015).
// Seed driver: device-ID probe, byte-granularity read, WREN + write.
// No erase cycle required — FRAM overwrites directly at byte granularity.
// See drivers/README.md.
//
// SPI mode 0 or mode 3. Bus surface: transfer(span<const uint8_t> tx,
// span<uint8_t> rx). tx and rx must be the same size.
//
// Address space: 17-bit (0x00000 – 0x1FFFF), sent as 3 bytes big-endian.
//
// Read / write are split into two transfers to avoid heap allocation:
//   Read:   [READ, A2, A1, A0] (4-byte header, 4 dummy rx), then
//           [0x00 × buf.size()] tx, buf as rx.
//   Write:  [WREN] (1-byte), then
//           [WRITE, A2, A1, A0] (4-byte header, 4 dummy rx), then
//           buf as tx with dummy rx in 64-byte stack chunks.
//
// CS must be deasserted between WREN and WRITE — this is handled automatically
// by the two-transfer sequence (each transfer asserts/deasserts CS via the
// CsPolicy). Similarly the READ/WRITE command and the data transfer must be
// contiguous (CS held across both): the driver therefore issues them as a
// single CS assertion by manually calling assert_cs() / deassert_cs() around
// both bus transfers.
//
// CS is managed by the CsPolicy template parameter:
//   NoOpCsPolicy       — SPI hardware holds CS permanently (default).
//   GpioCsPolicy<Pin>  — software GPIO CS (recommended for shared buses).

#include <array>
#include <cstdint>
#include <cstring>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::memory::fm25v10 {

// ── CS policies ───────────────────────────────────────────────────────────────

struct NoOpCsPolicy {
    void assert_cs()   const noexcept {}
    void deassert_cs() const noexcept {}
};

template <typename GpioPin>
struct GpioCsPolicy {
    explicit GpioCsPolicy(GpioPin& pin) : pin_{&pin} {
        (void)pin_->set_high();
    }
    void assert_cs()   const noexcept { (void)pin_->set_low(); }
    void deassert_cs() const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── Constants ─────────────────────────────────────────────────────────────────

/// Total addressable bytes in the FM25V10: 2^17 = 131072.
inline constexpr std::uint32_t kCapacityBytes = 131'072u;

namespace opcode {
inline constexpr std::uint8_t kWREN  = 0x06u;  ///< Write enable
inline constexpr std::uint8_t kWRDI  = 0x04u;  ///< Write disable
inline constexpr std::uint8_t kRDSR  = 0x05u;  ///< Read status register
inline constexpr std::uint8_t kWRSR  = 0x01u;  ///< Write status register
inline constexpr std::uint8_t kREAD  = 0x03u;  ///< Read memory
inline constexpr std::uint8_t kWRITE = 0x02u;  ///< Write memory
inline constexpr std::uint8_t kRDID  = 0x9Fu;  ///< Read device ID (8 bytes)
}  // namespace opcode

/// RDID byte[6] expected manufacturer code for FM25V10.
/// Note: some FM25V10 silicon revisions may return a different product code
/// in byte[7]; only the manufacturer byte (index 6) is checked.
inline constexpr std::uint8_t kExpectedManufacturer = 0xC2u;

/// Internal chunk size for write dummy-rx buffering (stack-allocated).
inline constexpr std::size_t kWriteChunkBytes = 64u;

// ── Types ─────────────────────────────────────────────────────────────────────

/// Device configuration. No run-time fields; the FM25V10 is ready immediately
/// after power-on without any device-side configuration.
struct Config { /* no fields */ };

// ── Private helpers ───────────────────────────────────────────────────────────

namespace detail {

/// RAII CS guard: asserts on construction, deasserts on destruction.
template <typename CsPolicy>
struct ScopedCs {
    const CsPolicy& cs;
    explicit ScopedCs(const CsPolicy& c) : cs(c) { cs.assert_cs(); }
    ~ScopedCs() { cs.deassert_cs(); }
    ScopedCs(const ScopedCs&)            = delete;
    ScopedCs& operator=(const ScopedCs&) = delete;
};

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle, typename CsPolicy = NoOpCsPolicy>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, CsPolicy cs = {}, Config /*cfg*/ = {})
        : bus_{&bus}, cs_{cs} {}

    /// Verifies device presence via RDID. Expects byte[6] == 0xC2 (Cypress
    /// manufacturer code). Returns CommunicationError on mismatch or SPI
    /// failure.
    [[nodiscard]] auto init() -> ResultVoid {
        // RDID: send 0x9F + 8 dummy bytes; receive 8 ID bytes in positions [1..8].
        std::array<std::uint8_t, 9> tx{};
        std::array<std::uint8_t, 9> rx{};
        tx[0] = opcode::kRDID;
        // tx[1..8] already zero.

        {
            detail::ScopedCs<CsPolicy> guard{cs_};
            if (auto r = bus_->transfer(std::span<const std::uint8_t>{tx},
                                        std::span<std::uint8_t>{rx});
                r.is_err()) {
                return r;
            }
        }

        // rx[0] is the dummy byte clocked during the command byte.
        // rx[1..8] are the 8 device-ID bytes.
        // Byte index 6 within those 8 bytes = rx[7].
        if (rx[7] != kExpectedManufacturer) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        return alloy::core::Ok();
    }

    /// Reads `buf.size()` bytes starting at `addr` into `buf`.
    ///
    /// Two separate transfers (CS held across both via manual assert/deassert):
    ///   1. tx=[READ, addr>>16, addr>>8, addr&0xFF], rx=[4 dummy bytes].
    ///   2. tx=[0x00 × buf.size()], rx=buf.
    ///
    /// Returns InvalidParameter if addr + buf.size() would exceed the device
    /// address space.
    [[nodiscard]] auto read(std::uint32_t addr,
                            std::span<std::uint8_t> buf) -> ResultVoid {
        if (buf.empty()) {
            return alloy::core::Ok();
        }
        if (addr + static_cast<std::uint32_t>(buf.size()) > kCapacityBytes) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        // Build 4-byte header.
        const std::array<std::uint8_t, 4> hdr_tx{
            opcode::kREAD,
            static_cast<std::uint8_t>((addr >> 16u) & 0xFFu),
            static_cast<std::uint8_t>((addr >> 8u)  & 0xFFu),
            static_cast<std::uint8_t>( addr         & 0xFFu),
        };
        std::array<std::uint8_t, 4> hdr_rx{};

        // Hold CS across both transfers: header + data.
        cs_.assert_cs();

        // Transfer 1: command + address.
        if (auto r = bus_->transfer(std::span<const std::uint8_t>{hdr_tx},
                                    std::span<std::uint8_t>{hdr_rx});
            r.is_err()) {
            cs_.deassert_cs();
            return r;
        }

        // Transfer 2: clock out zeros, capture data into buf.
        // We send buf.size() zero bytes; the received bytes go directly into buf.
        // To satisfy the bus surface (tx and rx must be the same size) we need
        // a tx buffer of the same size as buf. We cannot allocate on the heap,
        // so we iterate in kWriteChunkBytes chunks using a stack buffer.
        std::array<std::uint8_t, kWriteChunkBytes> tx_zeros{};
        // tx_zeros is already zero-initialised.

        std::size_t offset = 0u;
        while (offset < buf.size()) {
            const std::size_t chunk =
                (buf.size() - offset < kWriteChunkBytes)
                    ? (buf.size() - offset)
                    : kWriteChunkBytes;

            if (auto r = bus_->transfer(
                    std::span<const std::uint8_t>{tx_zeros.data(), chunk},
                    std::span<std::uint8_t>{buf.data() + offset, chunk});
                r.is_err()) {
                cs_.deassert_cs();
                return r;
            }
            offset += chunk;
        }

        cs_.deassert_cs();
        return alloy::core::Ok();
    }

    /// Writes `buf` to the FRAM starting at `addr`.
    ///
    /// Sequence:
    ///   1. WREN (separate CS transaction).
    ///   2. CS asserted; send [WRITE, addr>>16, addr>>8, addr&0xFF].
    ///   3. Send buf bytes (with dummy rx in 64-byte stack chunks). CS deasserted.
    ///
    /// Returns InvalidParameter if addr + buf.size() would exceed the device
    /// address space.
    [[nodiscard]] auto write(std::uint32_t addr,
                             std::span<const std::uint8_t> buf) -> ResultVoid {
        if (buf.empty()) {
            return alloy::core::Ok();
        }
        if (addr + static_cast<std::uint32_t>(buf.size()) > kCapacityBytes) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        // Step 1: Write Enable (WREN). CS must be deasserted between WREN and WRITE.
        {
            const std::array<std::uint8_t, 1> wren_tx{opcode::kWREN};
            std::array<std::uint8_t, 1> wren_rx{};
            detail::ScopedCs<CsPolicy> guard{cs_};
            if (auto r = bus_->transfer(std::span<const std::uint8_t>{wren_tx},
                                        std::span<std::uint8_t>{wren_rx});
                r.is_err()) {
                return r;
            }
        }

        // Step 2: WRITE header + data. CS held across header + all data chunks.
        const std::array<std::uint8_t, 4> hdr_tx{
            opcode::kWRITE,
            static_cast<std::uint8_t>((addr >> 16u) & 0xFFu),
            static_cast<std::uint8_t>((addr >> 8u)  & 0xFFu),
            static_cast<std::uint8_t>( addr         & 0xFFu),
        };
        std::array<std::uint8_t, 4> hdr_rx{};

        cs_.assert_cs();

        if (auto r = bus_->transfer(std::span<const std::uint8_t>{hdr_tx},
                                    std::span<std::uint8_t>{hdr_rx});
            r.is_err()) {
            cs_.deassert_cs();
            return r;
        }

        // Step 3: stream buf in chunks, discarding rx into a stack dummy buffer.
        std::array<std::uint8_t, kWriteChunkBytes> rx_dummy{};

        std::size_t offset = 0u;
        while (offset < buf.size()) {
            const std::size_t chunk =
                (buf.size() - offset < kWriteChunkBytes)
                    ? (buf.size() - offset)
                    : kWriteChunkBytes;

            if (auto r = bus_->transfer(
                    std::span<const std::uint8_t>{buf.data() + offset, chunk},
                    std::span<std::uint8_t>{rx_dummy.data(), chunk});
                r.is_err()) {
                cs_.deassert_cs();
                return r;
            }
            offset += chunk;
        }

        cs_.deassert_cs();
        return alloy::core::Ok();
    }

private:
    BusHandle* bus_;
    CsPolicy   cs_;
};

}  // namespace alloy::drivers::memory::fm25v10

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// SPI bus surface.
namespace {
struct _MockSpiForFm25v10Gate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};
static_assert(
    sizeof(alloy::drivers::memory::fm25v10::Device<_MockSpiForFm25v10Gate>) > 0,
    "fm25v10 Device must compile against the documented SPI bus surface");
}  // namespace
