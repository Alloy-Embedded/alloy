/// @file tests/compile_tests/test_sam_uart_lite.cpp
/// Compile-time verification of SAME70 UART dispatch in uart/lite.hpp (task 2.2.6).
///
/// Uses a self-contained mock PeripheralSpec — no real device artifact needed.
/// Verifies:
///   - SamUart concept gates on kTemplate == "uart"
///   - AnyUart accepts both STM32 USART and SAME70 UART specs
///   - port<P> instantiates for SAME70 without error
///   - irq_number() / irq_count() work as per the device-data bridge
///   - All public method return types compile correctly

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

/// Simulates a SAME70 UART0 flat-struct entry (kTemplate == "uart").
struct uart0 {
    static constexpr std::uintptr_t kBaseAddress = 0xDEAD0000u;
    static constexpr const char*    kName        = "uart0";
    static constexpr const char*    kTemplate    = "uart";
    static constexpr const char*    kIpVersion   = "uart_6088";  ///< SAME70 UART IP
    static constexpr unsigned       kIrqLines[]  = { 7u };       ///< UART0 IRQ on SAME70
    static constexpr unsigned       kIrqCount    = 1u;
};

/// Simulates an STM32 USART for contrast.
struct usart1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40013800u;
    static constexpr const char*    kName        = "usart1";
    static constexpr const char*    kTemplate    = "usart";
    static constexpr const char*    kIpVersion   = "sci3_v1_3";
    static constexpr unsigned       kIrqLines[]  = { 37u };
    static constexpr unsigned       kIrqCount    = 1u;
};

/// A non-UART peripheral — must NOT satisfy SamUart.
struct spi0 {
    static constexpr std::uintptr_t kBaseAddress = 0x40008000u;
    static constexpr const char*    kName        = "spi0";
    static constexpr const char*    kTemplate    = "spi";
    static constexpr const char*    kIpVersion   = "spi_6088";
};

}  // namespace mock

// ============================================================================
// Concept checks
// ============================================================================

namespace {

// SamUart accepts kTemplate == "uart"
static_assert( alloy::hal::uart::lite::SamUart<mock::uart0>,
    "mock::uart0 must satisfy SamUart");

// SamUart rejects other templates
static_assert(!alloy::hal::uart::lite::SamUart<mock::usart1>,
    "mock::usart1 must NOT satisfy SamUart");
static_assert(!alloy::hal::uart::lite::SamUart<mock::spi0>,
    "mock::spi0 must NOT satisfy SamUart");

// AnyUart accepts both variants
static_assert( alloy::hal::uart::lite::AnyUart<mock::uart0>,
    "mock::uart0 must satisfy AnyUart");
static_assert( alloy::hal::uart::lite::AnyUart<mock::usart1>,
    "mock::usart1 must satisfy AnyUart");

// ============================================================================
// Instantiation checks — port<uart0>
// ============================================================================

using Uart0 = alloy::hal::uart::lite::port<mock::uart0>;
using Uart1 = alloy::hal::uart::lite::port<mock::usart1>;

// configure() compiles (noexcept not required — it is noexcept but not mandated by spec)
static_assert(std::is_same_v<void,
    decltype(Uart0::configure(alloy::hal::uart::lite::Config{}))>);

// tx_ready / rx_ready → bool
static_assert(std::is_same_v<bool, decltype(Uart0::tx_ready())>);
static_assert(std::is_same_v<bool, decltype(Uart0::rx_ready())>);

// Aliases also compile
static_assert(std::is_same_v<bool, decltype(Uart0::ready_to_send())>);
static_assert(std::is_same_v<bool, decltype(Uart0::data_available())>);

// write_byte / read_byte
static_assert(std::is_same_v<void, decltype(Uart0::write_byte(std::byte{}))>);
static_assert(std::is_same_v<std::byte, decltype(Uart0::read_byte())>);

// try_read_byte → optional<byte>
static_assert(std::is_same_v<std::optional<std::byte>, decltype(Uart0::try_read_byte())>);

// try_write_byte → bool
static_assert(std::is_same_v<bool, decltype(Uart0::try_write_byte(std::byte{}))>);

// enabled / disable
static_assert(std::is_same_v<bool, decltype(Uart0::enabled())>);
static_assert(std::is_same_v<void, decltype(Uart0::disable())>);

// error methods → bool
static_assert(std::is_same_v<bool, decltype(Uart0::frame_error())>);
static_assert(std::is_same_v<bool, decltype(Uart0::overrun())>);
static_assert(std::is_same_v<bool, decltype(Uart0::parity_error())>);
static_assert(std::is_same_v<bool, decltype(Uart0::noise_error())>);  // always false on SAME70
static_assert(std::is_same_v<bool, decltype(Uart0::has_errors())>);

// clear_errors
static_assert(std::is_same_v<void, decltype(Uart0::clear_errors())>);

// flush
static_assert(std::is_same_v<void, decltype(Uart0::flush())>);

// IRQ control — void return
static_assert(std::is_same_v<void, decltype(Uart0::enable_rx_irq())>);
static_assert(std::is_same_v<void, decltype(Uart0::enable_tx_irq())>);
static_assert(std::is_same_v<void, decltype(Uart0::enable_tc_irq())>);
static_assert(std::is_same_v<void, decltype(Uart0::enable_error_irq())>);
static_assert(std::is_same_v<void, decltype(Uart0::disable_irqs())>);

// ============================================================================
// Device-data bridge — irq_number / irq_count
// ============================================================================

// SAME70 uart0: irq_number() → 7u, irq_count() → 1
static_assert(std::is_same_v<std::uint32_t, decltype(Uart0::irq_number())>);
static_assert(std::is_same_v<std::size_t,   decltype(Uart0::irq_count())>);
static_assert(Uart0::irq_number() == 7u);
static_assert(Uart0::irq_count()  == 1u);

// STM32 usart1: still works
static_assert(Uart1::irq_number() == 37u);
static_assert(Uart1::irq_count()  == 1u);

// noexcept
static_assert(noexcept(Uart0::irq_number()));
static_assert(noexcept(Uart0::irq_count()));

}  // namespace
