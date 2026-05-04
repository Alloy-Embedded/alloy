/// @file tests/compile_tests/test_uart_port.cpp
/// Compile-time verification of the Phase 3 extended MMIO surface in
/// uart/lite.hpp (uart-lite-full-surface openspec, tasks 3.1–3.3).
///
/// Uses self-contained mock PeripheralSpecs — no real device artifact needed.
///
/// Verifies:
///   - All new types (UartErrors, FifoTrigger, InterruptKind, Oversampling)
///   - All new detail:: bit constants are present and have the correct value
///   - All new methods compile with correct return types (tasks 3.3.1–3.3.26)
///   - StModernUsart-gated methods are inaccessible from SamUart mocks
///     (requires-clause check via expression-level requires{} tests)
///   - StUsart-gated methods work for both SCI3 and SCI2 mocks

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>

#include "hal/uart/lite.hpp"

// ============================================================================
// Mock peripheral specs
// ============================================================================

namespace mock {

/// SCI3 USART — STM32G0 / G4 family (modern).
struct usart_sci3 {
    static constexpr std::uintptr_t kBaseAddress = 0x40013800u;
    static constexpr const char*    kName        = "usart1";
    static constexpr const char*    kTemplate    = "usart";
    static constexpr const char*    kIpVersion   = "sci3_v1_3";
    static constexpr unsigned       kIrqLines[]  = { 27u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// SCI2 USART — STM32F4 family (legacy).
struct usart_sci2 {
    static constexpr std::uintptr_t kBaseAddress = 0x40011000u;
    static constexpr const char*    kName        = "usart1";
    static constexpr const char*    kTemplate    = "usart";
    static constexpr const char*    kIpVersion   = "sci2_v2_1";
    static constexpr unsigned       kIrqLines[]  = { 37u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// SAME70 UART — must NOT satisfy StUsart; used for negative requires-checks.
struct sam_uart {
    static constexpr std::uintptr_t kBaseAddress = 0xDEAD0000u;
    static constexpr const char*    kName        = "uart0";
    static constexpr const char*    kTemplate    = "uart";
    static constexpr const char*    kIpVersion   = "uart_6088";
    static constexpr unsigned       kIrqLines[]  = { 7u };
    static constexpr unsigned       kIrqCount    = 1u;
};

}  // namespace mock

// ============================================================================
// Aliases
// ============================================================================

namespace {

using namespace alloy::hal::uart::lite;

using Sci3Port  = port<mock::usart_sci3>;
using Sci2Port  = port<mock::usart_sci2>;
using SamPort   = port<mock::sam_uart>;

// ============================================================================
// Task 3.1 — New types
// ============================================================================

// 3.1.1 UartErrors: 4 bool fields + any()
static_assert(std::is_same_v<bool, decltype(std::declval<UartErrors>().parity)>);
static_assert(std::is_same_v<bool, decltype(std::declval<UartErrors>().framing)>);
static_assert(std::is_same_v<bool, decltype(std::declval<UartErrors>().noise)>);
static_assert(std::is_same_v<bool, decltype(std::declval<UartErrors>().overrun)>);
static_assert(noexcept(std::declval<UartErrors>().any()));
static_assert(std::is_same_v<bool, decltype(std::declval<const UartErrors>().any())>);

// 3.1.2 FifoTrigger enum
static_assert(std::is_same_v<std::uint8_t, std::underlying_type_t<FifoTrigger>>);
static_assert(static_cast<std::uint8_t>(FifoTrigger::Empty)         == 0u);
static_assert(static_cast<std::uint8_t>(FifoTrigger::Quarter)       == 1u);
static_assert(static_cast<std::uint8_t>(FifoTrigger::Half)          == 2u);
static_assert(static_cast<std::uint8_t>(FifoTrigger::ThreeQuarters) == 3u);
static_assert(static_cast<std::uint8_t>(FifoTrigger::Full)          == 4u);

// 3.1.3 InterruptKind enum
static_assert(std::is_same_v<std::uint8_t, std::underlying_type_t<InterruptKind>>);
static_assert(static_cast<std::uint8_t>(InterruptKind::Tc)              == 0u);
static_assert(static_cast<std::uint8_t>(InterruptKind::Txe)             == 1u);
static_assert(static_cast<std::uint8_t>(InterruptKind::Rxne)            == 2u);
static_assert(static_cast<std::uint8_t>(InterruptKind::IdleLine)        == 3u);
static_assert(static_cast<std::uint8_t>(InterruptKind::LinBreak)        == 4u);
static_assert(static_cast<std::uint8_t>(InterruptKind::Cts)             == 5u);
static_assert(static_cast<std::uint8_t>(InterruptKind::Error)           == 6u);
static_assert(static_cast<std::uint8_t>(InterruptKind::RxFifoThreshold) == 7u);
static_assert(static_cast<std::uint8_t>(InterruptKind::TxFifoThreshold) == 8u);

// 3.1.4 Oversampling enum
static_assert(std::is_same_v<std::uint8_t, std::underlying_type_t<Oversampling>>);
static_assert(static_cast<std::uint8_t>(Oversampling::X16) == 0u);
static_assert(static_cast<std::uint8_t>(Oversampling::X8)  == 1u);

// ============================================================================
// Task 3.2 — detail:: bit constants
// ============================================================================

// 3.2.1 CR3 bits
static_assert(detail::kCr3Dmar  == (1u << 6));
static_assert(detail::kCr3Dmat  == (1u << 7));
static_assert(detail::kCr3Rtse  == (1u << 8));
static_assert(detail::kCr3Ctse  == (1u << 9));
static_assert(detail::kCr3Hdsel == (1u << 3));
static_assert(detail::kCr3Dem   == (1u << 14));
static_assert(detail::kCr3Dep   == (1u << 15));
static_assert(detail::kCr3Iren  == (1u << 1));
static_assert(detail::kCr3Scen  == (1u << 5));

// 3.2.2 SCI3 CR1 extended bits
static_assert(detail::kSci3Cr1Fifoen   == (1u << 29));
static_assert(detail::kSci3Cr1Uesm    == (1u << 23));
static_assert(detail::kSci3Cr1Over8   == (1u << 15));
static_assert(detail::kSci3Cr1DeatMask == (0x1Fu << 21u));
static_assert(detail::kSci3Cr1DedtMask == (0x1Fu << 16u));
static_assert(detail::kSci3Cr1DeatShift == 21u);
static_assert(detail::kSci3Cr1DedtShift == 16u);

// 3.2.3 CR2 bits
static_assert(detail::kCr2Linen == (1u << 14));
static_assert(detail::kCr2Lbdl  == (1u << 5));
static_assert(detail::kCr2Lbdie == (1u << 6));
static_assert(detail::kCr2Clken == (1u << 11));

// 3.2.4 ISR/SR extended bits
static_assert(detail::kIsrLbdf == (1u << 8));
static_assert(detail::kIsrRxff == (1u << 24));
static_assert(detail::kIsrTxff == (1u << 25));

// 3.2.5 ICR extended bit
static_assert(detail::kIcrLbdcf == (1u << 8));

// 3.2.6 RQR bit
static_assert(detail::kRqrSbkrq == (1u << 1));

// 3.2.7 FIFO threshold shifts and masks
static_assert(detail::kCr3TxftcfgShift == 29u);
static_assert(detail::kCr3RxftcfgShift == 24u);
static_assert(detail::kCr3TxftcfgMask  == (0x7u << 29u));
static_assert(detail::kCr3RxftcfgMask  == (0x7u << 24u));

// ============================================================================
// Task 3.3 — Method return types (SCI3 port)
// ============================================================================

// 3.3.1 read_and_clear_errors() → UartErrors (all variants)
static_assert(std::is_same_v<UartErrors, decltype(Sci3Port::read_and_clear_errors())>);
static_assert(std::is_same_v<UartErrors, decltype(Sci2Port::read_and_clear_errors())>);
static_assert(std::is_same_v<UartErrors, decltype(SamPort::read_and_clear_errors())>);

// 3.3.2 error_flags() → UartErrors (all variants)
static_assert(std::is_same_v<UartErrors, decltype(Sci3Port::error_flags())>);
static_assert(std::is_same_v<UartErrors, decltype(Sci2Port::error_flags())>);
static_assert(std::is_same_v<UartErrors, decltype(SamPort::error_flags())>);

// 3.3.3 enable_hardware_flow_control() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_hardware_flow_control(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::enable_hardware_flow_control(false))>);
static_assert(!requires { SamPort::enable_hardware_flow_control(true); },
    "SamUart must NOT have enable_hardware_flow_control");

// 3.3.4 set_de_polarity() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::set_de_polarity(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::set_de_polarity(false))>);
static_assert(!requires { SamPort::set_de_polarity(true); },
    "SamUart must NOT have set_de_polarity");

// 3.3.5 enable_de() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_de(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::enable_de(false))>);
static_assert(!requires { SamPort::enable_de(true); },
    "SamUart must NOT have enable_de");

// 3.3.6 set_de_assertion_time() → void (SCI3 only)
static_assert(std::is_same_v<void,
    decltype(Sci3Port::set_de_assertion_time(std::uint8_t{5}))>);
static_assert(!requires { Sci2Port::set_de_assertion_time(std::uint8_t{5}); },
    "SCI2 must NOT have set_de_assertion_time");
static_assert(!requires { SamPort::set_de_assertion_time(std::uint8_t{5}); },
    "SamUart must NOT have set_de_assertion_time");

// 3.3.7 set_de_deassertion_time() → void (SCI3 only)
static_assert(std::is_same_v<void,
    decltype(Sci3Port::set_de_deassertion_time(std::uint8_t{3}))>);
static_assert(!requires { Sci2Port::set_de_deassertion_time(std::uint8_t{3}); },
    "SCI2 must NOT have set_de_deassertion_time");
static_assert(!requires { SamPort::set_de_deassertion_time(std::uint8_t{3}); },
    "SamUart must NOT have set_de_deassertion_time");

// 3.3.8 set_half_duplex() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::set_half_duplex(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::set_half_duplex(false))>);
static_assert(!requires { SamPort::set_half_duplex(true); },
    "SamUart must NOT have set_half_duplex");

// 3.3.9 enable_lin() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_lin(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::enable_lin(false))>);
static_assert(!requires { SamPort::enable_lin(true); },
    "SamUart must NOT have enable_lin");

// 3.3.10 set_lin_break_length() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::set_lin_break_length(false))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::set_lin_break_length(true))>);
static_assert(!requires { SamPort::set_lin_break_length(true); },
    "SamUart must NOT have set_lin_break_length");

// 3.3.11 send_lin_break() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::send_lin_break())>);
static_assert(std::is_same_v<void, decltype(Sci2Port::send_lin_break())>);
static_assert(!requires { SamPort::send_lin_break(); },
    "SamUart must NOT have send_lin_break");

// 3.3.12 lin_break_detected() → bool (StUsart only)
static_assert(std::is_same_v<bool, decltype(Sci3Port::lin_break_detected())>);
static_assert(std::is_same_v<bool, decltype(Sci2Port::lin_break_detected())>);
static_assert(!requires { SamPort::lin_break_detected(); },
    "SamUart must NOT have lin_break_detected");

// 3.3.13 clear_lin_break_flag() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::clear_lin_break_flag())>);
static_assert(std::is_same_v<void, decltype(Sci2Port::clear_lin_break_flag())>);
static_assert(!requires { SamPort::clear_lin_break_flag(); },
    "SamUart must NOT have clear_lin_break_flag");

// 3.3.14 enable_lin_break_irq() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_lin_break_irq(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::enable_lin_break_irq(false))>);
static_assert(!requires { SamPort::enable_lin_break_irq(true); },
    "SamUart must NOT have enable_lin_break_irq");

// 3.3.15 set_smartcard_mode() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::set_smartcard_mode(false))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::set_smartcard_mode(false))>);
static_assert(!requires { SamPort::set_smartcard_mode(true); },
    "SamUart must NOT have set_smartcard_mode");

// 3.3.16 set_irda_mode() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::set_irda_mode(false))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::set_irda_mode(false))>);
static_assert(!requires { SamPort::set_irda_mode(true); },
    "SamUart must NOT have set_irda_mode");

// 3.3.17 enable_fifo() → void (SCI3 only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_fifo(true))>);
static_assert(!requires { Sci2Port::enable_fifo(true); },
    "SCI2 must NOT have enable_fifo");
static_assert(!requires { SamPort::enable_fifo(true); },
    "SamUart must NOT have enable_fifo");

// 3.3.18 set_tx_fifo_threshold() → void (SCI3 only)
static_assert(std::is_same_v<void,
    decltype(Sci3Port::set_tx_fifo_threshold(FifoTrigger::Half))>);
static_assert(!requires { Sci2Port::set_tx_fifo_threshold(FifoTrigger::Half); },
    "SCI2 must NOT have set_tx_fifo_threshold");
static_assert(!requires { SamPort::set_tx_fifo_threshold(FifoTrigger::Half); },
    "SamUart must NOT have set_tx_fifo_threshold");

// 3.3.19 set_rx_fifo_threshold() → void (SCI3 only)
static_assert(std::is_same_v<void,
    decltype(Sci3Port::set_rx_fifo_threshold(FifoTrigger::Quarter))>);
static_assert(!requires { Sci2Port::set_rx_fifo_threshold(FifoTrigger::Quarter); },
    "SCI2 must NOT have set_rx_fifo_threshold");
static_assert(!requires { SamPort::set_rx_fifo_threshold(FifoTrigger::Quarter); },
    "SamUart must NOT have set_rx_fifo_threshold");

// 3.3.20 tx_fifo_full() → bool (SCI3 only)
static_assert(std::is_same_v<bool, decltype(Sci3Port::tx_fifo_full())>);
static_assert(!requires { Sci2Port::tx_fifo_full(); },
    "SCI2 must NOT have tx_fifo_full");
static_assert(!requires { SamPort::tx_fifo_full(); },
    "SamUart must NOT have tx_fifo_full");

// 3.3.21 rx_fifo_full() → bool (SCI3 only)
static_assert(std::is_same_v<bool, decltype(Sci3Port::rx_fifo_full())>);
static_assert(!requires { Sci2Port::rx_fifo_full(); },
    "SCI2 must NOT have rx_fifo_full");
static_assert(!requires { SamPort::rx_fifo_full(); },
    "SamUart must NOT have rx_fifo_full");

// 3.3.22 enable_dma_tx() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_dma_tx(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::enable_dma_tx(false))>);
static_assert(!requires { SamPort::enable_dma_tx(true); },
    "SamUart must NOT have enable_dma_tx");

// 3.3.23 enable_dma_rx() → void (StUsart only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_dma_rx(true))>);
static_assert(std::is_same_v<void, decltype(Sci2Port::enable_dma_rx(false))>);
static_assert(!requires { SamPort::enable_dma_rx(true); },
    "SamUart must NOT have enable_dma_rx");

// 3.3.24 enable_interrupt(InterruptKind, bool) → void (StUsart only)
static_assert(std::is_same_v<void,
    decltype(Sci3Port::enable_interrupt(InterruptKind::Rxne, true))>);
static_assert(std::is_same_v<void,
    decltype(Sci2Port::enable_interrupt(InterruptKind::Tc, false))>);
static_assert(!requires { SamPort::enable_interrupt(InterruptKind::Rxne, true); },
    "SamUart must NOT have enable_interrupt(InterruptKind, bool)");

// 3.3.25 enable_wakeup_from_stop() → void (SCI3 only)
static_assert(std::is_same_v<void, decltype(Sci3Port::enable_wakeup_from_stop(true))>);
static_assert(!requires { Sci2Port::enable_wakeup_from_stop(true); },
    "SCI2 must NOT have enable_wakeup_from_stop");
static_assert(!requires { SamPort::enable_wakeup_from_stop(true); },
    "SamUart must NOT have enable_wakeup_from_stop");

// 3.3.26 set_oversampling() → void (StUsart only)
static_assert(std::is_same_v<void,
    decltype(Sci3Port::set_oversampling(Oversampling::X8))>);
static_assert(std::is_same_v<void,
    decltype(Sci2Port::set_oversampling(Oversampling::X16))>);
static_assert(!requires { SamPort::set_oversampling(Oversampling::X8); },
    "SamUart must NOT have set_oversampling");

// ============================================================================
// noexcept checks — all MMIO methods are noexcept
// ============================================================================

static_assert(noexcept(Sci3Port::read_and_clear_errors()));
static_assert(noexcept(Sci3Port::error_flags()));
static_assert(noexcept(Sci3Port::enable_hardware_flow_control(true)));
static_assert(noexcept(Sci3Port::set_de_polarity(true)));
static_assert(noexcept(Sci3Port::enable_de(true)));
static_assert(noexcept(Sci3Port::set_de_assertion_time(std::uint8_t{4})));
static_assert(noexcept(Sci3Port::set_de_deassertion_time(std::uint8_t{4})));
static_assert(noexcept(Sci3Port::set_half_duplex(false)));
static_assert(noexcept(Sci3Port::enable_lin(false)));
static_assert(noexcept(Sci3Port::set_lin_break_length(false)));
static_assert(noexcept(Sci3Port::send_lin_break()));
static_assert(noexcept(Sci3Port::lin_break_detected()));
static_assert(noexcept(Sci3Port::clear_lin_break_flag()));
static_assert(noexcept(Sci3Port::enable_lin_break_irq(false)));
static_assert(noexcept(Sci3Port::set_smartcard_mode(false)));
static_assert(noexcept(Sci3Port::set_irda_mode(false)));
static_assert(noexcept(Sci3Port::enable_fifo(false)));
static_assert(noexcept(Sci3Port::set_tx_fifo_threshold(FifoTrigger::Empty)));
static_assert(noexcept(Sci3Port::set_rx_fifo_threshold(FifoTrigger::Full)));
static_assert(noexcept(Sci3Port::tx_fifo_full()));
static_assert(noexcept(Sci3Port::rx_fifo_full()));
static_assert(noexcept(Sci3Port::enable_dma_tx(true)));
static_assert(noexcept(Sci3Port::enable_dma_rx(true)));
static_assert(noexcept(Sci3Port::enable_interrupt(InterruptKind::Error, true)));
static_assert(noexcept(Sci3Port::enable_wakeup_from_stop(false)));
static_assert(noexcept(Sci3Port::set_oversampling(Oversampling::X8)));

// ============================================================================
// Concepts verify correctly
// ============================================================================

static_assert( StModernUsart<mock::usart_sci3>);
static_assert(!StModernUsart<mock::usart_sci2>);
static_assert(!StModernUsart<mock::sam_uart>);

static_assert(!StLegacyUsart<mock::usart_sci3>);
static_assert( StLegacyUsart<mock::usart_sci2>);
static_assert(!StLegacyUsart<mock::sam_uart>);

static_assert( StUsart<mock::usart_sci3>);
static_assert( StUsart<mock::usart_sci2>);
static_assert(!StUsart<mock::sam_uart>);

static_assert(!SamUart<mock::usart_sci3>);
static_assert(!SamUart<mock::usart_sci2>);
static_assert( SamUart<mock::sam_uart>);

static_assert( AnyUart<mock::usart_sci3>);
static_assert( AnyUart<mock::usart_sci2>);
static_assert( AnyUart<mock::sam_uart>);

}  // namespace
