/// @file hal/i2c/lite.hpp
/// Lightweight, direct-MMIO I2C master driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
/// Works whenever `ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE` is true.
///
/// Supports:
///   - ST I2C v2 (F3 / G0 / G4 / H7 / L4 / WB): kIpVersion starts with "i2c2"
///     Uses TIMINGR, AUTOEND mode, CR2.NBYTES, 7-bit addressing.
///
/// NOT supported (different register layout):
///   - ST I2C v1 (F1 / F2): SR1/SR2/CCR/TRISE registers — use the legacy HAL.
///
/// Register layout (i2c2_v* — modern unified layout):
///   0x00 CR1      — PE enable, interrupt enables, ANFOFF, DNF
///   0x04 CR2      — SADD, NBYTES, RELOAD, AUTOEND, RD_WRN, START, STOP
///   0x08 OAR1     — own address 1
///   0x0C OAR2     — own address 2
///   0x10 TIMINGR  — timing register (precomputed value from CubeMX / I2cTimingPreset)
///   0x14 TIMEOUTR — timeout register
///   0x18 ISR      — interrupt and status register
///   0x1C ICR      — interrupt flag clear register
///   0x20 PECR     — PEC register
///   0x24 RXDR     — receive data register
///   0x28 TXDR     — transmit data register
///
/// IMPORTANT: the caller must enable the peripheral clock (and deassert reset)
/// before calling `configure()`.  With alloy-codegen v2.1:
/// @code
///   namespace dev = alloy::device::traits;
///   dev::peripheral_on<dev::i2c1>();   // clk + rst
/// @endcode
///
/// Typical usage (I2C1 at 100 kHz on STM32G071 with 16 MHz clock):
/// @code
///   #include "hal/i2c.hpp"
///   #include "device/runtime.hpp"
///
///   namespace dev = alloy::device::traits;
///   using I2c1 = alloy::hal::i2c::lite::port<dev::i2c1>;
///
///   dev::peripheral_on<dev::i2c1>();
///   // TIMINGR = 0x00303D5B (STM32CubeMX: 16 MHz HSI → 100 kHz Standard)
///   I2c1::configure({.timingr = 0x00303D5Bu});
///
///   std::uint8_t buf[] = {0x01, 0xAB};
///   I2c1::write(0x48u, buf);         // write to slave at address 0x48
///   I2c1::read(0x48u, buf);          // read back 2 bytes
///   I2c1::probe(0x48u);              // bus scan: ACK → Ok, NACK → Nack
/// @endcode
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::i2c::lite {

// ============================================================================
// Detail — register layout and compile-time helpers
// ============================================================================

namespace detail {

/// STM32 I2C v2 register map (F3/G0/G4/H7/L4/WB — same layout).
struct i2c_regs {
    std::uint32_t cr1;       ///< 0x00
    std::uint32_t cr2;       ///< 0x04
    std::uint32_t oar1;      ///< 0x08
    std::uint32_t oar2;      ///< 0x0C
    std::uint32_t timingr;   ///< 0x10
    std::uint32_t timeoutr;  ///< 0x14
    std::uint32_t isr;       ///< 0x18 — status (read); some bits w1c
    std::uint32_t icr;       ///< 0x1C — flag clear (write-1-to-clear)
    std::uint32_t pecr;      ///< 0x20
    std::uint32_t rxdr;      ///< 0x24 — receive data (8-bit)
    std::uint32_t txdr;      ///< 0x28 — transmit data (8-bit)
};

// CR1 bits
inline constexpr std::uint32_t kCr1Pe      = 1u << 0;   ///< Peripheral enable
inline constexpr std::uint32_t kCr1Txie    = 1u << 1;   ///< TX interrupt enable (TXIS)
inline constexpr std::uint32_t kCr1Rxie    = 1u << 2;   ///< RX interrupt enable (RXNE)
inline constexpr std::uint32_t kCr1Addrie  = 1u << 3;   ///< Address-match interrupt enable
inline constexpr std::uint32_t kCr1Nackie  = 1u << 4;   ///< NACK interrupt enable
inline constexpr std::uint32_t kCr1Stopie  = 1u << 5;   ///< STOP interrupt enable
inline constexpr std::uint32_t kCr1Tcie    = 1u << 6;   ///< Transfer-complete interrupt enable
inline constexpr std::uint32_t kCr1Errie   = 1u << 7;   ///< Error interrupt enable (BERR/ARLO/OVR)

// CR2 bits
inline constexpr std::uint32_t kCr2SaddShift  = 1u;     ///< SADD[7:1] for 7-bit
inline constexpr std::uint32_t kCr2SaddMask   = 0x3FFu; ///< full SADD[9:0]
inline constexpr std::uint32_t kCr2RdWrn      = 1u << 10; ///< 1 = read, 0 = write
inline constexpr std::uint32_t kCr2Add10      = 1u << 11; ///< 10-bit addressing mode
inline constexpr std::uint32_t kCr2Head10r    = 1u << 12; ///< Read: resend header only (no 2nd addr byte)
inline constexpr std::uint32_t kCr2NbytesShift = 16u;   ///< NBYTES[7:0]
inline constexpr std::uint32_t kCr2NbytesMask  = 0xFFu << 16;
inline constexpr std::uint32_t kCr2Reload      = 1u << 24; ///< NBYTES reload
inline constexpr std::uint32_t kCr2Autoend     = 1u << 25; ///< Auto STOP after NBYTES
inline constexpr std::uint32_t kCr2Start       = 1u << 13; ///< START generation
inline constexpr std::uint32_t kCr2Stop        = 1u << 14; ///< STOP generation

// ISR bits
inline constexpr std::uint32_t kIsrTxis    = 1u << 1;   ///< TX interrupt status (TDR empty)
inline constexpr std::uint32_t kIsrRxne    = 1u << 2;   ///< RX not empty (byte in RXDR)
inline constexpr std::uint32_t kIsrAddr    = 1u << 3;   ///< Address matched (slave mode)
inline constexpr std::uint32_t kIsrNackf   = 1u << 4;   ///< NACK received
inline constexpr std::uint32_t kIsrStopf   = 1u << 5;   ///< STOP detected
inline constexpr std::uint32_t kIsrTc      = 1u << 6;   ///< Transfer complete (AUTOEND=0)
inline constexpr std::uint32_t kIsrTcr     = 1u << 7;   ///< Transfer complete reload
inline constexpr std::uint32_t kIsrBusy    = 1u << 15;  ///< Bus busy

// ICR bits (write 1 to clear corresponding ISR flag)
inline constexpr std::uint32_t kIcrNackcf  = 1u << 4;
inline constexpr std::uint32_t kIcrStopcf  = 1u << 5;

[[nodiscard]] consteval auto is_modern_i2c(const char* tmpl, const char* ip) -> bool {
    return std::string_view{tmpl} == "i2c" &&
           std::string_view{ip}.starts_with("i2c2");
}

}  // namespace detail

// ============================================================================
// Concept
// ============================================================================

/// Satisfied when P is a modern ST I2C peripheral (F3/G0/G4/H7/L4/WB).
template <typename P>
concept StModernI2c =
    device::PeripheralSpec<P> &&
    requires {
        { P::kTemplate  } -> std::convertible_to<const char*>;
        { P::kIpVersion } -> std::convertible_to<const char*>;
    } &&
    detail::is_modern_i2c(P::kTemplate, P::kIpVersion);

// ============================================================================
// Result type
// ============================================================================

/// Result of a polled I2C transfer.
enum class Status : std::uint8_t {
    Ok   = 0,  ///< Transfer completed without error.
    Nack = 1,  ///< Slave did not acknowledge (address or data NACK).
    Busy = 2,  ///< Bus was busy at the start of the transfer.
};

// ============================================================================
// Configuration
// ============================================================================

/// Configuration passed to `port::configure()`.
struct Config {
    /// Precomputed TIMINGR register value.
    ///
    /// Must match the actual peripheral clock frequency and desired I2C speed.
    /// Generate with STM32CubeMX or the timing tables in AN4235 / AN4803.
    std::uint32_t timingr{};
};

// ============================================================================
// port<P> — zero-size type, all methods static
// ============================================================================

/// Direct-MMIO I2C master port.  P must satisfy StModernI2c.
///
/// All transfers are polled (blocking).  No DMA, no interrupts.
/// Maximum NBYTES per transfer is 255; transfers larger than 255 bytes
/// are split automatically using RELOAD mode.
///
/// Slave address is always 7-bit (passed as the unshifted value, e.g.
/// address 0x48 means the 7 MS bits are 0b1001000).
template <typename P>
    requires StModernI2c<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

   private:
    [[nodiscard]] static auto r() noexcept -> volatile detail::i2c_regs& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile detail::i2c_regs*>(kBase);
    }

    // -----------------------------------------------------------------------
    // Timeout / polling helpers
    // -----------------------------------------------------------------------

    /// Spin until `flag` is set in ISR, or the NACK flag appears.
    /// Returns false on NACK (transfer should be aborted).
    [[nodiscard]] static auto wait_flag(std::uint32_t flag) noexcept -> bool {
        while (true) {
            const auto isr = r().isr;
            if ((isr & detail::kIsrNackf) != 0u) { return false; }
            if ((isr & flag) != 0u)               { return true;  }
        }
    }

    /// Send a STOP condition and clear pending flags.
    static void abort_with_stop() noexcept {
        r().cr2 |= detail::kCr2Stop;
        // Wait for STOP flag, then clear it
        while ((r().isr & detail::kIsrStopf) == 0u) { /* spin */ }
        r().icr = detail::kIcrStopcf | detail::kIcrNackcf;
        r().cr2 = 0u;
    }

    // -----------------------------------------------------------------------
    // Core transfer engine (used by write / read / write_read / probe)
    // -----------------------------------------------------------------------

    /// Write `total` bytes from `data` to slave `addr` (7-bit).
    ///
    /// Special case: `total == 0` issues START + addr + STOP with NBYTES=0
    /// (AUTOEND=1).  This probes whether the address is acknowledged without
    /// transferring any payload.  Hardware sends STOP automatically on both
    /// ACK and NACK when AUTOEND=1.
    ///
    /// Returns false on NACK.
    [[nodiscard]] static auto write_raw(
        std::uint8_t addr, const std::uint8_t* data, std::size_t total
    ) noexcept -> bool {
        // ── Zero-length probe path ────────────────────────────────────────────
        // NBYTES=0, AUTOEND=1 → START + address phase only.
        // On ACK:  STOPF is set (hardware auto-sends STOP), NACKF clear.
        // On NACK: NACKF is set, hardware auto-sends STOP, STOPF also set.
        if (total == 0u) {
            r().cr2 =
                (static_cast<std::uint32_t>(addr) << detail::kCr2SaddShift) |
                detail::kCr2Autoend |
                detail::kCr2Start;
            // Wait for STOP (set in both ACK and NACK cases with AUTOEND=1).
            while ((r().isr & detail::kIsrStopf) == 0u) { /* spin */ }
            const bool nacked = (r().isr & detail::kIsrNackf) != 0u;
            r().icr = detail::kIcrStopcf | detail::kIcrNackcf;
            r().cr2 = 0u;
            return !nacked;
        }
        // ── Normal multi-byte write path ──────────────────────────────────────
        while (total > 0u) {
            const auto chunk = static_cast<std::uint8_t>(
                total > 255u ? 255u : total);
            const bool last_chunk = (total <= 255u);

            // Set SADD, NBYTES, AUTOEND (on last chunk), START, write
            const std::uint32_t cr2 =
                (static_cast<std::uint32_t>(addr) << detail::kCr2SaddShift) |
                (static_cast<std::uint32_t>(chunk) << detail::kCr2NbytesShift) |
                (last_chunk ? detail::kCr2Autoend : detail::kCr2Reload) |
                detail::kCr2Start;

            r().cr2 = cr2;

            for (std::size_t i = 0u; i < chunk; ++i) {
                if (!wait_flag(detail::kIsrTxis)) {
                    abort_with_stop();
                    return false;
                }
                r().txdr = *data++;
            }

            if (last_chunk) {
                // Wait for STOP condition to be generated (AUTOEND)
                while ((r().isr & detail::kIsrStopf) == 0u) { /* spin */ }
                r().icr = detail::kIcrStopcf;
            } else {
                // RELOAD: wait for TCR (byte counter exhausted, no STOP)
                if (!wait_flag(detail::kIsrTcr)) {
                    abort_with_stop();
                    return false;
                }
            }
            total -= chunk;
        }
        r().cr2 = 0u;
        return true;
    }

    /// Read `total` bytes into `data` from slave `addr` (7-bit).
    /// Returns false on NACK.
    [[nodiscard]] static auto read_raw(
        std::uint8_t addr, std::uint8_t* data, std::size_t total
    ) noexcept -> bool {
        while (total > 0u) {
            const auto chunk = static_cast<std::uint8_t>(
                total > 255u ? 255u : total);
            const bool last_chunk = (total <= 255u);

            const std::uint32_t cr2 =
                (static_cast<std::uint32_t>(addr) << detail::kCr2SaddShift) |
                detail::kCr2RdWrn |
                (static_cast<std::uint32_t>(chunk) << detail::kCr2NbytesShift) |
                (last_chunk ? detail::kCr2Autoend : detail::kCr2Reload) |
                detail::kCr2Start;

            r().cr2 = cr2;

            for (std::size_t i = 0u; i < chunk; ++i) {
                if (!wait_flag(detail::kIsrRxne)) {
                    abort_with_stop();
                    return false;
                }
                *data++ = static_cast<std::uint8_t>(r().rxdr & 0xFFu);
            }

            if (last_chunk) {
                while ((r().isr & detail::kIsrStopf) == 0u) { /* spin */ }
                r().icr = detail::kIcrStopcf;
            } else {
                if (!wait_flag(detail::kIsrTcr)) {
                    abort_with_stop();
                    return false;
                }
            }
            total -= chunk;
        }
        r().cr2 = 0u;
        return true;
    }

   public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /// Enable the I2C peripheral with the given configuration.
    ///
    /// `cfg.timingr` must be precomputed for the actual peripheral clock
    /// frequency and desired I2C speed.  Use STM32CubeMX or the timing tables
    /// in the STM32 application notes (AN4235 / AN4803).
    ///
    /// @param cfg  Configuration (currently only `timingr`).
    static void configure(const Config& cfg) noexcept {
        clock_on();
        r().cr1 &= ~detail::kCr1Pe;  // disable while reconfiguring
        r().timingr = cfg.timingr;
        r().cr1 = detail::kCr1Pe;   // enable
    }

    /// Disable the peripheral (clears PE).
    static void disable() noexcept {
        r().cr1 &= ~detail::kCr1Pe;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// True when the I2C bus is busy (another master is transmitting).
    [[nodiscard]] static auto bus_busy() noexcept -> bool {
        return (r().isr & detail::kIsrBusy) != 0u;
    }

    // -----------------------------------------------------------------------
    // Master transfers — 7-bit addressing
    // -----------------------------------------------------------------------

    /// Write `buf` to slave at `addr` (7-bit, unshifted).
    ///
    /// @param addr  7-bit slave address (e.g. 0x48 for TMP102).
    /// @param buf   Data to send.
    /// @returns `Status::Ok` on success, `Status::Nack` on address/data NACK.
    [[nodiscard]] static auto write(
        std::uint8_t addr,
        std::span<const std::uint8_t> buf
    ) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }
        return write_raw(addr, buf.data(), buf.size())
            ? Status::Ok : Status::Nack;
    }

    /// Read `buf.size()` bytes from slave at `addr`.
    ///
    /// @param addr  7-bit slave address.
    /// @param buf   Buffer to receive data.
    /// @returns `Status::Ok` on success, `Status::Nack` on address NACK.
    [[nodiscard]] static auto read(
        std::uint8_t addr,
        std::span<std::uint8_t> buf
    ) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }
        return read_raw(addr, buf.data(), buf.size())
            ? Status::Ok : Status::Nack;
    }

    /// Write `tx`, then issue a repeated START and read `rx.size()` bytes.
    ///
    /// Common pattern for register-read operations: write a register address
    /// then read the value without releasing the bus.
    ///
    /// @param addr  7-bit slave address.
    /// @param tx    Data to write (e.g. register address).
    /// @param rx    Buffer to receive the register value.
    /// @returns `Status::Ok` on success, `Status::Nack` on any NACK.
    [[nodiscard]] static auto write_read(
        std::uint8_t addr,
        std::span<const std::uint8_t> tx,
        std::span<std::uint8_t>       rx
    ) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }
        // Write without AUTOEND so we get TC instead of STOP.
        const auto chunk_tx = static_cast<std::uint8_t>(
            tx.size() > 255u ? 255u : tx.size());
        const std::uint32_t cr2_w =
            (static_cast<std::uint32_t>(addr) << detail::kCr2SaddShift) |
            (static_cast<std::uint32_t>(chunk_tx) << detail::kCr2NbytesShift) |
            detail::kCr2Start;  // no AUTOEND → TC on completion

        r().cr2 = cr2_w;
        for (std::size_t i = 0u; i < chunk_tx; ++i) {
            if (!wait_flag(detail::kIsrTxis)) {
                abort_with_stop();
                return Status::Nack;
            }
            r().txdr = tx[i];
        }
        // Wait for TC (transfer complete, bus held)
        if (!wait_flag(detail::kIsrTc)) {
            abort_with_stop();
            return Status::Nack;
        }

        // Repeated START → read phase
        return read_raw(addr, rx.data(), rx.size())
            ? Status::Ok : Status::Nack;
    }

    /// Probe whether a slave device is present at `addr` (7-bit).
    ///
    /// Issues START + addr + STOP with NBYTES=0.  Returns `Status::Ok` if the
    /// device acknowledges its address, `Status::Nack` if no device is present.
    ///
    /// Preferred over `write(addr, {})` for bus scanning — makes intent clear
    /// and the 0-byte path is explicitly handled in hardware (AUTOEND=1).
    ///
    /// @param addr  7-bit slave address to probe (e.g. 0x48 for TMP102).
    [[nodiscard]] static auto probe(std::uint8_t addr) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }
        return write_raw(addr, nullptr, 0u) ? Status::Ok : Status::Nack;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// True when the peripheral is enabled (CR1.PE = 1).
    [[nodiscard]] static auto enabled() noexcept -> bool {
        return (r().cr1 & detail::kCr1Pe) != 0u;
    }

    // -----------------------------------------------------------------------
    // Interrupt control
    // -----------------------------------------------------------------------

    /// Enable the TXIS (transmit data register empty) interrupt.
    static void enable_tx_irq() noexcept {
        r().cr1 |= detail::kCr1Txie;
    }

    /// Enable the RXNE (receive data register not empty) interrupt.
    static void enable_rx_irq() noexcept {
        r().cr1 |= detail::kCr1Rxie;
    }

    /// Enable the NACK-received interrupt.
    static void enable_nack_irq() noexcept {
        r().cr1 |= detail::kCr1Nackie;
    }

    /// Enable the STOP-detected interrupt.
    static void enable_stop_irq() noexcept {
        r().cr1 |= detail::kCr1Stopie;
    }

    /// Enable the transfer-complete (TC / TCR) interrupt.
    ///
    /// Fires when NBYTES is transferred and AUTOEND=0 (TC) or RELOAD=1 (TCR).
    /// Useful for ISR-driven multi-byte transfers without polling.
    static void enable_tc_irq() noexcept {
        r().cr1 |= detail::kCr1Tcie;
    }

    /// Enable error interrupts (BERR, ARLO, OVR → ERRIE in CR1).
    static void enable_error_irq() noexcept {
        r().cr1 |= detail::kCr1Errie;
    }

    /// Disable all I2C interrupt sources.
    static void disable_irqs() noexcept {
        constexpr std::uint32_t kMask =
            detail::kCr1Txie   | detail::kCr1Rxie   | detail::kCr1Addrie |
            detail::kCr1Nackie | detail::kCr1Stopie  | detail::kCr1Tcie   |
            detail::kCr1Errie;
        r().cr1 &= ~kMask;
    }

    // -----------------------------------------------------------------------
    // 10-bit addressing — private helpers
    // -----------------------------------------------------------------------

    /// Write `total` bytes to a 10-bit slave address.
    [[nodiscard]] static auto write10_raw(
        std::uint16_t addr, const std::uint8_t* data, std::size_t total
    ) noexcept -> bool {
        if (total == 0u) {
            // Zero-length probe: START + 10-bit header + 2nd addr byte + STOP
            r().cr2 =
                (static_cast<std::uint32_t>(addr & 0x3FFu)) |
                detail::kCr2Add10     |
                detail::kCr2Autoend   |
                detail::kCr2Start;
            while ((r().isr & detail::kIsrStopf) == 0u) { /* spin */ }
            const bool nacked = (r().isr & detail::kIsrNackf) != 0u;
            r().icr = detail::kIcrStopcf | detail::kIcrNackcf;
            r().cr2 = 0u;
            return !nacked;
        }
        while (total > 0u) {
            const auto chunk     = static_cast<std::uint8_t>(total > 255u ? 255u : total);
            const bool last_chunk = (total <= 255u);

            r().cr2 =
                (static_cast<std::uint32_t>(addr & 0x3FFu)) |
                detail::kCr2Add10 |
                (static_cast<std::uint32_t>(chunk) << detail::kCr2NbytesShift) |
                (last_chunk ? detail::kCr2Autoend : detail::kCr2Reload) |
                detail::kCr2Start;

            for (std::size_t i = 0u; i < chunk; ++i) {
                if (!wait_flag(detail::kIsrTxis)) {
                    abort_with_stop();
                    return false;
                }
                r().txdr = *data++;
            }
            if (last_chunk) {
                while ((r().isr & detail::kIsrStopf) == 0u) { /* spin */ }
                r().icr = detail::kIcrStopcf;
            } else {
                if (!wait_flag(detail::kIsrTcr)) { abort_with_stop(); return false; }
            }
            total -= chunk;
        }
        r().cr2 = 0u;
        return true;
    }

    /// Read `total` bytes from a 10-bit slave address.
    ///
    /// HEAD10R=0 → the full 10-bit address sequence is sent before reading
    /// (START + 11110xx0 + addr2 + RESTART + 11110xx1).
    [[nodiscard]] static auto read10_raw(
        std::uint16_t addr, std::uint8_t* data, std::size_t total
    ) noexcept -> bool {
        while (total > 0u) {
            const auto chunk     = static_cast<std::uint8_t>(total > 255u ? 255u : total);
            const bool last_chunk = (total <= 255u);

            r().cr2 =
                (static_cast<std::uint32_t>(addr & 0x3FFu)) |
                detail::kCr2Add10   |
                detail::kCr2RdWrn   |
                // HEAD10R=0: send full 10-bit addr on every (re)start
                (static_cast<std::uint32_t>(chunk) << detail::kCr2NbytesShift) |
                (last_chunk ? detail::kCr2Autoend : detail::kCr2Reload) |
                detail::kCr2Start;

            for (std::size_t i = 0u; i < chunk; ++i) {
                if (!wait_flag(detail::kIsrRxne)) {
                    abort_with_stop();
                    return false;
                }
                *data++ = static_cast<std::uint8_t>(r().rxdr & 0xFFu);
            }
            if (last_chunk) {
                while ((r().isr & detail::kIsrStopf) == 0u) { /* spin */ }
                r().icr = detail::kIcrStopcf;
            } else {
                if (!wait_flag(detail::kIsrTcr)) { abort_with_stop(); return false; }
            }
            total -= chunk;
        }
        r().cr2 = 0u;
        return true;
    }

   public:
    // -----------------------------------------------------------------------
    // Master transfers — 10-bit addressing
    // -----------------------------------------------------------------------

    /// Write `buf` to a 10-bit addressed slave.
    ///
    /// @param addr10  Full 10-bit slave address (0x000–0x3FF).
    /// @param buf     Data to send.
    [[nodiscard]] static auto write(
        std::uint16_t addr10,
        std::span<const std::uint8_t> buf
    ) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }
        return write10_raw(addr10, buf.data(), buf.size())
            ? Status::Ok : Status::Nack;
    }

    /// Read into `buf` from a 10-bit addressed slave.
    ///
    /// @param addr10  Full 10-bit slave address.
    /// @param buf     Buffer to fill.
    [[nodiscard]] static auto read(
        std::uint16_t addr10,
        std::span<std::uint8_t> buf
    ) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }
        return read10_raw(addr10, buf.data(), buf.size())
            ? Status::Ok : Status::Nack;
    }

    /// Write then repeated-START read to a 10-bit addressed slave.
    ///
    /// Identical semantics to the 7-bit overload: sends `tx` without STOP,
    /// then issues a repeated START and reads `rx.size()` bytes.
    ///
    /// @param addr10  Full 10-bit slave address.
    /// @param tx      Register address / command bytes.
    /// @param rx      Buffer for response.
    [[nodiscard]] static auto write_read(
        std::uint16_t addr10,
        std::span<const std::uint8_t> tx,
        std::span<std::uint8_t>       rx
    ) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }

        const auto chunk_tx = static_cast<std::uint8_t>(
            tx.size() > 255u ? 255u : tx.size());
        r().cr2 =
            (static_cast<std::uint32_t>(addr10 & 0x3FFu)) |
            detail::kCr2Add10 |
            (static_cast<std::uint32_t>(chunk_tx) << detail::kCr2NbytesShift) |
            detail::kCr2Start;  // no AUTOEND → get TC

        for (std::size_t i = 0u; i < chunk_tx; ++i) {
            if (!wait_flag(detail::kIsrTxis)) {
                abort_with_stop();
                return Status::Nack;
            }
            r().txdr = tx[i];
        }
        if (!wait_flag(detail::kIsrTc)) {
            abort_with_stop();
            return Status::Nack;
        }

        return read10_raw(addr10, rx.data(), rx.size())
            ? Status::Ok : Status::Nack;
    }

    /// Probe a 10-bit slave address (zero-byte write).
    ///
    /// @param addr10  Full 10-bit slave address to probe.
    [[nodiscard]] static auto probe(std::uint16_t addr10) noexcept -> Status {
        if (bus_busy()) { return Status::Busy; }
        return write10_raw(addr10, nullptr, 0u) ? Status::Ok : Status::Nack;
    }

    // -----------------------------------------------------------------------
    // Device-data bridge — sourced from alloy.device.v2.1 flat-struct
    // -----------------------------------------------------------------------

    /// Returns the NVIC IRQ line for this peripheral.
    /// Sourced from `P::kIrqLines[idx]` (flat-struct v2.1).
    /// Compiles to a constexpr constant — zero overhead.
    [[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
        -> std::uint32_t {
        if constexpr (requires { P::kIrqLines[0]; }) {
            return static_cast<std::uint32_t>(P::kIrqLines[idx]);
        } else {
            static_assert(sizeof(P) == 0,
                "P::kIrqLines not present; upgrade device artifact to v2.1");
            return 0u;
        }
    }

    /// Returns the number of IRQ lines for this peripheral.
    /// Sourced from `P::kIrqCount` (flat-struct v2.1). Returns 0 if absent.
    [[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t {
        if constexpr (requires { P::kIrqCount; }) {
            return static_cast<std::size_t>(P::kIrqCount);
        }
        return 0u;
    }

    // -----------------------------------------------------------------------
    // Clock gate — sourced from alloy.device.v2.1 flat-struct kRccEnable
    // -----------------------------------------------------------------------

    /// Enable the peripheral clock and deassert reset (if kRccReset present).
    ///
    /// Uses the typed `P::kRccEnable = { addr, mask }` emitted by alloy-codegen v0.4+.
    /// No-op when the peripheral has no kRccEnable field.
    static void clock_on() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) |= P::kRccEnable.mask;
        }
        if constexpr (requires { P::kRccReset; }) {
            // Assert then release reset so the peripheral starts from a known state.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) |=  P::kRccReset.mask;
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccReset.addr) &= ~P::kRccReset.mask;
        }
    }

    /// Disable the peripheral clock.
    static void clock_off() noexcept {
        if constexpr (requires { P::kRccEnable; }) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            *reinterpret_cast<volatile std::uint32_t*>(P::kRccEnable.addr) &= ~P::kRccEnable.mask;
        }
    }

    port() = delete;
};

}  // namespace alloy::hal::i2c::lite
