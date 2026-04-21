#include <cstddef>

#include "hal/dma.hpp"

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
#include "hal/adc.hpp"
#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/uart.hpp"

namespace {

using DmaPeripheralId = alloy::hal::dma::PeripheralId;
using DmaSignalId = alloy::hal::dma::SignalId;
using UartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART1">, alloy::device::runtime::PeripheralId::USART1,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<alloy::hal::pin<"PA9">>,
                                            alloy::device::runtime::PinId::PA9,
                                            alloy::device::runtime::SignalId::signal_tx>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<alloy::hal::pin<"PA10">>,
                                            alloy::device::runtime::PinId::PA10,
                                            alloy::device::runtime::SignalId::signal_rx>>;

}  // namespace

void compile_peripheral_dma_api() {
    auto uart = alloy::hal::uart::open<UartConnector>({});
    auto uart_tx_dma = alloy::hal::dma::open<DmaPeripheralId::USART1, DmaSignalId::signal_TX>(
        {.direction = alloy::hal::dma::Direction::memory_to_peripheral});
    [[maybe_unused]] const auto uart_tx_result = uart.configure_tx_dma(uart_tx_dma);
    [[maybe_unused]] constexpr auto uart_tx_addr = decltype(uart)::tx_data_register_address();
    [[maybe_unused]] constexpr auto uart_rx_addr = decltype(uart)::rx_data_register_address();

    auto adc = alloy::hal::adc::open<alloy::hal::adc::PeripheralId::ADC1>({});
    [[maybe_unused]] const auto adc_dma_enable = adc.enable_dma(true);
    [[maybe_unused]] const auto adc_dma_disable = adc.disable_dma();
    [[maybe_unused]] constexpr auto adc_addr = decltype(adc)::data_register_address();
}

#elif defined(ALLOY_BOARD_NUCLEO_F401RE)

#include "hal/adc.hpp"
#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/uart.hpp"

namespace {

using DmaPeripheralId = alloy::hal::dma::PeripheralId;
using DmaSignalId = alloy::hal::dma::SignalId;
using UartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<alloy::hal::pin<"PA2">>,
                                            alloy::device::runtime::PinId::PA2,
                                            alloy::device::runtime::SignalId::signal_tx>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<alloy::hal::pin<"PA3">>,
                                            alloy::device::runtime::PinId::PA3,
                                            alloy::device::runtime::SignalId::signal_rx>>;

}  // namespace

void compile_peripheral_dma_api() {
    auto uart = alloy::hal::uart::open<UartConnector>({});
    auto uart_tx_dma = alloy::hal::dma::open<DmaPeripheralId::USART2, DmaSignalId::signal_TX>(
        {.direction = alloy::hal::dma::Direction::memory_to_peripheral});
    [[maybe_unused]] const auto uart_tx_result = uart.configure_tx_dma(uart_tx_dma);
    [[maybe_unused]] constexpr auto uart_tx_addr = decltype(uart)::tx_data_register_address();
    [[maybe_unused]] constexpr auto uart_rx_addr = decltype(uart)::rx_data_register_address();

    auto adc = alloy::hal::adc::open<alloy::hal::adc::PeripheralId::ADC1>({});
    [[maybe_unused]] const auto adc_dma_enable = adc.enable_dma(true);
    [[maybe_unused]] const auto adc_dma_disable = adc.disable_dma();
    [[maybe_unused]] constexpr auto adc_addr = decltype(adc)::data_register_address();
}

#elif defined(ALLOY_BOARD_SAME70_XPLD)

#include "hal/adc.hpp"
#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/dac.hpp"
#include "hal/i2c.hpp"
#include "hal/pwm.hpp"
#include "hal/spi.hpp"
#include "hal/uart.hpp"

