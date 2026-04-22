#include <array>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/spi.hpp"

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5, alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5, alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI0,
    alloy::hal::connection::sck<alloy::device::PinId::PD22, alloy::device::SignalId::signal_spck>,
    alloy::hal::connection::miso<alloy::device::PinId::PD20,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PD21,
                                 alloy::device::SignalId::signal_mosi>>;
#endif

#if defined(SpiConnector)
void compile_spi_shared_bus() {
    auto bus = alloy::hal::spi::open_shared_bus<SpiConnector>(
        {.mode = alloy::hal::SpiMode::Mode0,
         .frequency = 1'000'000u,
         .bit_order = alloy::hal::SpiBitOrder::MsbFirst,
         .data_size = alloy::hal::SpiDataSize::Bits8});
    auto fast_device = bus.device(
        {.mode = alloy::hal::SpiMode::Mode0,
         .frequency = 8'000'000u,
         .bit_order = alloy::hal::SpiBitOrder::MsbFirst,
         .data_size = alloy::hal::SpiDataSize::Bits8});
    auto slow_device = bus.device(
        {.mode = alloy::hal::SpiMode::Mode3,
         .frequency = 500'000u,
         .bit_order = alloy::hal::SpiBitOrder::MsbFirst,
         .data_size = alloy::hal::SpiDataSize::Bits8});

    std::array<std::uint8_t, 4> tx{0x9Fu, 0x00u, 0x00u, 0x00u};
    std::array<std::uint8_t, 4> rx{};

    [[maybe_unused]] const auto bus_result = bus.configure();
    [[maybe_unused]] const auto fast_result = fast_device.transfer(tx, rx);
    [[maybe_unused]] const auto slow_result = slow_device.transmit(tx);
}
#endif
