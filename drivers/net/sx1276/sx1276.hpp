#pragma once

// drivers/net/sx1276/sx1276.hpp
//
// Driver for Semtech SX1276 LoRa/FSK sub-GHz transceiver over SPI.
// Written against datasheet SX1276/77/78/79 Rev 7 (Semtech, March 2020).
// Seed driver: version probe + LoRa mode init + blocking TX + blocking
// single-shot RX + packet RSSI read. See drivers/README.md.
//
// SPI mode 0, MSB first.  Bus surface:
//   transfer(span<const uint8_t> tx, span<uint8_t> rx) -> Result<void, ErrorCode>
//
// Read  protocol: tx[0] = reg & 0x7F,  tx[1..N] = 0x00; rx[1..N] = data.
// Write protocol: tx[0] = reg | 0x80,  tx[1]    = value; rx ignored.
//
// CS is managed by the CsPolicy template parameter:
//   NoOpCsPolicy      — SPI hardware holds CS permanently (default).
//   GpioCsPolicy<Pin> — software GPIO CS (recommended for shared buses).
//
// Frequency register calculation (no floating-point):
//   FRF = (freq_hz * 524288) / 32000000
//   where 524288 = 2^19 and 32 MHz is the SX1276 XTAL frequency.
//   All arithmetic uses uint64_t to avoid 32-bit overflow.
//
// DIO0 interrupt line is NOT used; the driver polls RegIrqFlags exclusively.
// Integrate DIO0 edge detection at the application layer if lower latency is
// required.

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::net::sx1276 {

// ── Constants ─────────────────────────────────────────────────────────────────

/// RegVersion expected value for SX1276/77/78/79.
inline constexpr std::uint8_t kExpectedVersion = 0x12u;

/// SPI read bit — OR into the register address for reads.
inline constexpr std::uint8_t kSpiReadBit = 0x00u;  // clear MSB for read

/// SPI write bit — OR into the register address for writes.
inline constexpr std::uint8_t kSpiWriteBit = 0x80u;

// ── Register map ──────────────────────────────────────────────────────────────

namespace reg {
inline constexpr std::uint8_t kFifo              = 0x00u;
inline constexpr std::uint8_t kOpMode            = 0x01u;
inline constexpr std::uint8_t kFrfMsb            = 0x06u;
inline constexpr std::uint8_t kFrfMid            = 0x07u;
inline constexpr std::uint8_t kFrfLsb            = 0x08u;
inline constexpr std::uint8_t kPaConfig          = 0x09u;
inline constexpr std::uint8_t kPaRamp            = 0x0Au;
inline constexpr std::uint8_t kOcp               = 0x0Bu;
inline constexpr std::uint8_t kLna               = 0x0Cu;
inline constexpr std::uint8_t kFifoAddrPtr       = 0x0Du;
inline constexpr std::uint8_t kFifoTxBaseAddr    = 0x0Eu;
inline constexpr std::uint8_t kFifoRxBaseAddr    = 0x0Fu;
inline constexpr std::uint8_t kFifoRxCurrentAddr = 0x10u;
inline constexpr std::uint8_t kIrqFlagsMask      = 0x11u;
inline constexpr std::uint8_t kIrqFlags          = 0x12u;
inline constexpr std::uint8_t kRxNbBytes         = 0x13u;
inline constexpr std::uint8_t kPktRssiValue      = 0x1Au;  // last-packet RSSI
inline constexpr std::uint8_t kModemConfig1      = 0x1Du;
inline constexpr std::uint8_t kModemConfig2      = 0x1Eu;
inline constexpr std::uint8_t kSymbTimeout       = 0x1Fu;
inline constexpr std::uint8_t kPreambleMsb       = 0x20u;
inline constexpr std::uint8_t kPreambleLsb       = 0x21u;
inline constexpr std::uint8_t kPayloadLength     = 0x22u;
inline constexpr std::uint8_t kModemConfig3      = 0x26u;
inline constexpr std::uint8_t kDioMapping1       = 0x40u;
inline constexpr std::uint8_t kVersion           = 0x42u;
inline constexpr std::uint8_t kPaDac             = 0x4Du;
}  // namespace reg

// ── IRQ flag bits (RegIrqFlags = 0x12) ────────────────────────────────────────

namespace irq {
inline constexpr std::uint8_t kCadDetected   = 0x01u;
inline constexpr std::uint8_t kFhssChangeChannel = 0x02u;
inline constexpr std::uint8_t kCadDone       = 0x04u;
inline constexpr std::uint8_t kTxDone        = 0x08u;
inline constexpr std::uint8_t kValidHeader   = 0x10u;
inline constexpr std::uint8_t kCrcError      = 0x20u;
inline constexpr std::uint8_t kRxDone        = 0x40u;
inline constexpr std::uint8_t kRxTimeout     = 0x80u;
}  // namespace irq

// ── Operating modes (RegOpMode bits [2:0], with LoRa bit 7 set) ───────────────

namespace mode {
inline constexpr std::uint8_t kLoRaSleep    = 0x80u;  // LoRa | Sleep
inline constexpr std::uint8_t kLoRaStandby  = 0x81u;  // LoRa | Standby
inline constexpr std::uint8_t kLoRaFSTX     = 0x82u;  // LoRa | FSTX
inline constexpr std::uint8_t kLoRaTX       = 0x83u;  // LoRa | TX
inline constexpr std::uint8_t kLoRaFSRX     = 0x84u;  // LoRa | FSRX
inline constexpr std::uint8_t kLoRaRxCont   = 0x85u;  // LoRa | RxContinuous
inline constexpr std::uint8_t kLoRaRxSingle = 0x86u;  // LoRa | RxSingle
}  // namespace mode

// ── Polling limits ────────────────────────────────────────────────────────────

/// Maximum polling iterations for TX/RX completion. At ~150 MHz Cortex-M7
/// each iteration takes a few SPI clock cycles; 10 million gives >5 s margin
/// at 1 MHz SPI.
inline constexpr std::uint32_t kMaxPollIters = 10'000'000u;

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

// ── Configuration ─────────────────────────────────────────────────────────────

enum class Bandwidth : std::uint8_t {
    kHz125 = 0x70u,  ///< 125 kHz — highest sensitivity, lowest data rate
    kHz250 = 0x80u,  ///< 250 kHz
    kHz500 = 0x90u,  ///< 500 kHz — lowest sensitivity, highest data rate
};

enum class SpreadingFactor : std::uint8_t {
    SF7  = 7u,   ///< Fastest; shortest range
    SF8  = 8u,
    SF9  = 9u,
    SF10 = 10u,
    SF11 = 11u,
    SF12 = 12u,  ///< Slowest; longest range
};

enum class CodingRate : std::uint8_t {
    CR4_5 = 0x02u,  ///< 4/5 — least overhead
    CR4_6 = 0x04u,
    CR4_7 = 0x06u,
    CR4_8 = 0x08u,  ///< 4/8 — most resilient
};

struct Config {
    std::uint32_t   frequency_hz  = 915'000'000u;     ///< RF carrier (Hz); 915 MHz default
    Bandwidth       bandwidth     = Bandwidth::kHz125;
    SpreadingFactor sf            = SpreadingFactor::SF7;
    CodingRate      cr            = CodingRate::CR4_5;
    std::uint8_t    tx_power_dbm  = 17u;               ///< Output power 2..20 dBm via PA_BOOST
    bool            crc_enable    = true;
};

// ── Private detail ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 10 ms (LoRa mode-switch settling margin).
inline void busy_wait_10ms() noexcept {
    volatile std::uint32_t n = 100'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Read N data bytes beginning at reg_addr.
/// SPI frame: tx[0] = reg & 0x7F (MSB clear = read), tx[1..N] = 0x00;
///            rx[1..N] = register data.
template <typename Bus, typename CsPolicy, std::size_t N>
[[nodiscard]] auto spi_read(Bus& bus, CsPolicy& cs, std::uint8_t reg,
                             std::array<std::uint8_t, N>& out)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    static_assert(N >= 1u, "spi_read: output buffer must hold at least 1 byte");
    std::array<std::uint8_t, 1u + N> tx{};
    std::array<std::uint8_t, 1u + N> rx{};
    tx[0] = static_cast<std::uint8_t>(reg & 0x7Fu);  // MSB = 0 → read

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx},
                          std::span<std::uint8_t>{rx});
    cs.deassert_cs();

    if (r.is_err()) {
        return alloy::core::Err(std::move(r).err());
    }
    for (std::size_t i = 0u; i < N; ++i) {
        out[i] = rx[i + 1u];
    }
    return alloy::core::Ok();
}

