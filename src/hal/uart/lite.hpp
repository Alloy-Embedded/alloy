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
/// IMPORTANT: the caller must enable the peripheral clock (and deassert reset)
/// before calling `configure()`.  With alloy-codegen v2.1 and `hal/rcc.hpp`:
/// @code
///   namespace dev = alloy::device::traits;
///   dev::peripheral_on<dev::usart1>();   // clk_enable + rst_release
/// @endcode
///
/// Typical usage:
/// @code
///   #include "hal/uart.hpp"
///   #include "hal/rcc.hpp"
///   #include "device/runtime.hpp"
///
///   namespace dev = alloy::device::traits;
///   using Uart1 = alloy::hal::uart::lite::port<dev::usart1>;
///
///   dev::peripheral_on<dev::usart1>();
///   Uart1::configure({.baudrate = 115200, .clock_hz = 16'000'000u});
///   Uart1::write("Hello\n");
/// @endcode
///
/// All methods are static — `port<P>` is a zero-size tag type, never instantiated.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "device/concepts.hpp"
#include "device/rcc_gate_table.hpp"

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

/// SAME70 UART / FLEXCOM-UART detector (kTemplate == "uart").
[[nodiscard]] consteval auto is_sam_uart(const char* tmpl) -> bool {
    return std::string_view{tmpl} == "uart";
}

// ── SAME70 UART register offsets ────────────────────────────────────────────
inline constexpr std::uintptr_t kSamCrOfs   = 0x00u;  ///< UART_CR  Control
inline constexpr std::uintptr_t kSamMrOfs   = 0x04u;  ///< UART_MR  Mode
inline constexpr std::uintptr_t kSamIerOfs  = 0x08u;  ///< UART_IER Interrupt Enable
inline constexpr std::uintptr_t kSamIdrOfs  = 0x0Cu;  ///< UART_IDR Interrupt Disable
inline constexpr std::uintptr_t kSamSrOfs   = 0x14u;  ///< UART_SR  Status
inline constexpr std::uintptr_t kSamRhrOfs  = 0x18u;  ///< UART_RHR Receive Holding
inline constexpr std::uintptr_t kSamThrOfs  = 0x1Cu;  ///< UART_THR Transmit Holding
inline constexpr std::uintptr_t kSamBrgrOfs = 0x20u;  ///< UART_BRGR Baud Rate Generator

// UART_CR write-only bits
inline constexpr std::uint32_t kSamCrRstRx  = 1u << 2u;  ///< Reset Receiver
inline constexpr std::uint32_t kSamCrRstTx  = 1u << 3u;  ///< Reset Transmitter
inline constexpr std::uint32_t kSamCrRxEn   = 1u << 4u;  ///< Receiver Enable
inline constexpr std::uint32_t kSamCrRxDis  = 1u << 5u;  ///< Receiver Disable
inline constexpr std::uint32_t kSamCrTxEn   = 1u << 6u;  ///< Transmitter Enable
inline constexpr std::uint32_t kSamCrTxDis  = 1u << 7u;  ///< Transmitter Disable
inline constexpr std::uint32_t kSamCrRstSta = 1u << 8u;  ///< Reset Status Bits

// UART_MR parity field PAR[2:0] at bits [11:9]
inline constexpr std::uint32_t kSamMrParMask = 0x7u << 9u;   ///< Parity mask
inline constexpr std::uint32_t kSamMrParEven = 0x0u << 9u;   ///< Even parity (000)
inline constexpr std::uint32_t kSamMrParOdd  = 0x1u << 9u;   ///< Odd  parity (001)
inline constexpr std::uint32_t kSamMrParNone = 0x4u << 9u;   ///< No   parity (100)

// UART_SR status bits
inline constexpr std::uint32_t kSamSrRxrdy   = 1u << 0u;  ///< Receiver Ready
inline constexpr std::uint32_t kSamSrTxrdy   = 1u << 1u;  ///< Transmitter Ready
inline constexpr std::uint32_t kSamSrOvre    = 1u << 5u;  ///< Overrun Error
inline constexpr std::uint32_t kSamSrFrame   = 1u << 6u;  ///< Framing Error
inline constexpr std::uint32_t kSamSrPare    = 1u << 7u;  ///< Parity Error
inline constexpr std::uint32_t kSamSrTxEmpty = 1u << 9u;  ///< TX Shift Register Empty

// IER / IDR bits mirror SR (same positions)
inline constexpr std::uint32_t kSamIerRxrdy  = kSamSrRxrdy;   ///< RXRDY IRQ enable
inline constexpr std::uint32_t kSamIerTxrdy  = kSamSrTxrdy;   ///< TXRDY IRQ enable
inline constexpr std::uint32_t kSamIerTxEmpty = kSamSrTxEmpty; ///< TXEMPTY IRQ enable
inline constexpr std::uint32_t kSamIerErr    = kSamSrOvre | kSamSrFrame | kSamSrPare;

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

// ISR / SR error bits — identical positions in both SCI2 (SR) and SCI3 (ISR)
inline constexpr std::uint32_t kPe  = 1u << 0;  ///< Parity error
inline constexpr std::uint32_t kFe  = 1u << 1;  ///< Framing error
inline constexpr std::uint32_t kNf  = 1u << 2;  ///< Noise flag (SCI3: NF; SCI2: NE)
inline constexpr std::uint32_t kOre = 1u << 3;  ///< Overrun error

