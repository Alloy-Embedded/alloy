/// @file hal/uart/lite.hpp
/// Lightweight, direct-MMIO UART driver for alloy.device.v2.1 peripherals.
///
/// No dependency on the legacy descriptor-runtime (alloy-devices).
/// Works whenever `ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE` is true.
///
/// Supports:
///   - ST SCI3 (STM32G0 / G4 / H7 / WB): kIpVersion starts with "sci3"
///   - ST SCI2 (STM32F1 / F2 / F3 / F4): kIpVersion starts with "sci2"
///
/// Typical usage:
/// @code
///   #include "hal/uart/lite.hpp"
///   #include "device/runtime.hpp"   // for alloy::device::traits
///
///   using namespace alloy::device;
///   using Uart1 = hal::uart::lite::port<traits::usart1>;
///
///   Uart1::configure({.baudrate = 115200, .clock_hz = 16'000'000u});
///   Uart1::write(std::as_bytes(std::span{"Hello\n"}));
/// @endcode
///
/// All methods are static — `port<P>` is a zero-size tag type, never instantiated.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "device/concepts.hpp"

namespace alloy::hal::uart::lite {

// ============================================================================
// Detail — register layouts and compile-time helpers
// ============================================================================

namespace detail {

/// ST SCI3 (modern: STM32G0/G4/H7) — unified register map starting at base.
struct sci3_regs {
    std::uint32_t cr1;   ///< 0x00 — control register 1
    std::uint32_t cr2;   ///< 0x04 — control register 2
    std::uint32_t cr3;   ///< 0x08 — control register 3
    std::uint32_t brr;   ///< 0x0C — baud rate register
    std::uint32_t gtpr;  ///< 0x10 — guard time / prescaler
    std::uint32_t rtor;  ///< 0x14 — receiver timeout
    std::uint32_t rqr;   ///< 0x18 — request register
    std::uint32_t isr;   ///< 0x1C — interrupt and status register (read-only)
    std::uint32_t icr;   ///< 0x20 — interrupt flag clear register (write-1-to-clear)
    std::uint32_t rdr;   ///< 0x24 — receive data register
    std::uint32_t tdr;   ///< 0x28 — transmit data register
};

/// ST SCI2 (legacy: STM32F1/F2/F3/F4) — unified register map starting at base.
struct sci2_regs {
    std::uint32_t sr;    ///< 0x00 — status register
    std::uint32_t dr;    ///< 0x04 — data register (RW — reads RDR, writes TDR)
    std::uint32_t brr;   ///< 0x08 — baud rate register
    std::uint32_t cr1;   ///< 0x0C — control register 1
    std::uint32_t cr2;   ///< 0x10 — control register 2
    std::uint32_t cr3;   ///< 0x14 — control register 3
    std::uint32_t gtpr;  ///< 0x18 — guard time / prescaler
};

// IP-version detection helpers — consteval so they fold at compile time.

[[nodiscard]] consteval auto ip_starts_with(const char* ip, std::string_view prefix) -> bool {
    return std::string_view{ip}.starts_with(prefix);
}

[[nodiscard]] consteval auto is_sci3(const char* ip) -> bool {
    return ip_starts_with(ip, "sci3");
}

[[nodiscard]] consteval auto is_sci2(const char* ip) -> bool {
    return ip_starts_with(ip, "sci2");
}

[[nodiscard]] consteval auto is_st_usart(const char* tmpl, const char* ip) -> bool {
    return std::string_view{tmpl} == "usart" && (is_sci3(ip) || is_sci2(ip));
}

// CR1 bit positions (modern SCI3)
inline constexpr std::uint32_t kSci3Cr1Ue   = 1u << 0;   ///< USART enable
inline constexpr std::uint32_t kSci3Cr1Re   = 1u << 2;   ///< Receiver enable
inline constexpr std::uint32_t kSci3Cr1Te   = 1u << 3;   ///< Transmitter enable
inline constexpr std::uint32_t kSci3Cr1Ps   = 1u << 9;   ///< Parity selection (0=even)
inline constexpr std::uint32_t kSci3Cr1Pce  = 1u << 10;  ///< Parity control enable
inline constexpr std::uint32_t kSci3Cr1M0   = 1u << 12;  ///< Word length bit 0
inline constexpr std::uint32_t kSci3Cr1M1   = 1u << 28;  ///< Word length bit 1

// CR2 bits
inline constexpr std::uint32_t kCr2StopMask = 0x3u << 12;  ///< STOP[13:12]
// ISR / SR status bits (same offsets for ISR in sci3 and SR in sci2)
inline constexpr std::uint32_t kRxne = 1u << 5;   ///< RX not empty
inline constexpr std::uint32_t kTc   = 1u << 6;   ///< Transmission complete
inline constexpr std::uint32_t kTxe  = 1u << 7;   ///< TX empty (or TXFNF)

// CR1 bit positions (legacy SCI2 — slightly different from sci3)
inline constexpr std::uint32_t kSci2Cr1Ue   = 1u << 13;
inline constexpr std::uint32_t kSci2Cr1Re   = 1u << 2;
inline constexpr std::uint32_t kSci2Cr1Te   = 1u << 3;
inline constexpr std::uint32_t kSci2Cr1M    = 1u << 12;  ///< Word length (sci2 uses M not M0/M1)
inline constexpr std::uint32_t kSci2Cr1Pce  = 1u << 10;
inline constexpr std::uint32_t kSci2Cr1Ps   = 1u << 9;

}  // namespace detail

// ============================================================================
// Concepts
// ============================================================================

/// Satisfied when P is an ST SCI3 USART peripheral (modern: G0/G4/H7).
template <typename P>
concept StModernUsart =
    device::PeripheralSpec<P> &&
    detail::is_sci3(P::kIpVersion) &&
    std::string_view{P::kTemplate} == "usart";

/// Satisfied when P is an ST SCI2 USART peripheral (legacy: F1/F2/F4).
template <typename P>
concept StLegacyUsart =
    device::PeripheralSpec<P> &&
    detail::is_sci2(P::kIpVersion) &&
    std::string_view{P::kTemplate} == "usart";

/// Any ST USART — modern or legacy.
template <typename P>
concept StUsart = StModernUsart<P> || StLegacyUsart<P>;

// ============================================================================
// Configuration
// ============================================================================

/// Parity selection.
enum class Parity : std::uint8_t { None, Even, Odd };

/// Stop-bit count.
enum class StopBits : std::uint8_t { One, Two };

/// Configuration struct for port::configure().
struct Config {
    std::uint32_t baudrate  = 115200u;     ///< Requested baud rate in bps.
    std::uint32_t clock_hz  = 0u;          ///< Peripheral kernel clock in Hz (mandatory).
    Parity        parity    = Parity::None;
    StopBits      stop_bits = StopBits::One;
    bool          rx_enable = true;
    bool          tx_enable = true;
};

// ============================================================================
// port<P> — zero-size type, all methods static
// ============================================================================

/// Direct-MMIO UART port.  P must satisfy StUsart (ST SCI3 or SCI2).
///
/// All methods are static — obtain a "handle" simply by naming the type:
/// @code
///   using Uart1 = alloy::hal::uart::lite::port<traits::usart1>;
///   Uart1::configure({.baudrate = 115200, .clock_hz = 16'000'000});
///   Uart1::write_byte(std::byte{'X'});
/// @endcode
template <typename P>
    requires StUsart<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

