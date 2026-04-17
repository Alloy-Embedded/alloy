#include <array>
#include <string_view>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/uart.hpp"

#include "device/traits.hpp"

namespace {

template <typename UartHandle>
consteval auto uart_is_usable() -> bool {
    if constexpr (!UartHandle::valid) {
        return false;
    }

    return !UartHandle::peripheral_name.empty() && UartHandle::base_address() != 0u &&
           UartHandle::operations().size() >= 2;
}

template <typename UartHandle>
void exercise_uart_backend(std::uint32_t peripheral_clock_hz) {
    auto uart = UartHandle{
        {
            .baudrate = alloy::hal::Baudrate::e115200,
            .data_bits = alloy::hal::DataBits::Eight,
            .parity = alloy::hal::Parity::None,
            .stop_bits = alloy::hal::StopBits::One,
            .flow_control = alloy::hal::FlowControl::None,
            .peripheral_clock_hz = peripheral_clock_hz,
        },
    };

    std::array<std::byte, 3> tx_buffer{
        std::byte{0x41},
        std::byte{0x6Cu},
        std::byte{0x6Cu},
    };
    std::array<std::byte, 4> rx_buffer{};

    [[maybe_unused]] const auto configure_result = uart.configure();
    [[maybe_unused]] const auto write_result = uart.write(std::span<const std::byte>{tx_buffer});
    [[maybe_unused]] const auto write_byte_result = uart.write_byte(std::byte{0x79});
    [[maybe_unused]] const auto read_result = uart.read(std::span<std::byte>{rx_buffer});
    [[maybe_unused]] const auto flush_result = uart.flush();
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using DebugUartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<alloy::hal::pin<"PA2">>,
                                            alloy::device::runtime::PinId::PA2,
                                            alloy::device::runtime::SignalId::signal_tx>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<alloy::hal::pin<"PA3">>,
                                            alloy::device::runtime::PinId::PA3,
                                            alloy::device::runtime::SignalId::signal_rx>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART2"});
static_assert(uart_is_usable<DebugUart>());
[[maybe_unused]] void compile_g071_uart_backend() {
    exercise_uart_backend<DebugUart>(64'000'000u);
}
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using DebugUartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<alloy::hal::pin<"PA2">>,
                                            alloy::device::runtime::PinId::PA2,
                                            alloy::device::runtime::SignalId::signal_tx>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<alloy::hal::pin<"PA3">>,
                                            alloy::device::runtime::PinId::PA3,
                                            alloy::device::runtime::SignalId::signal_rx>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART2"});
static_assert(uart_is_usable<DebugUart>());
[[maybe_unused]] void compile_f401_uart_backend() {
    exercise_uart_backend<DebugUart>(42'000'000u);
}
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using DebugUartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART0">, alloy::device::runtime::PeripheralId::USART0,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<alloy::hal::pin<"PB1">>,
                                            alloy::device::runtime::PinId::PB1,
                                            alloy::device::runtime::SignalId::signal_txd0>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<alloy::hal::pin<"PB0">>,
                                            alloy::device::runtime::PinId::PB0,
                                            alloy::device::runtime::SignalId::signal_rxd0>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART0"});
static_assert(uart_is_usable<DebugUart>());
[[maybe_unused]] void compile_same70_uart_backend() {
    exercise_uart_backend<DebugUart>(12'000'000u);
}
#endif