// ICR bits — SCI3 only (write 1 to clear corresponding ISR flag)
inline constexpr std::uint32_t kIcrPecf  = 1u << 0;
inline constexpr std::uint32_t kIcrFecf  = 1u << 1;
inline constexpr std::uint32_t kIcrNecf  = 1u << 2;
inline constexpr std::uint32_t kIcrOrecf = 1u << 3;

// CR1 interrupt-enable bits — same offsets in SCI2 and SCI3
inline constexpr std::uint32_t kCr1Rxneie = 1u << 5;  ///< RXNE interrupt enable
inline constexpr std::uint32_t kCr1Tcie   = 1u << 6;  ///< Transmission-complete IE
inline constexpr std::uint32_t kCr1Txeie  = 1u << 7;  ///< TXE interrupt enable
inline constexpr std::uint32_t kCr1Peie   = 1u << 8;  ///< Parity-error interrupt enable

// CR3.EIE (bit 0) — enables FE / NF / ORE interrupts; same in both variants
inline constexpr std::uint32_t kCr3Eie = 1u << 0;

// ── New CR3 bits ────────────────────────────────────────────────────────────
inline constexpr std::uint32_t kCr3Hdsel = 1u << 3;   ///< Half-duplex selection
inline constexpr std::uint32_t kCr3Dmar  = 1u << 6;   ///< DMA receiver enable
inline constexpr std::uint32_t kCr3Dmat  = 1u << 7;   ///< DMA transmitter enable
inline constexpr std::uint32_t kCr3Rtse  = 1u << 8;   ///< RTS enable (HW flow control)
inline constexpr std::uint32_t kCr3Ctse  = 1u << 9;   ///< CTS enable (HW flow control)
inline constexpr std::uint32_t kCr3Ctsie = 1u << 10;  ///< CTS interrupt enable
inline constexpr std::uint32_t kCr3Scen  = 1u << 5;   ///< Smartcard mode enable
inline constexpr std::uint32_t kCr3Iren  = 1u << 1;   ///< IrDA enable
inline constexpr std::uint32_t kCr3Dem   = 1u << 14;  ///< DE (RS-485 driver enable) mode
inline constexpr std::uint32_t kCr3Dep   = 1u << 15;  ///< DE polarity (0=active-high)

// ── SCI3 CR1 extended bits ──────────────────────────────────────────────────
inline constexpr std::uint32_t kSci3Cr1Over8 = 1u << 15;  ///< 8x oversampling
inline constexpr std::uint32_t kSci3Cr1Uesm  = 1u << 23;  ///< Wake from Stop enable
inline constexpr std::uint32_t kSci3Cr1Fifoen = 1u << 29; ///< FIFO mode enable
inline constexpr std::uint32_t kSci3Cr1DedtShift = 16u;
inline constexpr std::uint32_t kSci3Cr1DedtMask  = 0x1Fu << 16u;  ///< DE deassertion time [20:16]
inline constexpr std::uint32_t kSci3Cr1DeatShift = 21u;
inline constexpr std::uint32_t kSci3Cr1DeatMask  = 0x1Fu << 21u;  ///< DE assertion time [25:21]

// ── CR2 extensions ──────────────────────────────────────────────────────────
inline constexpr std::uint32_t kCr2Lbdie = 1u << 6;   ///< LIN break detection IE
inline constexpr std::uint32_t kCr2Lbdl  = 1u << 5;   ///< LIN break detection length (0=10b,1=11b)
inline constexpr std::uint32_t kCr2Clken = 1u << 11;  ///< SCLK pin enable (synchronous)
inline constexpr std::uint32_t kCr2Linen = 1u << 14;  ///< LIN mode enable

// ── SCI2 CR1 extra bit ──────────────────────────────────────────────────────
inline constexpr std::uint32_t kSci2Cr1Sbk = 1u << 0; ///< Send break (SCI2 LIN)

// ── ISR / SR extended status bits ───────────────────────────────────────────
inline constexpr std::uint32_t kIsrLbdf = 1u << 8;    ///< LIN break detected
inline constexpr std::uint32_t kIsrRxff = 1u << 24;   ///< RX FIFO full (SCI3 FIFO mode)
inline constexpr std::uint32_t kIsrTxff = 1u << 25;   ///< TX FIFO full (SCI3 FIFO mode)
inline constexpr std::uint32_t kIsrIdle = 1u << 4;    ///< IDLE line detected

// ── ICR extended clear bits ─────────────────────────────────────────────────
inline constexpr std::uint32_t kIcrLbdcf = 1u << 8;   ///< LIN break detection clear

// ── RQR bits (SCI3 only) ─────────────────────────────────────────────────────
inline constexpr std::uint32_t kRqrSbkrq = 1u << 1;   ///< Send break request

// ── CR1 IDLE interrupt enable ───────────────────────────────────────────────
inline constexpr std::uint32_t kCr1Idleie = 1u << 4;  ///< IDLE interrupt enable

// ── SCI3 CR3 FIFO threshold fields ──────────────────────────────────────────
inline constexpr std::uint32_t kCr3RxftcfgShift = 24u;
inline constexpr std::uint32_t kCr3RxftcfgMask  = 0x7u << 24u;  ///< RX FIFO threshold [26:24]
inline constexpr std::uint32_t kCr3RxftieShift  = 27u;
inline constexpr std::uint32_t kCr3Rxftie       = 1u << 27u;    ///< RX FIFO threshold IE
inline constexpr std::uint32_t kCr3TxftcfgShift = 29u;
inline constexpr std::uint32_t kCr3TxftcfgMask  = 0x7u << 29u;  ///< TX FIFO threshold [31:29]
inline constexpr std::uint32_t kCr3Txftie       = 1u << 23u;    ///< TX FIFO threshold IE

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

