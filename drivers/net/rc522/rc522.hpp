#pragma once

// drivers/net/rc522/rc522.hpp
//
// Driver for NXP MFRC522 ISO 14443-A RFID reader/writer over SPI.
// Written against datasheet MFRC522 Rev 3.9 (April 2016).
// Seed driver: chip-version probe + PICC_REQA (card-present check) +
// anti-collision/select (4-byte UID, cascade level 1). See drivers/README.md.
//
// SPI mode 0. Bus surface: transfer(span<const uint8_t> tx, span<uint8_t> rx).
// Read  protocol: tx[0] = (reg << 1) | 0x80, tx[1] = 0x00; rx[1] = data.
// Write protocol: tx[0] = (reg << 1) & 0x7E, tx[1] = value; rx ignored.
//
// CS is managed by the CsPolicy template parameter:
//   NoOpCsPolicy       — SPI hardware holds CS permanently (default).
//   GpioCsPolicy<Pin>  — software GPIO CS (recommended for shared buses).

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::net::rc522 {

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

// ── Register map ──────────────────────────────────────────────────────────────

namespace reg {
inline constexpr std::uint8_t kCommandReg    = 0x01u;  ///< Transceiver command
inline constexpr std::uint8_t kComlEnReg     = 0x02u;  ///< Interrupt enable
inline constexpr std::uint8_t kComIrqReg     = 0x04u;  ///< Interrupt flags
inline constexpr std::uint8_t kErrorReg      = 0x06u;  ///< Error flags
inline constexpr std::uint8_t kFIFODataReg   = 0x09u;  ///< FIFO data R/W
inline constexpr std::uint8_t kFIFOLevelReg  = 0x0Au;  ///< FIFO byte count
inline constexpr std::uint8_t kControlReg    = 0x0Cu;  ///< Misc control
inline constexpr std::uint8_t kBitFramingReg = 0x0Du;  ///< Bit framing
inline constexpr std::uint8_t kCollReg       = 0x0Eu;  ///< Collision position
inline constexpr std::uint8_t kModeReg       = 0x11u;  ///< General mode
inline constexpr std::uint8_t kTxControlReg  = 0x14u;  ///< TX1/TX2 enable
inline constexpr std::uint8_t kTxASKReg      = 0x15u;  ///< 100% ASK force
inline constexpr std::uint8_t kCRCResultRegH = 0x21u;  ///< CRC result high
inline constexpr std::uint8_t kCRCResultRegL = 0x22u;  ///< CRC result low
inline constexpr std::uint8_t kModWidthReg   = 0x24u;  ///< Modulation width
inline constexpr std::uint8_t kRFCfgReg      = 0x26u;  ///< RF gain (0x48 = 38 dB)
inline constexpr std::uint8_t kGsNReg        = 0x27u;  ///< Conductance of antenna
inline constexpr std::uint8_t kTModeReg      = 0x2Au;  ///< Timer prescaler MSB
inline constexpr std::uint8_t kTPrescalerReg = 0x2Bu;  ///< Timer prescaler LSB
inline constexpr std::uint8_t kTReloadRegH   = 0x2Cu;  ///< Timer reload high
inline constexpr std::uint8_t kTReloadRegL   = 0x2Du;  ///< Timer reload low
inline constexpr std::uint8_t kVersionReg    = 0x37u;  ///< Chip version: 0x91/0x92
}  // namespace reg

// ── Commands (written to CommandReg) ──────────────────────────────────────────

namespace cmd {
inline constexpr std::uint8_t kIdle       = 0x00u;
inline constexpr std::uint8_t kTransceive = 0x0Cu;
inline constexpr std::uint8_t kMFAuthent  = 0x0Eu;
inline constexpr std::uint8_t kSoftReset  = 0x0Fu;
}  // namespace cmd

// ── IRQ / error flag masks ────────────────────────────────────────────────────

namespace irq {
inline constexpr std::uint8_t kRxIRq   = 0x20u;  ///< ComIrqReg: data received
inline constexpr std::uint8_t kIdleIRq = 0x10u;  ///< ComIrqReg: command complete
inline constexpr std::uint8_t kErrIRq  = 0x02u;  ///< ComIrqReg: error flag set
}  // namespace irq

namespace err {
inline constexpr std::uint8_t kCollErr    = 0x08u;
inline constexpr std::uint8_t kBufferOvfl = 0x10u;
inline constexpr std::uint8_t kCRCErr     = 0x04u;
inline constexpr std::uint8_t kParityErr  = 0x02u;
inline constexpr std::uint8_t kProtocErr  = 0x01u;
}  // namespace err

// ── Types ─────────────────────────────────────────────────────────────────────

/// Device configuration. No run-time fields; all behaviour is fixed by the
/// datasheet init sequence.
struct Config { /* no fields */ };

/// Holds a PICC UID read during anti-collision. Supports 4, 7, or 10-byte
/// UIDs; this seed driver only performs cascade level-1 (4-byte UIDs).
struct Uid {
    std::uint8_t data[10]{};
    std::uint8_t size{0};  ///< Valid bytes in data[]: 4, 7, or 10.
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 50 ms (after SoftReset).
inline void busy_wait_50ms() {
    volatile std::uint32_t n = 500'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Busy-wait approximately 5 ms (polling loop iterations).
inline void busy_wait_5ms() {
    volatile std::uint32_t n = 50'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Read one byte from the MFRC522 register `reg`.
/// tx = [(reg << 1) | 0x80, 0x00]; rx[1] = data byte.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto spi_read(Bus& bus, CsPolicy& cs, std::uint8_t reg_addr,
                             std::uint8_t& out)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    std::array<std::uint8_t, 2> tx{
        static_cast<std::uint8_t>((reg_addr << 1u) | 0x80u),
        0x00u
    };
    std::array<std::uint8_t, 2> rx{};

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx},
                          std::span<std::uint8_t>{rx});
    cs.deassert_cs();
    if (r.is_err()) {
        return alloy::core::Err(std::move(r).err());
    }
    out = rx[1];
    return alloy::core::Ok();
}