    // -----------------------------------------------------------------------
    // Register access helpers — cast base address to the appropriate overlay
    // -----------------------------------------------------------------------
   private:
    [[nodiscard]] static auto sci3() noexcept -> volatile detail::sci3_regs& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile detail::sci3_regs*>(kBase);
    }

    [[nodiscard]] static auto sci2() noexcept -> volatile detail::sci2_regs& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile detail::sci2_regs*>(kBase);
    }

    // -----------------------------------------------------------------------
    // Private implementation helpers
    // -----------------------------------------------------------------------

    static void configure_modern(const Config& cfg) {
        auto& r = sci3();

        // 1. Disable USART while reconfiguring
        r.cr1 = 0u;

        // 2. BRR = clock / baudrate (16x oversampling, rounded)
        const auto div = (cfg.clock_hz + cfg.baudrate / 2u) / cfg.baudrate;
        r.brr = div & 0xFFFFu;

        // 3. STOP bits in CR2[13:12]: 0b00=1, 0b10=2
        r.cr2 = (cfg.stop_bits == StopBits::Two) ? (0x2u << 12u) : 0u;

        // 4. CR3 — default (no flow control, no DMA)
        r.cr3 = 0u;

        // 5. CR1: parity + word length + RE/TE + UE
        std::uint32_t cr1 = detail::kSci3Cr1Ue;
        if (cfg.rx_enable) { cr1 |= detail::kSci3Cr1Re; }
        if (cfg.tx_enable) { cr1 |= detail::kSci3Cr1Te; }
        if (cfg.parity != Parity::None) {
            cr1 |= detail::kSci3Cr1Pce;
            if (cfg.parity == Parity::Odd) { cr1 |= detail::kSci3Cr1Ps; }
            // 9-bit frame when parity enabled (M0=1, M1=0)
            cr1 |= detail::kSci3Cr1M0;
        }
        r.cr1 = cr1;
    }

    static void configure_legacy(const Config& cfg) {
        auto& r = sci2();

        // 1. Disable USART
        r.cr1 = 0u;

        // 2. BRR
        const auto div = (cfg.clock_hz + cfg.baudrate / 2u) / cfg.baudrate;
        // Legacy SCI2 BRR: mantissa in [15:4], fraction in [3:0]
        // For 16x oversampling: mantissa = div >> 4, fraction = div & 0xF
        r.brr = div & 0xFFFFu;

        // 3. STOP bits
        r.cr2 = (cfg.stop_bits == StopBits::Two) ? (0x2u << 12u) : 0u;

        // 4. CR3
        r.cr3 = 0u;

        // 5. CR1: UE at bit 13 (different from sci3!)
        std::uint32_t cr1 = detail::kSci2Cr1Ue;
        if (cfg.rx_enable) { cr1 |= detail::kSci2Cr1Re; }
        if (cfg.tx_enable) { cr1 |= detail::kSci2Cr1Te; }
        if (cfg.parity != Parity::None) {
            cr1 |= detail::kSci2Cr1Pce;
            if (cfg.parity == Parity::Odd) { cr1 |= detail::kSci2Cr1Ps; }
            cr1 |= detail::kSci2Cr1M;  // 9-bit frame
        }
        r.cr1 = cr1;
    }

   public:
    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /// Configure the UART. Disables and re-enables the peripheral around the write.
    /// `cfg.clock_hz` must be the peripheral kernel clock (e.g. APB bus frequency).
    static void configure(const Config& cfg) noexcept {
        if constexpr (StModernUsart<P>) {
            configure_modern(cfg);
        } else {
            configure_legacy(cfg);
        }
    }

    /// True when the TX data register is empty (safe to write another byte).
    [[nodiscard]] static auto tx_ready() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kTxe) != 0u;
        } else {
            return (sci2().sr & detail::kTxe) != 0u;
        }
    }

    /// True when the RX data register holds unread data.
    [[nodiscard]] static auto rx_ready() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kRxne) != 0u;
        } else {
            return (sci2().sr & detail::kRxne) != 0u;
        }
    }

    /// Write one byte.  Spins until the transmit register is empty.
    static void write_byte(std::byte b) noexcept {
        while (!tx_ready()) { /* spin */ }
        if constexpr (StModernUsart<P>) {
            sci3().tdr = static_cast<std::uint32_t>(b);
        } else {
            sci2().dr = static_cast<std::uint32_t>(b);
        }
    }

    /// Write a buffer.  Blocks until every byte is written to the shift register.
    static void write(std::span<const std::byte> buf) noexcept {
        for (const auto b : buf) { write_byte(b); }
    }

    /// Write a null-terminated string.
    static void write(std::string_view s) noexcept {
        write(std::as_bytes(std::span{s.data(), s.size()}));
    }

    /// Read one byte.  Spins until the receive register is not empty.
    [[nodiscard]] static auto read_byte() noexcept -> std::byte {
        while (!rx_ready()) { /* spin */ }
        if constexpr (StModernUsart<P>) {
            return static_cast<std::byte>(sci3().rdr & 0xFFu);
        } else {
            return static_cast<std::byte>(sci2().dr & 0xFFu);
        }
    }

    /// Read into a buffer.  Returns when `buf` is full.
    static void read(std::span<std::byte> buf) noexcept {
        for (auto& b : buf) { b = read_byte(); }
    }

    /// Spin until the transmission-complete flag is set (all bits on the wire).
    static void flush() noexcept {
        if constexpr (StModernUsart<P>) {
            while ((sci3().isr & detail::kTc) == 0u) { /* spin */ }
            // Clear TC flag via ICR
            sci3().icr = detail::kTc;
        } else {
            while ((sci2().sr & detail::kTc) == 0u) { /* spin */ }
            // Legacy: TC is cleared by reading SR then writing DR;
            // a dummy SR read followed by ICR-equivalent is not available —
            // the hardware clears TC automatically when the next byte is loaded.
            // Write DR=0 would corrupt output, so just let it clear naturally.
        }
    }

    /// True when the peripheral is enabled (UE bit set).
    [[nodiscard]] static auto enabled() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().cr1 & detail::kSci3Cr1Ue) != 0u;
        } else {
            return (sci2().cr1 & detail::kSci2Cr1Ue) != 0u;
        }
    }

    /// Disable the UART (clears UE).
    static void disable() noexcept {
        if constexpr (StModernUsart<P>) {
            sci3().cr1 &= ~detail::kSci3Cr1Ue;
        } else {
            sci2().cr1 &= ~detail::kSci2Cr1Ue;
        }
    }

    port() = delete;
};

}  // namespace alloy::hal::uart::lite