/// Satisfied when P is a SAME70 UART / FLEXCOM-UART peripheral (kTemplate == "uart").
template <typename P>
concept SamUart =
    device::PeripheralSpec<P> &&
    requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
    detail::is_sam_uart(P::kTemplate);

/// Any supported UART variant: STM32 (SCI2 / SCI3) or SAME70 UART.
template <typename P>
concept AnyUart = StUsart<P> || SamUart<P>;

// ============================================================================
// Extended types — RS-485, DMA, LIN, FIFO, errors
// ============================================================================

/// Aggregated UART error flags.
struct UartErrors {
    bool parity  = false;  ///< PE  — parity mismatch
    bool framing = false;  ///< FE  — stop bit not detected
    bool noise   = false;  ///< NE  — noise flag
    bool overrun = false;  ///< ORE — overrun error (data lost)
    [[nodiscard]] constexpr auto any() const noexcept -> bool {
        return parity || framing || noise || overrun;
    }
};

/// FIFO depth threshold for set_tx/rx_fifo_threshold() (SCI3 only).
enum class FifoTrigger : std::uint8_t {
    Empty         = 0,
    Quarter       = 1,
    Half          = 2,
    ThreeQuarters = 3,
    Full          = 4,
};

/// Interrupt source selector for enable_interrupt().
enum class InterruptKind : std::uint8_t {
    Tc,              ///< Transmission complete
    Txe,             ///< TX register empty
    Rxne,            ///< RX register not empty
    IdleLine,        ///< IDLE line detected
    LinBreak,        ///< LIN break detected (CR2 LBDIE)
    Cts,             ///< CTS edge detected (CR3 CTSIE)
    Error,           ///< Error group: FE/NF/ORE (CR3 EIE) + PE (CR1 PEIE)
    RxFifoThreshold, ///< RX FIFO threshold reached (SCI3, CR3 RXFTIE)
    TxFifoThreshold, ///< TX FIFO threshold reached (SCI3, CR3 TXFTIE)
};

/// Oversampling ratio — affects baud rate divisor and noise tolerance.
enum class Oversampling : std::uint8_t {
    X16 = 0,  ///< 16x (default; better noise rejection)
    X8  = 1,  ///< 8x  (allows higher baud rates; less noise rejection)
};

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

