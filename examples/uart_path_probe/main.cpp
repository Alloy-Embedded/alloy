#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#if !defined(ALLOY_BOARD_SAME70_XPLAINED) && !defined(ALLOY_BOARD_SAME70_XPLD)
    #error "uart_path_probe is SAME70-only"
#endif

#include "hal/connect/connector.hpp"
#include "hal/systick.hpp"
#include "hal/uart.hpp"

namespace {

template <typename Uart>
[[noreturn]] void blink_loop(Uart& uart, const char* banner, std::uint32_t period_ms) {
    auto length = std::size_t{0};
    while (banner[length] != '\0') {
        ++length;
    }
    const auto bytes =
        std::span{reinterpret_cast<const std::byte*>(banner), static_cast<std::size_t>(length)};
    static_cast<void>(uart.write(bytes));
    static_cast<void>(uart.flush());

    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

#if defined(ALLOY_UART_PROBE_USART0_PB01)
using ProbeConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART0,
    alloy::hal::connection::tx<alloy::device::PinId::PB1, alloy::device::SignalId::signal_txd0>,
    alloy::hal::connection::rx<alloy::device::PinId::PB0, alloy::device::SignalId::signal_rxd0>>;
constexpr auto kBanner = "same70 probe USART0 PB1/PB0\r\n";
constexpr auto kBlinkMs = 111u;
#elif defined(ALLOY_UART_PROBE_UART1_PA56)
using ProbeConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::UART1,
    alloy::hal::connection::tx<alloy::device::PinId::PA6, alloy::device::SignalId::signal_utxd1>,
    alloy::hal::connection::rx<alloy::device::PinId::PA5, alloy::device::SignalId::signal_urxd1>>;
constexpr auto kBanner = "same70 probe UART1 PA6/PA5\r\n";
constexpr auto kBlinkMs = 222u;
#elif defined(ALLOY_UART_PROBE_USART1_PB4_PA21)
using ProbeConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART1,
    alloy::hal::connection::tx<alloy::device::PinId::PB4, alloy::device::SignalId::signal_txd1>,
    alloy::hal::connection::rx<alloy::device::PinId::PA21, alloy::device::SignalId::signal_rxd1>>;
constexpr auto kBanner = "same70 probe USART1 PB4/PA21\r\n";
constexpr auto kBlinkMs = 333u;
#else
    #error "Select one ALLOY_UART_PROBE_* candidate"
#endif

using ProbeUart = alloy::hal::uart::port_handle<ProbeConnector>;

}  // namespace

int main() {
    board::init();

    auto uart = ProbeUart{{.peripheral_clock_hz = board::same70_xplained::ClockConfig::pclk_freq_hz}};
    if (const auto result = uart.configure(); result.is_err()) {
        while (true) {
            board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(50);
        }
    }

    blink_loop(uart, kBanner, kBlinkMs);
}