/// Write a single byte to reg_addr.
/// SPI frame: tx[0] = reg | 0x80 (MSB set = write), tx[1] = value.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto spi_write(Bus& bus, CsPolicy& cs,
                              std::uint8_t reg, std::uint8_t val)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    std::array<std::uint8_t, 2u> tx{static_cast<std::uint8_t>(reg | 0x80u), val};
    std::array<std::uint8_t, 2u> rx{};

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx},
                          std::span<std::uint8_t>{rx});
    cs.deassert_cs();
    return r;
}

/// Burst-write up to 255 payload bytes into the FIFO register (0x00).
/// SPI frame: tx[0] = 0x80 (write FIFO), tx[1..n] = payload bytes.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto spi_write_fifo(Bus& bus, CsPolicy& cs,
                                   std::span<const std::uint8_t> payload)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    // Maximum LoRa payload is 255 bytes; 1 command byte + 255 data = 256.
    std::array<std::uint8_t, 256u> tx{};
    std::array<std::uint8_t, 256u> rx{};

    const std::size_t n = std::min(payload.size(), static_cast<std::size_t>(255u));
    tx[0] = 0x80u;  // write FIFO register
    for (std::size_t i = 0u; i < n; ++i) {
        tx[i + 1u] = payload[i];
    }

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx.data(), n + 1u},
                          std::span<std::uint8_t>{rx.data(), n + 1u});
    cs.deassert_cs();
    return r;
}

