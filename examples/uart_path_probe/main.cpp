#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#if !defined(ALLOY_BOARD_SAME70_XPLAINED) && !defined(ALLOY_BOARD_SAME70_XPLD)
    #error "uart_path_probe is SAME70-only"
#endif

#include "hal/systick.hpp"
#include "hal/uart.hpp"

namespace {

// This is an expert SAME70 route probe. The canonical examples stay on board::make_*().

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
using ProbeConnector = alloy::hal::uart::route<
    alloy::dev::periph::USART0,
    alloy::hal::tx<alloy::dev::pin::PB1, alloy::dev::sig::signal_txd0>,
    alloy::hal::rx<alloy::dev::pin::PB0, alloy::dev::sig::signal_rxd0>>;
constexpr auto kBanner = "same70 probe USART0 PB1/PB0\r\n";
constexpr auto kBlinkMs = 111u;
#elif defined(ALLOY_UART_PROBE_UART1_PA56)
using ProbeConnector = alloy::hal::uart::route<
    alloy::dev::periph::UART1,
    alloy::hal::tx<alloy::dev::pin::PA6, alloy::dev::sig::signal_utxd1>,
    alloy::hal::rx<alloy::dev::pin::PA5, alloy::dev::sig::signal_urxd1>>;
constexpr auto kBanner = "same70 probe UART1 PA6/PA5\r\n";
constexpr auto kBlinkMs = 222u;
#elif defined(ALLOY_UART_PROBE_USART1_PB4_PA21)
using ProbeConnector = alloy::hal::uart::route<
    alloy::dev::periph::USART1,
    alloy::hal::tx<alloy::dev::pin::PB4, alloy::dev::sig::signal_txd1>,
    alloy::hal::rx<alloy::dev::pin::PA21, alloy::dev::sig::signal_rxd1>>;
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