/// Write one byte to the MFRC522 register `reg`.
/// tx = [(reg << 1) & 0x7E, value]; rx ignored.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto spi_write(Bus& bus, CsPolicy& cs, std::uint8_t reg_addr,
                              std::uint8_t val)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    std::array<std::uint8_t, 2> tx{
        static_cast<std::uint8_t>((reg_addr << 1u) & 0x7Eu),
        val
    };
    std::array<std::uint8_t, 2> rx{};

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx},
                          std::span<std::uint8_t>{rx});
    cs.deassert_cs();
    return r;
}

/// Write one byte to the FIFO data register (FIFODataReg = 0x09).
/// Same as spi_write but the register is always kFIFODataReg.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto fifo_write_byte(Bus& bus, CsPolicy& cs, std::uint8_t data)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    return spi_write(bus, cs, reg::kFIFODataReg, data);
}

/// Execute a Transceive command: put the MFRC522 into Transceive, poll
/// ComIrqReg until RxIRq or error, then return the IRQ and ErrorReg state.
/// Returns Ok on successful SPI communication even if no card responded.
/// `irq_out`   — value of ComIrqReg after the command.
/// `error_out` — value of ErrorReg after the command.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto transceive_start_poll(Bus& bus, CsPolicy& cs,
                                          std::uint8_t& irq_out,
                                          std::uint8_t& error_out)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    // Set bit 7 of CommandReg to start the command.
    if (auto r = spi_write(bus, cs, reg::kCommandReg,
                           static_cast<std::uint8_t>(0x80u | cmd::kTransceive));
        r.is_err()) {
        return r;
    }
    // Set StartSend (bit 7 of BitFramingReg) to trigger TX.
    std::uint8_t bf{};
    if (auto r = spi_read(bus, cs, reg::kBitFramingReg, bf); r.is_err()) {
        return r;
    }
    if (auto r = spi_write(bus, cs, reg::kBitFramingReg,
                           static_cast<std::uint8_t>(bf | 0x80u));
        r.is_err()) {
        return r;
    }

    // Poll ComIrqReg for up to ~100 iterations (~500 ms at 5 ms each).
    constexpr std::uint32_t kMaxIter = 100u;
    for (std::uint32_t i = 0u; i < kMaxIter; ++i) {
        std::uint8_t irq{};
        if (auto r = spi_read(bus, cs, reg::kComIrqReg, irq); r.is_err()) {
            return r;
        }
        if ((irq & (irq::kRxIRq | irq::kIdleIRq | irq::kErrIRq)) != 0u) {
            irq_out = irq;
            // Read error register.
            std::uint8_t er{};
            if (auto r = spi_read(bus, cs, reg::kErrorReg, er); r.is_err()) {
                return r;
            }
            error_out = er;
            return alloy::core::Ok();
        }
        busy_wait_5ms();
    }
    // Timed-out at driver level — return the last IRQ state (RxIRq not set).
    irq_out   = 0u;
    error_out = 0u;
    return alloy::core::Ok();
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle, typename CsPolicy = NoOpCsPolicy>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultBool = alloy::core::Result<bool, alloy::core::ErrorCode>;
    using ResultUid  = alloy::core::Result<Uid,  alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, CsPolicy cs = {}, Config /*cfg*/ = {})
        : bus_{&bus}, cs_{cs} {}

    /// Verifies device presence via VersionReg and runs the MFRC522 startup
    /// sequence.
    ///
    /// Sequence:
    ///   1. Write CommandReg = SoftReset (0x0F).
    ///   2. Busy-wait ~50 ms for reset to complete.
    ///   3. Read VersionReg — expect 0x91 or 0x92; else CommunicationError.
    ///   4. Write TModeReg     = 0x80 (timer auto-start after transceive).
    ///   5. Write TPrescalerReg= 0xA9 (prescaler gives ~25 ms timeout).
    ///   6. Write TReloadRegH  = 0x03, TReloadRegL = 0xE8 (1000 ticks).
    ///   7. Write TxASKReg     = 0x40 (100% ASK modulation).
    ///   8. Write ModeReg      = 0x3D (CRC preset 0x6363).
    ///   9. Antenna on: read TxControlReg, set bits 0x03, write back.
    [[nodiscard]] auto init() -> ResultVoid {
        // 1. Soft reset.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kCommandReg, cmd::kSoftReset);
            r.is_err()) {
            return r;
        }

        // 2. Wait for reset to complete (~50 ms).
        detail::busy_wait_50ms();

        // 3. Version check.
        std::uint8_t ver{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kVersionReg, ver); r.is_err()) {
            return r;
        }
        if (ver != 0x91u && ver != 0x92u) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // 4. Timer mode: auto-start.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kTModeReg, 0x80u); r.is_err()) {
            return r;
        }

        // 5. Timer prescaler.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kTPrescalerReg, 0xA9u); r.is_err()) {
            return r;
        }

        // 6. Timer reload.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kTReloadRegH, 0x03u); r.is_err()) {
            return r;
        }
        if (auto r = detail::spi_write(*bus_, cs_, reg::kTReloadRegL, 0xE8u); r.is_err()) {
            return r;
        }

        // 7. 100% ASK.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kTxASKReg, 0x40u); r.is_err()) {
            return r;
        }

        // 8. Mode: CRC preset 0x6363 (ISO 14443-A).
        if (auto r = detail::spi_write(*bus_, cs_, reg::kModeReg, 0x3Du); r.is_err()) {
            return r;
        }

        // 9. Antenna on: set TX1 and TX2 enable bits.
        std::uint8_t tx_ctrl{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kTxControlReg, tx_ctrl); r.is_err()) {
            return r;
        }
        if (auto r = detail::spi_write(*bus_, cs_, reg::kTxControlReg,
                                       static_cast<std::uint8_t>(tx_ctrl | 0x03u));
            r.is_err()) {
            return r;
        }

        return alloy::core::Ok();
    }

    /// Sends REQA (0x26) and checks whether any PICC responded.
    ///
    /// Returns Ok(true) when a card is present and Ok(false) when none
    /// responded. Never returns a Timeout error — absence of a card is normal.
    [[nodiscard]] auto is_card_present() -> ResultBool {
        // Idle the transceiver first.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kCommandReg, cmd::kIdle);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Flush FIFO.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFIFOLevelReg, 0x80u);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // 7-bit frame (REQA is 7 bits).
        if (auto r = detail::spi_write(*bus_, cs_, reg::kBitFramingReg, 0x07u);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Write REQA command byte to FIFO.
        if (auto r = detail::fifo_write_byte(*bus_, cs_, 0x26u); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Execute Transceive and poll.
        std::uint8_t irq_val{};
        std::uint8_t err_val{};
        if (auto r = detail::transceive_start_poll(*bus_, cs_, irq_val, err_val);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Card present if RxIRq set and no critical error.
        const bool rx_ok = (irq_val & irq::kRxIRq) != 0u;
        const bool no_err = (err_val & (err::kBufferOvfl | err::kParityErr |
                                        err::kProtocErr)) == 0u;
        return alloy::core::Ok(rx_ok && no_err);
    }

    /// Anti-collision + select — reads the PICC UID.
    ///
    /// Only supports single-card, 4-byte UID (ISO 14443-A cascade level 1).
    /// Returns CommunicationError if the BCC check byte is invalid or the
    /// transceive fails.
    [[nodiscard]] auto read_uid() -> ResultUid {
        // Idle then flush FIFO.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kCommandReg, cmd::kIdle);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFIFOLevelReg, 0x80u);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Full 8-bit frames for anti-collision.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kBitFramingReg, 0x00u);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Write ANTICOLL command: SEL = 0x93, NVB = 0x20 (20 bits = no UID bits known yet).
        if (auto r = detail::fifo_write_byte(*bus_, cs_, 0x93u); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        if (auto r = detail::fifo_write_byte(*bus_, cs_, 0x20u); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Transceive.
        std::uint8_t irq_val{};
        std::uint8_t err_val{};
        if (auto r = detail::transceive_start_poll(*bus_, cs_, irq_val, err_val);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        if ((irq_val & irq::kRxIRq) == 0u) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if ((err_val & (err::kCollErr | err::kBufferOvfl |
                        err::kParityErr | err::kProtocErr)) != 0u) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Read 5 bytes from FIFO: UID[0..3] + BCC.
        std::array<std::uint8_t, 5> fifo_bytes{};
        for (std::size_t i = 0u; i < 5u; ++i) {
            if (auto r = detail::spi_read(*bus_, cs_, reg::kFIFODataReg, fifo_bytes[i]);
                r.is_err()) {
                return alloy::core::Err(std::move(r).err());
            }
        }

        // BCC check: XOR of the four UID bytes must equal fifo_bytes[4].
        const std::uint8_t bcc =
            static_cast<std::uint8_t>(fifo_bytes[0] ^ fifo_bytes[1] ^
                                      fifo_bytes[2] ^ fifo_bytes[3]);
        if (bcc != fifo_bytes[4]) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        Uid uid{};
        uid.size    = 4u;
        uid.data[0] = fifo_bytes[0];
        uid.data[1] = fifo_bytes[1];
        uid.data[2] = fifo_bytes[2];
        uid.data[3] = fifo_bytes[3];
        return alloy::core::Ok(uid);
    }

private:
    BusHandle* bus_;
    CsPolicy   cs_;
};

}  // namespace alloy::drivers::net::rc522

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// SPI bus surface.
namespace {
struct _MockSpiForRc522Gate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};
static_assert(
    sizeof(alloy::drivers::net::rc522::Device<_MockSpiForRc522Gate>) > 0,
    "rc522 Device must compile against the documented SPI bus surface");
}  // namespace