/// Burst-read n bytes from the FIFO register (0x00) into dst.
/// SPI frame: tx[0] = 0x00 (read FIFO), tx[1..n] = 0x00; rx[1..n] = data.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto spi_read_fifo(Bus& bus, CsPolicy& cs,
                                  std::span<std::uint8_t> dst, std::uint8_t n_bytes)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    const std::size_t n = static_cast<std::size_t>(n_bytes);
    std::array<std::uint8_t, 256u> tx{};  // zero-initialised
    std::array<std::uint8_t, 256u> rx{};
    tx[0] = 0x00u;  // read FIFO register

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx.data(), n + 1u},
                          std::span<std::uint8_t>{rx.data(), n + 1u});
    cs.deassert_cs();

    if (r.is_err()) {
        return alloy::core::Err(std::move(r).err());
    }
    const std::size_t copy_n = std::min(n, dst.size());
    for (std::size_t i = 0u; i < copy_n; ++i) {
        dst[i] = rx[i + 1u];
    }
    return alloy::core::Ok();
}

/// Poll RegIrqFlags until the bits in mask are all set.
/// Returns Timeout if kMaxPollIters is exhausted without success.
template <typename Bus, typename CsPolicy>
[[nodiscard]] auto busy_wait_irq(Bus& bus, CsPolicy& cs, std::uint8_t mask)
    -> alloy::core::Result<std::uint8_t, alloy::core::ErrorCode>
{
    for (std::uint32_t i = 0u; i < kMaxPollIters; ++i) {
        std::array<std::uint8_t, 1u> flags{};
        if (auto r = spi_read(bus, cs, reg::kIrqFlags, flags); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        if ((flags[0] & mask) == mask) {
            return alloy::core::Ok(flags[0]);
        }
    }
    return alloy::core::Err(alloy::core::ErrorCode::Timeout);
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle, typename CsPolicy = NoOpCsPolicy>
class Device {
public:
    using ResultVoid   = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultU8     = alloy::core::Result<std::uint8_t, alloy::core::ErrorCode>;
    using ResultI16    = alloy::core::Result<std::int16_t, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, CsPolicy cs = {}, Config cfg = {})
        : bus_{&bus}, cs_{cs}, cfg_{cfg} {}

    /// Initialise the SX1276 into LoRa mode with the parameters in cfg_.
    ///
    /// Sequence:
    ///   1.  Read RegVersion → must be 0x12.
    ///   2.  Write RegOpMode = 0x80 (LoRa, sleep).
    ///   3.  Program carrier frequency via RegFrfMsb/Mid/Lsb.
    ///       FRF = (freq_hz * 524288ULL) / 32000000ULL (integer, uint64_t).
    ///   4.  Write RegFifoTxBaseAddr = 0x00, RegFifoRxBaseAddr = 0x00.
    ///   5.  Write RegLna = 0x23 (LNA G1 = max gain, boost on for HF port).
    ///   6.  Write RegModemConfig1 = bandwidth | coding_rate | 0x00 (explicit header).
    ///   7.  Write RegModemConfig2 = (SF << 4) | (crc ? 0x04 : 0x00).
    ///   8.  Write RegModemConfig3 = 0x04 (AgcAutoOn=1, LowDataRateOpt=0).
    ///       Note: set LowDataRateOpt (bit 3) if SF >= 11 and BW = 125 kHz per
    ///       datasheet §4.1.1.6.
    ///   9.  Write RegPaConfig = 0x80 | 0x70 | (tx_power_dbm - 2) (PA_BOOST,
    ///       MaxPower=7, OutputPower=tx_power_dbm-2).
    ///       Write RegPaDac = 0x87 if tx_power_dbm > 17, else 0x84.
    ///  10.  Write RegOpMode = 0x81 (LoRa, standby).
    [[nodiscard]] auto init() -> ResultVoid {
        // 1. Version check.
        std::array<std::uint8_t, 1u> ver{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kVersion, ver); r.is_err()) {
            return r;
        }
        if (ver[0] != kExpectedVersion) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // 2. Switch to LoRa sleep so all LoRa registers become accessible.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kOpMode, mode::kLoRaSleep);
            r.is_err()) {
            return r;
        }
        // Brief settling after mode switch.
        detail::busy_wait_10ms();

        // 3. Carrier frequency.
        //    FRF = (freq_hz * 2^19) / 32_000_000
        //    2^19 = 524288; use uint64_t to prevent 32-bit overflow.
        const std::uint64_t frf =
            (static_cast<std::uint64_t>(cfg_.frequency_hz) * 524288ULL) /
            32'000'000ULL;
        const auto frf_msb = static_cast<std::uint8_t>((frf >> 16u) & 0xFFu);
        const auto frf_mid = static_cast<std::uint8_t>((frf >>  8u) & 0xFFu);
        const auto frf_lsb = static_cast<std::uint8_t>( frf         & 0xFFu);

        if (auto r = detail::spi_write(*bus_, cs_, reg::kFrfMsb, frf_msb); r.is_err()) {
            return r;
        }
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFrfMid, frf_mid); r.is_err()) {
            return r;
        }
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFrfLsb, frf_lsb); r.is_err()) {
            return r;
        }

        // 4. FIFO base pointers.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFifoTxBaseAddr, 0x00u);
            r.is_err()) {
            return r;
        }
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFifoRxBaseAddr, 0x00u);
            r.is_err()) {
            return r;
        }

        // 5. LNA: G1 (maximum gain), LNA boost HF on.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kLna, 0x23u); r.is_err()) {
            return r;
        }

        // 6. ModemConfig1: BW | CodingRate | ExplicitHeader(0).
        const std::uint8_t mc1 =
            static_cast<std::uint8_t>(
                static_cast<std::uint8_t>(cfg_.bandwidth) |
                static_cast<std::uint8_t>(cfg_.cr));
        if (auto r = detail::spi_write(*bus_, cs_, reg::kModemConfig1, mc1);
            r.is_err()) {
            return r;
        }

        // 7. ModemConfig2: SF[7:4] | CrcOn[2] | RxTimeoutMsb=0.
        const std::uint8_t mc2 =
            static_cast<std::uint8_t>(
                (static_cast<std::uint8_t>(cfg_.sf) << 4u) |
                (cfg_.crc_enable ? 0x04u : 0x00u));
        if (auto r = detail::spi_write(*bus_, cs_, reg::kModemConfig2, mc2);
            r.is_err()) {
            return r;
        }

        // 8. ModemConfig3: AgcAutoOn=1; LowDataRateOptimize when symbol time > 16 ms
        //    (SF11/SF12 at 125 kHz per datasheet §4.1.1.6).
        const bool low_dr_opt =
            (cfg_.sf == SpreadingFactor::SF11 || cfg_.sf == SpreadingFactor::SF12) &&
            (cfg_.bandwidth == Bandwidth::kHz125);
        const std::uint8_t mc3 =
            static_cast<std::uint8_t>(0x04u | (low_dr_opt ? 0x08u : 0x00u));
        if (auto r = detail::spi_write(*bus_, cs_, reg::kModemConfig3, mc3);
            r.is_err()) {
            return r;
        }

        // 9. PA configuration.
        //    PA_BOOST path: PaSelect=1 (bit 7), MaxPower=7 (bits[6:4]=0x7),
        //    OutputPower = tx_power_dbm - 2 (bits[3:0], clamped 0..15).
        const std::uint8_t out_pwr =
            static_cast<std::uint8_t>(
                (cfg_.tx_power_dbm >= 2u) ?
                    std::min(static_cast<std::uint8_t>(cfg_.tx_power_dbm - 2u),
                             static_cast<std::uint8_t>(15u)) :
                    0u);
        const std::uint8_t pa_config =
            static_cast<std::uint8_t>(0x80u | 0x70u | out_pwr);
        if (auto r = detail::spi_write(*bus_, cs_, reg::kPaConfig, pa_config);
            r.is_err()) {
            return r;
        }
        // RegPaDac: 0x87 enables +20 dBm boost mode; 0x84 is the default.
        const std::uint8_t pa_dac =
            (cfg_.tx_power_dbm > 17u) ? 0x87u : 0x84u;
        if (auto r = detail::spi_write(*bus_, cs_, reg::kPaDac, pa_dac); r.is_err()) {
            return r;
        }

        // 10. Leave in LoRa standby ready to TX or RX.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kOpMode, mode::kLoRaStandby);
            r.is_err()) {
            return r;
        }

        return alloy::core::Ok();
    }

    /// Transmit payload over LoRa (blocking, polls TxDone flag).
    ///
    /// Payload must be 1–255 bytes. Empty spans return InvalidParameter.
    ///
    /// Sequence:
    ///   1. Standby mode.
    ///   2. Reset FIFO pointer to TX base (0x00).
    ///   3. Burst-write payload to FIFO.
    ///   4. Write RegPayloadLength.
    ///   5. TX mode → poll TxDone (0x08) flag, max kMaxPollIters.
    ///   6. Clear all IRQ flags.
    [[nodiscard]] auto transmit(std::span<const std::uint8_t> payload) -> ResultVoid {
        if (payload.empty() || payload.size() > 255u) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        // 1. Standby.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kOpMode, mode::kLoRaStandby);
            r.is_err()) {
            return r;
        }

        // 2. Reset FIFO address pointer to TX base.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFifoAddrPtr, 0x00u);
            r.is_err()) {
            return r;
        }

        // 3. Burst-write payload bytes into FIFO.
        if (auto r = detail::spi_write_fifo(*bus_, cs_, payload); r.is_err()) {
            return r;
        }

        // 4. Set payload length.
        const auto len = static_cast<std::uint8_t>(payload.size());
        if (auto r = detail::spi_write(*bus_, cs_, reg::kPayloadLength, len);
            r.is_err()) {
            return r;
        }

        // 5. Trigger TX.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kOpMode, mode::kLoRaTX);
            r.is_err()) {
            return r;
        }

        // Poll until TxDone bit is set.
        if (auto r = detail::busy_wait_irq(*bus_, cs_, irq::kTxDone); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // 6. Clear all IRQ flags.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kIrqFlags, 0xFFu);
            r.is_err()) {
            return r;
        }

        return alloy::core::Ok();
    }

    /// Receive one LoRa packet (single-shot mode, blocking).
    ///
    /// Sequence:
    ///   1. Single-RX mode → poll ValidHeader|RxDone (0x50) flags.
    ///   2. If CrcError is set return CommunicationError.
    ///   3. Read RegRxNbBytes → n.
    ///   4. Read RegFifoRxCurrentAddr → ptr.
    ///   5. Set RegFifoAddrPtr = ptr; burst-read n bytes from FIFO.
    ///   6. Return min(n, rx_buf.size()) as the byte count.
    ///
    /// Returns the number of bytes written to rx_buf.
    [[nodiscard]] auto receive(std::span<std::uint8_t> rx_buf) -> ResultU8 {
        // 1. Enter single-RX mode.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kOpMode, mode::kLoRaRxSingle);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Poll for ValidHeader + RxDone.
        auto poll = detail::busy_wait_irq(*bus_, cs_,
                                          irq::kValidHeader | irq::kRxDone);
        if (poll.is_err()) {
            return alloy::core::Err(std::move(poll).err());
        }

        const std::uint8_t flags = poll.unwrap();

        // 2. CRC error check.
        if ((flags & irq::kCrcError) != 0u) {
            // Clear flags before returning.
            (void)detail::spi_write(*bus_, cs_, reg::kIrqFlags, 0xFFu);
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // 3. Number of bytes received.
        std::array<std::uint8_t, 1u> nb_bytes{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kRxNbBytes, nb_bytes);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        const std::uint8_t n = nb_bytes[0];

        // 4. Current RX FIFO pointer.
        std::array<std::uint8_t, 1u> rx_ptr{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kFifoRxCurrentAddr, rx_ptr);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // 5. Set FIFO address pointer and burst-read.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kFifoAddrPtr, rx_ptr[0]);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        if (auto r = detail::spi_read_fifo(*bus_, cs_, rx_buf, n); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Clear all IRQ flags.
        (void)detail::spi_write(*bus_, cs_, reg::kIrqFlags, 0xFFu);

        // 6. Return bytes copied (clamped to rx_buf size).
        const std::uint8_t copied =
            static_cast<std::uint8_t>(
                std::min(static_cast<std::size_t>(n), rx_buf.size()));
        return alloy::core::Ok(copied);
    }

    /// Read RSSI of the last received packet (dBm).
    ///
    /// Formula (HF port, 915 MHz): RSSI = -157 + RegPktRssiValue.
    /// Returns the RSSI value as int16_t to handle the full signed range.
    [[nodiscard]] auto rssi() -> ResultI16 {
        std::array<std::uint8_t, 1u> raw{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kPktRssiValue, raw);
            r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }
        const std::int16_t rssi_dbm =
            static_cast<std::int16_t>(-157 + static_cast<std::int16_t>(raw[0]));
        return alloy::core::Ok(rssi_dbm);
    }

private:
    BusHandle* bus_;
    CsPolicy   cs_;
    Config     cfg_;
};

}  // namespace alloy::drivers::net::sx1276

// ── Static-assert concept gate ────────────────────────────────────────────────
// Instantiates Device against a minimal mock SPI bus to catch API drift at
// include time. The mock transfer() zero-fills rx so init() will fail the
// version check at runtime, but the compilation gate only checks types.
namespace {
struct _MockSpiBusForSx1276Gate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        for (auto& b : rx) { b = 0u; }
        return alloy::core::Ok();
    }
};

static_assert(
    sizeof(alloy::drivers::net::sx1276::Device<_MockSpiBusForSx1276Gate>) > 0u,
    "sx1276 Device must compile against the documented SPI bus surface");
}  // namespace
