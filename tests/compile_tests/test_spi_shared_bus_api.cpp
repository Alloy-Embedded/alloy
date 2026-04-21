#include <array>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/spi.hpp"

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using SpiConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"SPI1">, alloy::device::runtime::PeripheralId::SPI1,
    alloy::hal::connection::runtime_binding<alloy::hal::sck<alloy::hal::pin<"PA5">>,
                                            alloy::device::runtime::PinId::PA5,
                                            alloy::device::runtime::SignalId::signal_sck>,
    alloy::hal::connection::runtime_binding<alloy::hal::miso<alloy::hal::pin<"PA6">>,
                                            alloy::device::runtime::PinId::PA6,
                                            alloy::device::runtime::SignalId::signal_miso>,
    alloy::hal::connection::runtime_binding<alloy::hal::mosi<alloy::hal::pin<"PA7">>,
                                            alloy::device::runtime::PinId::PA7,
                                            alloy::device::runtime::SignalId::signal_mosi>>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using SpiConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"SPI1">, alloy::device::runtime::PeripheralId::SPI1,
    alloy::hal::connection::runtime_binding<alloy::hal::sck<alloy::hal::pin<"PA5">>,
                                            alloy::device::runtime::PinId::PA5,
                                            alloy::device::runtime::SignalId::signal_sck>,
    alloy::hal::connection::runtime_binding<alloy::hal::miso<alloy::hal::pin<"PA6">>,
                                            alloy::device::runtime::PinId::PA6,
                                            alloy::device::runtime::SignalId::signal_miso>,
    alloy::hal::connection::runtime_binding<alloy::hal::mosi<alloy::hal::pin<"PA7">>,
                                            alloy::device::runtime::PinId::PA7,
                                            alloy::device::runtime::SignalId::signal_mosi>>;
#elif defined(ALLOY_BOARD_SAME70_XPLD)
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