/// Direct-MMIO UART port.  P must satisfy AnyUart (STM32 SCI3/SCI2 or SAME70 UART).
///
/// Dispatch between register layouts is fully compile-time (if constexpr).
/// All methods are static — obtain a "handle" simply by naming the type:
/// @code
///   // STM32
///   using Uart1 = alloy::hal::uart::lite::port<traits::usart1>;
///   Uart1::configure({.baudrate = 115200, .clock_hz = 16'000'000});
///   Uart1::write_byte(std::byte{'X'});
///
///   // SAME70
///   using Uart0 = alloy::hal::uart::lite::port<traits::uart0>;
///   Uart0::configure({.baudrate = 115200, .clock_hz = 150'000'000});
///   Uart0::write_byte(std::byte{'X'});
/// @endcode
template <typename P>
    requires AnyUart<P>
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

    /// SAME70 UART: register access by byte offset from peripheral base.
    [[nodiscard]] static auto sam_reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + ofs);
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

    static void configure_sam(const Config& cfg) {
        // 1. Reset receiver + transmitter + status flags
        sam_reg(detail::kSamCrOfs) =
            detail::kSamCrRstRx | detail::kSamCrRstTx | detail::kSamCrRstSta;

        // 2. Baud rate: BRGR.CD = clock_hz / (16 × baudrate)
        //    (SAME70 UART is fixed at 16× oversampling)
        const auto cd = cfg.clock_hz / (cfg.baudrate * 16u);
        sam_reg(detail::kSamBrgrOfs) = cd & 0xFFFFu;

        // 3. Mode register: parity + normal channel mode (no loop-back)
        std::uint32_t mr = 0u;
        if (cfg.parity == Parity::None) {
            mr |= detail::kSamMrParNone;
        } else if (cfg.parity == Parity::Odd) {
            mr |= detail::kSamMrParOdd;
        } else {
            mr |= detail::kSamMrParEven;
        }
        sam_reg(detail::kSamMrOfs) = mr;

        // 4. Enable requested directions
        std::uint32_t cr = 0u;
        if (cfg.rx_enable) { cr |= detail::kSamCrRxEn;  }
        else               { cr |= detail::kSamCrRxDis; }
        if (cfg.tx_enable) { cr |= detail::kSamCrTxEn;  }
        else               { cr |= detail::kSamCrTxDis; }
        sam_reg(detail::kSamCrOfs) = cr;
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
        } else if constexpr (StLegacyUsart<P>) {
            configure_legacy(cfg);
        } else if constexpr (SamUart<P>) {
            configure_sam(cfg);
        }
    }

    /// True when the TX data register is empty (safe to write another byte).
    [[nodiscard]] static auto tx_ready() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kTxe) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            return (sci2().sr & detail::kTxe) != 0u;
        } else {
            return (sam_reg(detail::kSamSrOfs) & detail::kSamSrTxrdy) != 0u;
        }
    }

    /// True when the RX data register holds unread data.
    [[nodiscard]] static auto rx_ready() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kRxne) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            return (sci2().sr & detail::kRxne) != 0u;
        } else {
            return (sam_reg(detail::kSamSrOfs) & detail::kSamSrRxrdy) != 0u;
        }
    }

    /// Alias for rx_ready() — matches SAME70 naming convention.
    [[nodiscard]] static auto data_available() noexcept -> bool { return rx_ready(); }

    /// Alias for tx_ready() — matches SAME70 naming convention.
    [[nodiscard]] static auto ready_to_send() noexcept -> bool { return tx_ready(); }

    /// Write one byte.  Spins until the transmit register is empty.
    static void write_byte(std::byte b) noexcept {
        while (!tx_ready()) { /* spin */ }
        if constexpr (StModernUsart<P>) {
            sci3().tdr = static_cast<std::uint32_t>(b);
        } else if constexpr (StLegacyUsart<P>) {
            sci2().dr = static_cast<std::uint32_t>(b);
        } else {
            sam_reg(detail::kSamThrOfs) = static_cast<std::uint32_t>(b);
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
        } else if constexpr (StLegacyUsart<P>) {
            return static_cast<std::byte>(sci2().dr & 0xFFu);
        } else {
            return static_cast<std::byte>(sam_reg(detail::kSamRhrOfs) & 0xFFu);
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
            sci3().icr = detail::kTc;
        } else if constexpr (StLegacyUsart<P>) {
            while ((sci2().sr & detail::kTc) == 0u) { /* spin */ }
        } else {
            // SAME70: TXEMPTY = shift register is empty (equivalent to TC)
            while ((sam_reg(detail::kSamSrOfs) & detail::kSamSrTxEmpty) == 0u) { /* spin */ }
        }
    }

    /// True when the peripheral is enabled (UE bit set).
    [[nodiscard]] static auto enabled() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().cr1 & detail::kSci3Cr1Ue) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            return (sci2().cr1 & detail::kSci2Cr1Ue) != 0u;
        } else {
            // SAME70: UART_CR is write-only; assume enabled after configure().
            return true;
        }
    }

    /// Disable the UART.
    static void disable() noexcept {
        if constexpr (StModernUsart<P>) {
            sci3().cr1 &= ~detail::kSci3Cr1Ue;
        } else if constexpr (StLegacyUsart<P>) {
            sci2().cr1 &= ~detail::kSci2Cr1Ue;
        } else {
            sam_reg(detail::kSamCrOfs) = detail::kSamCrRxDis | detail::kSamCrTxDis;
        }
    }

    // -----------------------------------------------------------------------
    // Error status
    // -----------------------------------------------------------------------

    /// True if a framing error was detected in the last received byte.
    [[nodiscard]] static auto frame_error() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kFe) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            return (sci2().sr  & detail::kFe) != 0u;
        } else {
            return (sam_reg(detail::kSamSrOfs) & detail::kSamSrFrame) != 0u;
        }
    }

    /// True if the receive shift register was overrun (unread byte lost).
    [[nodiscard]] static auto overrun() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kOre) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            return (sci2().sr  & detail::kOre) != 0u;
        } else {
            return (sam_reg(detail::kSamSrOfs) & detail::kSamSrOvre) != 0u;
        }
    }

    /// True if a parity error was detected.
    [[nodiscard]] static auto parity_error() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kPe) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            return (sci2().sr  & detail::kPe) != 0u;
        } else {
            return (sam_reg(detail::kSamSrOfs) & detail::kSamSrPare) != 0u;
        }
    }

    /// True if a noise flag was detected (NF in SCI3/SCI2; not available on SAME70 UART).
    [[nodiscard]] static auto noise_error() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kNf) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            return (sci2().sr  & detail::kNf) != 0u;
        } else {
            return false;  // SAME70 UART has no noise-flag bit
        }
    }

    /// True if any error flag is currently set.
    [[nodiscard]] static auto has_errors() noexcept -> bool {
        if constexpr (StModernUsart<P>) {
            constexpr auto kMask = detail::kPe | detail::kFe | detail::kNf | detail::kOre;
            return (sci3().isr & kMask) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            constexpr auto kMask = detail::kPe | detail::kFe | detail::kNf | detail::kOre;
            return (sci2().sr  & kMask) != 0u;
        } else {
            constexpr auto kMask = detail::kSamSrOvre | detail::kSamSrFrame | detail::kSamSrPare;
            return (sam_reg(detail::kSamSrOfs) & kMask) != 0u;
        }
    }

    /// Clear all error flags.
    ///
    /// SCI3: writes ICR.  SCI2: reads SR+DR.  SAME70: writes RSTSTA to UART_CR.
    static void clear_errors() noexcept {
        if constexpr (StModernUsart<P>) {
            sci3().icr = detail::kIcrPecf | detail::kIcrFecf |
                         detail::kIcrNecf | detail::kIcrOrecf;
        } else if constexpr (StLegacyUsart<P>) {
            (void)sci2().sr;
            (void)sci2().dr;
        } else {
            sam_reg(detail::kSamCrOfs) = detail::kSamCrRstSta;
        }
    }

    // -----------------------------------------------------------------------
    // Non-blocking I/O
    // -----------------------------------------------------------------------

    /// Try to read one byte without blocking.
    ///
    /// Returns the byte if RXNE is set, or an empty optional if no data is
    /// available.  Does NOT wait for data — call `rx_ready()` to poll.
    [[nodiscard]] static auto try_read_byte() noexcept -> std::optional<std::byte> {
        if (!rx_ready()) { return {}; }
        if constexpr (StModernUsart<P>) {
            return static_cast<std::byte>(sci3().rdr & 0xFFu);
        } else if constexpr (StLegacyUsart<P>) {
            return static_cast<std::byte>(sci2().dr & 0xFFu);
        } else {
            return static_cast<std::byte>(sam_reg(detail::kSamRhrOfs) & 0xFFu);
        }
    }

    /// Try to write one byte without blocking.
    ///
    /// Returns true and transmits `b` if TXE is set (register was empty),
    /// or returns false immediately if the transmit register is still full.
    [[nodiscard]] static auto try_write_byte(std::byte b) noexcept -> bool {
        if (!tx_ready()) { return false; }
        if constexpr (StModernUsart<P>) {
            sci3().tdr = static_cast<std::uint32_t>(b);
        } else if constexpr (StLegacyUsart<P>) {
            sci2().dr = static_cast<std::uint32_t>(b);
        } else {
            sam_reg(detail::kSamThrOfs) = static_cast<std::uint32_t>(b);
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Interrupt control
    // -----------------------------------------------------------------------

    /// Enable the RXNE / RXRDY interrupt.
    static void enable_rx_irq() noexcept {
        if constexpr (StModernUsart<P>) {
            sci3().cr1 |= detail::kCr1Rxneie;
        } else if constexpr (StLegacyUsart<P>) {
            sci2().cr1 |= detail::kCr1Rxneie;
        } else {
            sam_reg(detail::kSamIerOfs) = detail::kSamIerRxrdy;
        }
    }

    /// Enable the TXE / TXRDY interrupt.
    static void enable_tx_irq() noexcept {
        if constexpr (StModernUsart<P>) {
            sci3().cr1 |= detail::kCr1Txeie;
        } else if constexpr (StLegacyUsart<P>) {
            sci2().cr1 |= detail::kCr1Txeie;
        } else {
            sam_reg(detail::kSamIerOfs) = detail::kSamIerTxrdy;
        }
    }

    /// Enable the TC (transmission complete) / TXEMPTY interrupt.
    static void enable_tc_irq() noexcept {
        if constexpr (StModernUsart<P>) {
            sci3().cr1 |= detail::kCr1Tcie;
        } else if constexpr (StLegacyUsart<P>) {
            sci2().cr1 |= detail::kCr1Tcie;
        } else {
            // SAME70: TXEMPTY (shift register empty) is the closest equivalent to TC
            sam_reg(detail::kSamIerOfs) = detail::kSamIerTxEmpty;
        }
    }

    /// Enable error interrupts.
    ///
    /// STM32: sets CR3.EIE (FE/NF/ORE) + CR1.PEIE.
    /// SAME70: enables OVRE + FRAME + PARE bits via IER.
    static void enable_error_irq() noexcept {
        if constexpr (StModernUsart<P>) {
            sci3().cr3 |= detail::kCr3Eie;
            sci3().cr1 |= detail::kCr1Peie;
        } else if constexpr (StLegacyUsart<P>) {
            sci2().cr3 |= detail::kCr3Eie;
            sci2().cr1 |= detail::kCr1Peie;
        } else {
            sam_reg(detail::kSamIerOfs) = detail::kSamIerErr;
        }
    }

    /// Disable all UART interrupt sources.
    ///
    /// STM32: clears RXNEIE/TXEIE/TCIE/PEIE in CR1 and EIE in CR3.
    /// SAME70: writes all-ones to IDR (write-only mask register).
    static void disable_irqs() noexcept {
        constexpr std::uint32_t kCr1Mask =
            detail::kCr1Rxneie | detail::kCr1Txeie |
            detail::kCr1Tcie   | detail::kCr1Peie;
        if constexpr (StModernUsart<P>) {
            sci3().cr1 &= ~kCr1Mask;
            sci3().cr3 &= ~detail::kCr3Eie;
        } else if constexpr (StLegacyUsart<P>) {
            sci2().cr1 &= ~kCr1Mask;
            sci2().cr3 &= ~detail::kCr3Eie;
        } else {
            sam_reg(detail::kSamIdrOfs) = 0xFFFFFFFFu;
        }
    }

    // -----------------------------------------------------------------------
    // Device-data bridge — sourced from alloy.device.v2.1 flat-struct
    // -----------------------------------------------------------------------

    /// Returns the NVIC IRQ line for this peripheral.
    ///
    /// Sourced from `P::kIrqLines[idx]` (flat-struct v2.1).
    /// Compiles to a constexpr constant — zero overhead.
    /// @param idx  Index into the IRQ array (default 0 for single-IRQ peripherals).
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
    ///
    /// Sourced from `P::kIrqCount` (flat-struct v2.1).
    /// Returns 0 when the field is absent (pre-v2.1 artifact).
    [[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t {
        if constexpr (requires { P::kIrqCount; }) {
            return static_cast<std::size_t>(P::kIrqCount);
        }
        return 0u;
    }

    // -----------------------------------------------------------------------
    // Clock gate — sourced from alloy.device.v2.1 flat-struct kRccEnable
    // -----------------------------------------------------------------------

    /// Enable the peripheral clock (APBx/AHBx ENR bit).
    ///
    /// Requires `ALLOY_DEVICE_RCC_TABLE_AVAILABLE` at build time (set by the
    /// `alloy_device_rcc_table` CMake target) to actually write the register.
    /// The method always compiles; the body is a no-op in host compile tests
    /// where no generated gate table is present.
    static void clock_on() noexcept
        requires (requires { P::kRccEnable; })
    {
#if defined(ALLOY_DEVICE_RCC_TABLE_AVAILABLE)
        constexpr auto gate = device::detail::find_rcc_gate(P::kRccEnable);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<volatile std::uint32_t*>(gate.addr) |= gate.mask;
#endif
    }

    /// Disable the peripheral clock.
    static void clock_off() noexcept
        requires (requires { P::kRccEnable; })
    {
#if defined(ALLOY_DEVICE_RCC_TABLE_AVAILABLE)
        constexpr auto gate = device::detail::find_rcc_gate(P::kRccEnable);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<volatile std::uint32_t*>(gate.addr) &= ~gate.mask;
#endif
    }

    // -----------------------------------------------------------------------
    // Error snapshot (read + optionally clear)
    // -----------------------------------------------------------------------

    /// Read all error flags without clearing them.
    [[nodiscard]] static auto error_flags() noexcept -> UartErrors {
        UartErrors e{};
        if constexpr (StModernUsart<P>) {
            const auto isr = sci3().isr;
            e.parity  = (isr & detail::kPe)  != 0u;
            e.framing = (isr & detail::kFe)  != 0u;
            e.noise   = (isr & detail::kNf)  != 0u;
            e.overrun = (isr & detail::kOre) != 0u;
        } else if constexpr (StLegacyUsart<P>) {
            const auto sr = sci2().sr;
            e.parity  = (sr & detail::kPe)  != 0u;
            e.framing = (sr & detail::kFe)  != 0u;
            e.noise   = (sr & detail::kNf)  != 0u;
            e.overrun = (sr & detail::kOre) != 0u;
        } else {
            const auto sr = sam_reg(detail::kSamSrOfs);
            e.framing = (sr & detail::kSamSrFrame) != 0u;
            e.overrun = (sr & detail::kSamSrOvre)  != 0u;
            e.parity  = (sr & detail::kSamSrPare)  != 0u;
        }
        return e;
    }

    /// Read all error flags and atomically clear them.
    ///
    /// SCI3: reads ISR, then writes ICR (single-register, atomic).
    /// SCI2: reads SR, then dummy-reads DR to clear sticky flags.
    /// SAME70: reads SR, then writes UART_CR RSTSTA.
    [[nodiscard]] static auto read_and_clear_errors() noexcept -> UartErrors {
        const auto e = error_flags();
        clear_errors();
        return e;
    }

    // -----------------------------------------------------------------------
    // Hardware flow control (ST USART only)
    // -----------------------------------------------------------------------

    /// Enable or disable hardware RTS/CTS flow control.
    ///
    /// Sets CR3.RTSE (bit 8) and CR3.CTSE (bit 9) together.
    /// Must be called while the UART is disabled or between frames.
    static void enable_hardware_flow_control(bool enable) noexcept
        requires StUsart<P>
    {
        constexpr auto kMask = detail::kCr3Rtse | detail::kCr3Ctse;
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr3 |=  kMask; }
            else        { sci3().cr3 &= ~kMask; }
        } else {
            if (enable) { sci2().cr3 |=  kMask; }
            else        { sci2().cr3 &= ~kMask; }
        }
    }

    // -----------------------------------------------------------------------
    // RS-485 DE (Driver Enable) control (ST USART only)
    // -----------------------------------------------------------------------

    /// Set Driver Enable polarity.
    ///
    /// @param active_high  true = DE active-high (default); false = active-low.
    /// CR3.DEP (bit 15): 0 = DE active-high, 1 = DE active-low.
    static void set_de_polarity(bool active_high) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (active_high) { sci3().cr3 &= ~detail::kCr3Dep; }
            else             { sci3().cr3 |=  detail::kCr3Dep; }
        } else {
            if (active_high) { sci2().cr3 &= ~detail::kCr3Dep; }
            else             { sci2().cr3 |=  detail::kCr3Dep; }
        }
    }

    /// Enable or disable the RS-485 Driver Enable output (CR3.DEM, bit 14).
    static void enable_de(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr3 |=  detail::kCr3Dem; }
            else        { sci3().cr3 &= ~detail::kCr3Dem; }
        } else {
            if (enable) { sci2().cr3 |=  detail::kCr3Dem; }
            else        { sci2().cr3 &= ~detail::kCr3Dem; }
        }
    }

    /// Set DE assertion time (SCI3 only) — CR1.DEAT[25:21].
    ///
    /// @param half_bits Number of sample clock periods from DE assertion to start of stop bit.
    static void set_de_assertion_time(std::uint8_t half_bits) noexcept
        requires StModernUsart<P>
    {
        auto cr1 = sci3().cr1;
        cr1 &= ~detail::kSci3Cr1DeatMask;
        cr1 |= (static_cast<std::uint32_t>(half_bits) & 0x1Fu) << detail::kSci3Cr1DeatShift;
        sci3().cr1 = cr1;
    }

    /// Set DE deassertion time (SCI3 only) — CR1.DEDT[20:16].
    static void set_de_deassertion_time(std::uint8_t half_bits) noexcept
        requires StModernUsart<P>
    {
        auto cr1 = sci3().cr1;
        cr1 &= ~detail::kSci3Cr1DedtMask;
        cr1 |= (static_cast<std::uint32_t>(half_bits) & 0x1Fu) << detail::kSci3Cr1DedtShift;
        sci3().cr1 = cr1;
    }

    // -----------------------------------------------------------------------
    // Half-duplex (ST USART only)
    // -----------------------------------------------------------------------

    /// Enable or disable half-duplex (single-wire) mode — CR3.HDSEL (bit 3).
    ///
    /// Note: disable CTS/RTS flow control before enabling half-duplex.
    static void set_half_duplex(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr3 |=  detail::kCr3Hdsel; }
            else        { sci3().cr3 &= ~detail::kCr3Hdsel; }
        } else {
            if (enable) { sci2().cr3 |=  detail::kCr3Hdsel; }
            else        { sci2().cr3 &= ~detail::kCr3Hdsel; }
        }
    }

    // -----------------------------------------------------------------------
    // LIN mode (ST USART only)
    // -----------------------------------------------------------------------

    /// Enable or disable LIN mode — CR2.LINEN (bit 14).
    ///
    /// Automatically clears CR2.CLKEN (synchronous mode) when enabling LIN.
    static void enable_lin(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) {
                sci3().cr2 = (sci3().cr2 | detail::kCr2Linen) & ~detail::kCr2Clken;
            } else {
                sci3().cr2 &= ~detail::kCr2Linen;
            }
        } else {
            if (enable) {
                sci2().cr2 = (sci2().cr2 | detail::kCr2Linen) & ~detail::kCr2Clken;
            } else {
                sci2().cr2 &= ~detail::kCr2Linen;
            }
        }
    }

    /// Set LIN break detection length — CR2.LBDL (bit 5).
    ///
    /// @param long_break  false = 10-bit break (default); true = 11-bit break.
    static void set_lin_break_length(bool long_break) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (long_break) { sci3().cr2 |=  detail::kCr2Lbdl; }
            else            { sci3().cr2 &= ~detail::kCr2Lbdl; }
        } else {
            if (long_break) { sci2().cr2 |=  detail::kCr2Lbdl; }
            else            { sci2().cr2 &= ~detail::kCr2Lbdl; }
        }
    }

    /// Send a LIN break character.
    ///
    /// SCI3: sets RQR.SBKRQ (bit 1) — hardware sends break and clears the bit.
    /// SCI2: sets CR1.SBK (bit 0) and polls until hardware clears it.
    static void send_lin_break() noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            sci3().rqr |= detail::kRqrSbkrq;
        } else {
            sci2().cr1 |= detail::kSci2Cr1Sbk;
            // HW clears SBK after the break is sent; poll for safety.
            while (sci2().cr1 & detail::kSci2Cr1Sbk) { /* spin */ }
        }
    }

    /// True when the LIN break detection flag is set (ISR/SR bit 8).
    [[nodiscard]] static auto lin_break_detected() noexcept -> bool
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            return (sci3().isr & detail::kIsrLbdf) != 0u;
        } else {
            return (sci2().sr & detail::kIsrLbdf) != 0u;
        }
    }

    /// Clear the LIN break detection flag.
    ///
    /// SCI3: writes ICR.LBDCF (bit 8).
    /// SCI2: reads SR + DR (sticky-flag side-effect clear).
    static void clear_lin_break_flag() noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            sci3().icr = detail::kIcrLbdcf;
        } else {
            (void)sci2().sr;
            (void)sci2().dr;
        }
    }

    /// Enable or disable the LIN break detection interrupt (CR2.LBDIE, bit 6).
    static void enable_lin_break_irq(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr2 |=  detail::kCr2Lbdie; }
            else        { sci3().cr2 &= ~detail::kCr2Lbdie; }
        } else {
            if (enable) { sci2().cr2 |=  detail::kCr2Lbdie; }
            else        { sci2().cr2 &= ~detail::kCr2Lbdie; }
        }
    }

    // -----------------------------------------------------------------------
    // Smartcard and IrDA (ST USART only)
    // -----------------------------------------------------------------------

    /// Enable or disable Smartcard mode — CR3.SCEN (bit 5).
    static void set_smartcard_mode(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr3 |=  detail::kCr3Scen; }
            else        { sci3().cr3 &= ~detail::kCr3Scen; }
        } else {
            if (enable) { sci2().cr3 |=  detail::kCr3Scen; }
            else        { sci2().cr3 &= ~detail::kCr3Scen; }
        }
    }

    /// Enable or disable IrDA mode — CR3.IREN (bit 1).
    static void set_irda_mode(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr3 |=  detail::kCr3Iren; }
            else        { sci3().cr3 &= ~detail::kCr3Iren; }
        } else {
            if (enable) { sci2().cr3 |=  detail::kCr3Iren; }
            else        { sci2().cr3 &= ~detail::kCr3Iren; }
        }
    }

    // -----------------------------------------------------------------------
    // FIFO (SCI3 only)
    // -----------------------------------------------------------------------

    /// Enable or disable FIFO mode — SCI3 CR1.FIFOEN (bit 29).
    ///
    /// Must be called while UART is disabled (UE=0).
    static void enable_fifo(bool enable) noexcept
        requires StModernUsart<P>
    {
        if (enable) { sci3().cr1 |=  detail::kSci3Cr1Fifoen; }
        else        { sci3().cr1 &= ~detail::kSci3Cr1Fifoen; }
    }

    /// Set TX FIFO threshold — CR3.TXFTCFG[31:29].
    static void set_tx_fifo_threshold(FifoTrigger t) noexcept
        requires StModernUsart<P>
    {
        auto cr3 = sci3().cr3;
        cr3 &= ~detail::kCr3TxftcfgMask;
        cr3 |= (static_cast<std::uint32_t>(t) << detail::kCr3TxftcfgShift);
        sci3().cr3 = cr3;
    }

    /// Set RX FIFO threshold — CR3.RXFTCFG[26:24].
    static void set_rx_fifo_threshold(FifoTrigger t) noexcept
        requires StModernUsart<P>
    {
        auto cr3 = sci3().cr3;
        cr3 &= ~detail::kCr3RxftcfgMask;
        cr3 |= (static_cast<std::uint32_t>(t) << detail::kCr3RxftcfgShift);
        sci3().cr3 = cr3;
    }

    /// True when the TX FIFO is full (ISR bit 25).
    [[nodiscard]] static auto tx_fifo_full() noexcept -> bool
        requires StModernUsart<P>
    {
        return (sci3().isr & detail::kIsrTxff) != 0u;
    }

    /// True when the RX FIFO is full (ISR bit 24).
    [[nodiscard]] static auto rx_fifo_full() noexcept -> bool
        requires StModernUsart<P>
    {
        return (sci3().isr & detail::kIsrRxff) != 0u;
    }

    // -----------------------------------------------------------------------
    // DMA enable (ST USART only)
    // -----------------------------------------------------------------------

    /// Enable or disable DMA transmitter request — CR3.DMAT (bit 7).
    static void enable_dma_tx(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr3 |=  detail::kCr3Dmat; }
            else        { sci3().cr3 &= ~detail::kCr3Dmat; }
        } else {
            if (enable) { sci2().cr3 |=  detail::kCr3Dmat; }
            else        { sci2().cr3 &= ~detail::kCr3Dmat; }
        }
    }

    /// Enable or disable DMA receiver request — CR3.DMAR (bit 6).
    static void enable_dma_rx(bool enable) noexcept
        requires StUsart<P>
    {
        if constexpr (StModernUsart<P>) {
            if (enable) { sci3().cr3 |=  detail::kCr3Dmar; }
            else        { sci3().cr3 &= ~detail::kCr3Dmar; }
        } else {
            if (enable) { sci2().cr3 |=  detail::kCr3Dmar; }
            else        { sci2().cr3 &= ~detail::kCr3Dmar; }
        }
    }

    // -----------------------------------------------------------------------
    // Unified interrupt control
    // -----------------------------------------------------------------------

    /// Enable or disable a specific interrupt source.
    ///
    /// Dispatches to the correct control register based on InterruptKind.
    /// RxFifoThreshold / TxFifoThreshold only have effect on SCI3.
    static void enable_interrupt(InterruptKind kind, bool enable) noexcept
        requires StUsart<P>
    {
        auto set_cr1 = [&](std::uint32_t bit) {
            if constexpr (StModernUsart<P>) {
                if (enable) { sci3().cr1 |= bit; } else { sci3().cr1 &= ~bit; }
            } else {
                if (enable) { sci2().cr1 |= bit; } else { sci2().cr1 &= ~bit; }
            }
        };
        auto set_cr2 = [&](std::uint32_t bit) {
            if constexpr (StModernUsart<P>) {
                if (enable) { sci3().cr2 |= bit; } else { sci3().cr2 &= ~bit; }
            } else {
                if (enable) { sci2().cr2 |= bit; } else { sci2().cr2 &= ~bit; }
            }
        };
        auto set_cr3 = [&](std::uint32_t bit) {
            if constexpr (StModernUsart<P>) {
                if (enable) { sci3().cr3 |= bit; } else { sci3().cr3 &= ~bit; }
            } else {
                if (enable) { sci2().cr3 |= bit; } else { sci2().cr3 &= ~bit; }
            }
        };

        switch (kind) {
            case InterruptKind::Tc:
                set_cr1(detail::kCr1Tcie);
                break;
            case InterruptKind::Txe:
                set_cr1(detail::kCr1Txeie);
                break;
            case InterruptKind::Rxne:
                set_cr1(detail::kCr1Rxneie);
                break;
            case InterruptKind::IdleLine:
                set_cr1(detail::kCr1Idleie);
                break;
            case InterruptKind::LinBreak:
                set_cr2(detail::kCr2Lbdie);
                break;
            case InterruptKind::Cts:
                set_cr3(detail::kCr3Ctsie);
                break;
            case InterruptKind::Error:
                set_cr3(detail::kCr3Eie);
                set_cr1(detail::kCr1Peie);
                break;
            case InterruptKind::RxFifoThreshold:
                if constexpr (StModernUsart<P>) {
                    if (enable) { sci3().cr3 |=  detail::kCr3Rxftie; }
                    else        { sci3().cr3 &= ~detail::kCr3Rxftie; }
                }
                break;
            case InterruptKind::TxFifoThreshold:
                if constexpr (StModernUsart<P>) {
                    if (enable) { sci3().cr3 |=  detail::kCr3Txftie; }
                    else        { sci3().cr3 &= ~detail::kCr3Txftie; }
                }
                break;
        }
    }

    // -----------------------------------------------------------------------
    // Oversampling and wakeup from Stop (ST USART only)
    // -----------------------------------------------------------------------

    /// Set oversampling ratio — CR1.OVER8 (bit 15).
    ///
    /// Must be called while UART is disabled.
    /// X8 allows higher baud rates at the cost of noise immunity.
    static void set_oversampling(Oversampling mode) noexcept
        requires StUsart<P>
    {
        const auto bit = detail::kSci3Cr1Over8;
        if constexpr (StModernUsart<P>) {
            if (mode == Oversampling::X8) { sci3().cr1 |= bit; }
            else                          { sci3().cr1 &= ~bit; }
        } else {
            // SCI2 also has OVER8 at bit 15
            if (mode == Oversampling::X8) { sci2().cr1 |= bit; }
            else                          { sci2().cr1 &= ~bit; }
        }
    }

    /// Enable or disable wake-from-Stop mode — SCI3 CR1.UESM (bit 23).
    static void enable_wakeup_from_stop(bool enable) noexcept
        requires StModernUsart<P>
    {
        if (enable) { sci3().cr1 |=  detail::kSci3Cr1Uesm; }
        else        { sci3().cr1 &= ~detail::kSci3Cr1Uesm; }
    }

    port() = delete;
};

}  // namespace alloy::hal::uart::lite
