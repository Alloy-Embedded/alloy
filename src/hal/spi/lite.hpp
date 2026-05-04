/// @file hal/spi/lite.hpp
/// Lightweight, direct-MMIO SPI master driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
/// Works whenever `ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE` is true.
///
/// Supports:
///   - ST SPI v2 (F0/F3/F4 and equivalents): kIpVersion starts with "spi2s1_v2"
///   - ST SPI v3+ (G0/G4/H7/WB/L4): kIpVersion starts with "spi2s1_v3"
///     or "spi2s1_v4" — has 4-entry FIFO and DS[3:0] field in CR2.
///
/// Register layout (both variants share CR1/CR2/SR/DR at the same offsets):
///   0x00 CR1  — master config: SPE, BR, CPOL, CPHA, SSM, SSI, DFF, LSBFIRST
///   0x04 CR2  — frame size (DS) / FIFO thresholds (v3+), SSOE, NSSP
///   0x08 SR   — status: RXNE, TXE, BSY, OVR, MODF, FRLVL/FTLVL (v3+)
///   0x0C DR   — data register (8-bit or 16-bit depending on frame size)
///
/// IMPORTANT: the caller must enable the peripheral clock (and deassert reset)
/// before calling `configure()`.  With alloy-codegen v2.1:
/// @code
///   namespace dev = alloy::device::traits;
///   dev::peripheral_on<dev::spi1>();   // clk + rst
/// @endcode
///
/// Typical usage (SPI1 on STM32G071, 8-bit, Mode 0, div/8):
/// @code
///   #include "hal/spi.hpp"
///   #include "device/runtime.hpp"
///
///   namespace dev = alloy::device::traits;
///   using Spi1 = alloy::hal::spi::lite::port<dev::spi1>;
///
///   dev::peripheral_on<dev::spi1>();
///   Spi1::configure({});               // defaults: Mode0, div/8, 8-bit MSB
///   uint8_t rx = Spi1::transfer(0xAB);
/// @endcode
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::spi::lite {

// ============================================================================
// Detail — register layout and compile-time helpers
// ============================================================================

namespace detail {

/// STM32 SPI register map (applies to all F0/F3/F4/G0/G4/H7 — same offsets).
struct spi_regs {
    std::uint32_t cr1;   ///< 0x00 — control register 1
    std::uint32_t cr2;   ///< 0x04 — control register 2
    std::uint32_t sr;    ///< 0x08 — status register (read-only for most bits)
    std::uint32_t dr;    ///< 0x0C — data register (8 or 16-bit transfer)
};

// CR1 bits (both variants)
inline constexpr std::uint32_t kCr1Cpha     = 1u << 0;   ///< Clock phase
inline constexpr std::uint32_t kCr1Cpol     = 1u << 1;   ///< Clock polarity
inline constexpr std::uint32_t kCr1Mstr     = 1u << 2;   ///< Master selection
inline constexpr std::uint32_t kCr1BrShift  = 3u;        ///< BR[2:0] LSB position
inline constexpr std::uint32_t kCr1BrMask   = 0x7u << 3; ///< Baud rate divisor mask
inline constexpr std::uint32_t kCr1Spe      = 1u << 6;   ///< SPI enable
inline constexpr std::uint32_t kCr1Lsbfirst = 1u << 7;   ///< LSB-first frame format
inline constexpr std::uint32_t kCr1Ssi      = 1u << 8;   ///< Internal slave select
inline constexpr std::uint32_t kCr1Ssm      = 1u << 9;   ///< Software slave management
inline constexpr std::uint32_t kCr1Dff      = 1u << 11;  ///< 16-bit frame (legacy: v2)

// CR2 bits (v3+, FIFO-capable variants)
inline constexpr std::uint32_t kCr2Ssoe     = 1u << 2;   ///< SS output enable
inline constexpr std::uint32_t kCr2Nssp     = 1u << 3;   ///< NSS pulse management
inline constexpr std::uint32_t kCr2DsShift  = 8u;        ///< DS[3:0] LSB in CR2
inline constexpr std::uint32_t kCr2DsMask   = 0xFu << 8; ///< Data size mask
inline constexpr std::uint32_t kCr2Frxth    = 1u << 12;  ///< FIFO reception threshold

// SR bits (both variants)
inline constexpr std::uint32_t kSrRxne      = 1u << 0;   ///< Receive buffer not empty
inline constexpr std::uint32_t kSrTxe       = 1u << 1;   ///< Transmit buffer empty
inline constexpr std::uint32_t kSrModf      = 1u << 5;   ///< Mode fault
inline constexpr std::uint32_t kSrOvr       = 1u << 6;   ///< Overrun flag
inline constexpr std::uint32_t kSrBsy       = 1u << 7;   ///< Busy flag

// CR2 interrupt-enable bits (both variants)
inline constexpr std::uint32_t kCr2Errie    = 1u << 5;   ///< Error interrupt enable (OVR/MODF/CRCERR)
inline constexpr std::uint32_t kCr2Rxneie   = 1u << 6;   ///< RXNE interrupt enable
inline constexpr std::uint32_t kCr2Txeie    = 1u << 7;   ///< TXE interrupt enable

/// True for FIFO-capable SPI variants (spi2s1_v3*, spi2s1_v4*).
/// These have DS[3:0] in CR2 and an FRXTH threshold bit.
[[nodiscard]] consteval auto is_fifo_spi(const char* ip) -> bool {
    const std::string_view sv{ip};
    return sv.starts_with("spi2s1_v3") || sv.starts_with("spi2s1_v4");
}

[[nodiscard]] consteval auto is_st_spi(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "spi";
}

}  // namespace detail

// ============================================================================
// Enumerations
// ============================================================================

/// SPI clock baud-rate prescaler (divides the peripheral kernel clock).
enum class BaudDiv : std::uint8_t {
    Div2   = 0,
    Div4   = 1,
    Div8   = 2,
    Div16  = 3,
    Div32  = 4,
    Div64  = 5,
    Div128 = 6,
    Div256 = 7,
};

/// SPI clock polarity (CPOL).
enum class Polarity : std::uint8_t {
    Low  = 0,   ///< Clock idles low  (CPOL=0)
    High = 1,   ///< Clock idles high (CPOL=1)
};

/// SPI clock phase (CPHA).
enum class Phase : std::uint8_t {
    Edge1 = 0,  ///< Data captured on first  edge (CPHA=0)
    Edge2 = 1,  ///< Data captured on second edge (CPHA=1)
};

// ============================================================================
// Concept
// ============================================================================

/// Satisfied when P is an ST SPI peripheral (any STM32 family).
/// Requires both `kTemplate` (for is_st_spi) and `kIpVersion` (for FIFO
/// detection inside `port<P>::kFifo`).
template <typename P>
concept StSpi =
    device::PeripheralSpec<P> &&
    requires {
        { P::kTemplate   } -> std::convertible_to<const char*>;
        { P::kIpVersion  } -> std::convertible_to<const char*>;
    } &&
    detail::is_st_spi(P::kTemplate);

// ============================================================================
// Configuration
// ============================================================================

/// Configuration for `port::configure()`.
struct Config {
    BaudDiv  baud_div  = BaudDiv::Div8;    ///< Clock prescaler (kernel_clk / div).
    Polarity cpol      = Polarity::Low;    ///< Clock polarity.
    Phase    cpha      = Phase::Edge1;     ///< Clock phase.
    bool     lsb_first = false;            ///< Send LSB first (default: MSB first).
    bool     frame_16  = false;            ///< 16-bit frame (default: 8-bit).
};

// ============================================================================
// port<P> — zero-size type, all methods static
// ============================================================================

/// Direct-MMIO SPI master port.  P must satisfy StSpi.
///
/// All methods are static — obtain a "handle" by naming the type:
/// @code
///   using Spi1 = alloy::hal::spi::lite::port<dev::spi1>;
///   Spi1::configure({.baud_div = BaudDiv::Div16});
///   uint8_t rx = Spi1::transfer(0x9F);
/// @endcode
///
/// CS (chip select) is intentionally left to the caller — use GPIO
/// `set_low` / `set_high` before and after transfers.
template <typename P>
    requires StSpi<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

    /// True when this peripheral has a hardware FIFO (spi2s1_v3/v4).
    static constexpr bool kFifo = detail::is_fifo_spi(P::kIpVersion);

   private:
    [[nodiscard]] static auto r() noexcept -> volatile detail::spi_regs& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile detail::spi_regs*>(kBase);
    }

    /// 8-bit pointer into DR for FIFO-mode writes (forces byte-width access
    /// so only one byte is pushed into the FIFO at a time).
    [[nodiscard]] static auto dr8() noexcept -> volatile std::uint8_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint8_t*>(kBase + 0x0Cu);
    }

    /// 16-bit pointer into DR for 16-bit frame transfers.
    [[nodiscard]] static auto dr16() noexcept -> volatile std::uint16_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint16_t*>(kBase + 0x0Cu);
    }

   public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /// Configure and enable the SPI peripheral.
    /// Disables SPE first, writes CR1/CR2, re-enables.
    static void configure(const Config& cfg = {}) noexcept {
        clock_on();
        // Disable while reconfiguring
        r().cr1 &= ~detail::kCr1Spe;

        // Build CR1
        std::uint32_t cr1 =
            detail::kCr1Mstr |                             // master mode
            detail::kCr1Ssm  |                             // software NSS
            detail::kCr1Ssi  |                             // NSS high internally
            (static_cast<std::uint32_t>(cfg.baud_div) << detail::kCr1BrShift);

        if (cfg.cpol == Polarity::High)  { cr1 |= detail::kCr1Cpol; }
        if (cfg.cpha == Phase::Edge2)    { cr1 |= detail::kCr1Cpha; }
        if (cfg.lsb_first)               { cr1 |= detail::kCr1Lsbfirst; }

        // Build CR2 (data size)
        std::uint32_t cr2 = 0u;
        if constexpr (kFifo) {
            // v3+: DS[3:0] selects frame width.  0b0111 = 8 bits, 0b1111 = 16 bits.
            const std::uint32_t ds = cfg.frame_16 ? 0xFu : 0x7u;
            cr2 = (ds << detail::kCr2DsShift);
            // FRXTH must be 1 for 8-bit reception (RXNE triggers at ≥ 1 byte).
            if (!cfg.frame_16) { cr2 |= detail::kCr2Frxth; }
        } else {
            // v2 / legacy: DFF bit in CR1.
            if (cfg.frame_16) { cr1 |= detail::kCr1Dff; }
        }

        r().cr2 = cr2;
        r().cr1 = cr1 | detail::kCr1Spe;  // enable in same write
    }

    /// Disable the peripheral (waits for BSY to clear first).
    static void disable() noexcept {
        while (busy()) { /* drain */ }
        r().cr1 &= ~detail::kCr1Spe;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// True when the transmit data register / FIFO slot is empty.
    [[nodiscard]] static auto tx_empty() noexcept -> bool {
        return (r().sr & detail::kSrTxe) != 0u;
    }

    /// True when the receive buffer / FIFO holds unread data.
    [[nodiscard]] static auto rx_not_empty() noexcept -> bool {
        return (r().sr & detail::kSrRxne) != 0u;
    }

    /// True while the SPI bus is transferring data.
    [[nodiscard]] static auto busy() noexcept -> bool {
        return (r().sr & detail::kSrBsy) != 0u;
    }

    // -----------------------------------------------------------------------
    // Single-byte transfers
    // -----------------------------------------------------------------------

    /// Write one byte (spin until TXE).  Does not read the received byte.
    static void write_byte(std::uint8_t b) noexcept {
        while (!tx_empty()) { /* spin */ }
        if constexpr (kFifo) {
            dr8() = b;  // byte-width write — avoids spurious FIFO fill
        } else {
            r().dr = b;
        }
    }

    /// Read one byte (spin until RXNE).
    [[nodiscard]] static auto read_byte() noexcept -> std::uint8_t {
        while (!rx_not_empty()) { /* spin */ }
        return static_cast<std::uint8_t>(r().dr & 0xFFu);
    }

    /// Simultaneously write `b` and return the received byte (full-duplex).
    [[nodiscard]] static auto transfer(std::uint8_t b) noexcept -> std::uint8_t {
        write_byte(b);
        return read_byte();
    }

    // -----------------------------------------------------------------------
    // Buffer transfers
    // -----------------------------------------------------------------------

    /// Write a buffer; discard received bytes.
    static void write(std::span<const std::uint8_t> buf) noexcept {
        for (const auto b : buf) {
            write_byte(b);
            (void)read_byte();  // drain RX to prevent overrun
        }
    }

    /// Read `buf.size()` bytes; send `fill` during each byte.
    static void read(std::span<std::uint8_t> buf,
                     std::uint8_t fill = 0xFFu) noexcept {
        for (auto& b : buf) {
            b = transfer(fill);
        }
    }

    /// Full-duplex: transmit `tx`, receive into `rx`.
    /// `tx` and `rx` must have the same size.
    static void transfer(std::span<const std::uint8_t> tx,
                         std::span<std::uint8_t>       rx) noexcept {
        const std::size_t n = tx.size() < rx.size() ? tx.size() : rx.size();
        for (std::size_t i = 0u; i < n; ++i) {
            rx[i] = transfer(tx[i]);
        }
    }

    /// Spin until the SPI bus is idle (all bytes shifted out).
    static void flush() noexcept {
        while (busy()) { /* spin */ }
    }

    // -----------------------------------------------------------------------
    // 16-bit single transfer
    // -----------------------------------------------------------------------

    /// Simultaneously write a 16-bit word and return the received word.
    ///
    /// The peripheral must be configured with `frame_16 = true`.
    /// On FIFO variants (v3+) the 16-bit DR pointer is used to push a full
    /// word in one write; on legacy v2 the DR register receives the 16-bit
    /// value directly.
    [[nodiscard]] static auto transfer16(std::uint16_t val) noexcept -> std::uint16_t {
        while (!tx_empty()) { /* spin */ }
        if constexpr (kFifo) {
            dr16() = val;
        } else {
            r().dr = val;
        }
        while (!rx_not_empty()) { /* spin */ }
        return static_cast<std::uint16_t>(r().dr & 0xFFFFu);
    }

    // -----------------------------------------------------------------------
    // Error status
    // -----------------------------------------------------------------------

    /// True if the RX data register was overrun (OVR flag in SR).
    [[nodiscard]] static auto overrun() noexcept -> bool {
        return (r().sr & detail::kSrOvr) != 0u;
    }

    /// True if a mode fault occurred (MODF flag) — NSS went low unexpectedly
    /// while the peripheral was configured as master.
    [[nodiscard]] static auto mode_fault() noexcept -> bool {
        return (r().sr & detail::kSrModf) != 0u;
    }

    /// Clear the OVR (overrun) flag.
    ///
    /// Hardware requires reading DR then SR to clear OVR — no ICR register.
    /// Note: this discards the overrun byte from the FIFO/DR.
    static void clear_overrun() noexcept {
        (void)r().dr;
        (void)r().sr;
    }

    // -----------------------------------------------------------------------
    // Interrupt control
    // -----------------------------------------------------------------------

    /// Enable the RXNE (receive buffer not empty) interrupt.
    static void enable_rx_irq() noexcept {
        r().cr2 |= detail::kCr2Rxneie;
    }

    /// Enable the TXE (transmit buffer empty) interrupt.
    static void enable_tx_irq() noexcept {
        r().cr2 |= detail::kCr2Txeie;
    }

    /// Enable error interrupts (OVR, MODF, CRCERR → ERRIE in CR2).
    static void enable_error_irq() noexcept {
        r().cr2 |= detail::kCr2Errie;
    }

    /// Disable all SPI interrupt sources (RXNEIE, TXEIE, ERRIE).
    static void disable_irqs() noexcept {
        r().cr2 &= ~(detail::kCr2Rxneie | detail::kCr2Txeie | detail::kCr2Errie);
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

}  // namespace alloy::hal::spi::lite