namespace {

using DmaPeripheralId = alloy::hal::dma::PeripheralId;
using DmaSignalId = alloy::hal::dma::SignalId;
using UartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART0">, alloy::device::runtime::PeripheralId::USART0,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<alloy::hal::pin<"PB1">>,
                                            alloy::device::runtime::PinId::PB1,
                                            alloy::device::runtime::SignalId::signal_txd0>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<alloy::hal::pin<"PB0">>,
                                            alloy::device::runtime::PinId::PB0,
                                            alloy::device::runtime::SignalId::signal_rxd0>>;
using SpiConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"SPI0">, alloy::device::runtime::PeripheralId::SPI0,
    alloy::hal::connection::runtime_binding<alloy::hal::sck<alloy::hal::pin<"PD22">>,
                                            alloy::device::runtime::PinId::PD22,
                                            alloy::device::runtime::SignalId::signal_spck>,
    alloy::hal::connection::runtime_binding<alloy::hal::miso<alloy::hal::pin<"PD20">>,
                                            alloy::device::runtime::PinId::PD20,
                                            alloy::device::runtime::SignalId::signal_miso>,
    alloy::hal::connection::runtime_binding<alloy::hal::mosi<alloy::hal::pin<"PD21">>,
                                            alloy::device::runtime::PinId::PD21,
                                            alloy::device::runtime::SignalId::signal_mosi>>;
using I2cConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"TWIHS0">, alloy::device::runtime::PeripheralId::TWIHS0,
    alloy::hal::connection::runtime_binding<alloy::hal::scl<alloy::hal::pin<"PA4">>,
                                            alloy::device::runtime::PinId::PA4,
                                            alloy::device::runtime::SignalId::signal_twck0>,
    alloy::hal::connection::runtime_binding<alloy::hal::sda<alloy::hal::pin<"PA3">>,
                                            alloy::device::runtime::PinId::PA3,
                                            alloy::device::runtime::SignalId::signal_twd0>>;

}  // namespace

void compile_peripheral_dma_api() {
    auto uart = alloy::hal::uart::open<UartConnector>({});
    auto uart_tx_dma = alloy::hal::dma::open<DmaPeripheralId::USART0, DmaSignalId::signal_TX>({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
        .channel_index = 0,
    });
    auto uart_rx_dma = alloy::hal::dma::open<DmaPeripheralId::USART0, DmaSignalId::signal_RX>({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
        .channel_index = 1,
    });
    [[maybe_unused]] const auto uart_tx_result = uart.configure_tx_dma(uart_tx_dma);
    [[maybe_unused]] const auto uart_rx_result = uart.configure_rx_dma(uart_rx_dma);

    auto spi = alloy::hal::spi::open<SpiConnector>({});
    auto spi_tx_dma = alloy::hal::dma::open<DmaPeripheralId::SPI0, DmaSignalId::signal_TX>({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
        .channel_index = 0,
    });
    auto spi_rx_dma = alloy::hal::dma::open<DmaPeripheralId::SPI0, DmaSignalId::signal_RX>({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
        .channel_index = 1,
    });
    [[maybe_unused]] const auto spi_tx_result = spi.configure_tx_dma(spi_tx_dma);
    [[maybe_unused]] const auto spi_rx_result = spi.configure_rx_dma(spi_rx_dma);

    auto i2c = alloy::hal::i2c::open<I2cConnector>({});
    auto i2c_tx_dma = alloy::hal::dma::open<DmaPeripheralId::TWIHS0, DmaSignalId::signal_TX>({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
        .channel_index = 0,
    });
    auto i2c_rx_dma = alloy::hal::dma::open<DmaPeripheralId::TWIHS0, DmaSignalId::signal_RX>({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
        .channel_index = 1,
    });
    [[maybe_unused]] const auto i2c_tx_result = i2c.configure_tx_dma(i2c_tx_dma);
    [[maybe_unused]] const auto i2c_rx_result = i2c.configure_rx_dma(i2c_rx_dma);

    auto adc = alloy::hal::adc::open<alloy::hal::adc::PeripheralId::AFEC0>({});
    auto adc_dma = alloy::hal::dma::open<DmaPeripheralId::AFEC0, DmaSignalId::signal_RX>({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
        .channel_index = 0,
    });
    [[maybe_unused]] const auto adc_dma_result = adc.configure_dma(adc_dma);

    auto dac = alloy::hal::dac::open<alloy::hal::dac::PeripheralId::DACC, 0u>({});
    auto dac_dma = alloy::hal::dma::open<DmaPeripheralId::DACC, DmaSignalId::signal_CH0_TX>({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
        .channel_index = 0,
    });
    [[maybe_unused]] const auto dac_dma_result = dac.configure_dma(dac_dma);

    auto pwm = alloy::hal::pwm::open<alloy::hal::pwm::PeripheralId::PWM0, 0u>({});
    auto pwm_dma = alloy::hal::dma::open<DmaPeripheralId::PWM0, DmaSignalId::signal_TX>({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
        .channel_index = 0,
    });
    [[maybe_unused]] const auto pwm_dma_result = pwm.configure_dma(pwm_dma);
}

#endif
