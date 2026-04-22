#include <array>
#include <cstdint>
#include <string_view>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/spi.hpp"

#include "device/traits.hpp"

namespace {

constexpr auto spi_peripheral_clock_hz() -> std::uint32_t {
#if defined(ALLOY_BOARD_NUCLEO_G071RB)
    return 64'000'000u;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    return 84'000'000u;
#elif defined(ALLOY_BOARD_SAME70_XPLD)
    return 12'000'000u;
#else
    return 0u;
#endif
}

template <typename SpiHandle>
consteval auto spi_is_usable() -> bool {
    if constexpr (!SpiHandle::valid) {
        return false;
    }

    return !SpiHandle::peripheral_name.empty() && SpiHandle::base_address() != 0u &&
           SpiHandle::operations().size() >= 3;
}

template <typename SpiHandle>
void exercise_spi_backend() {
    auto port = alloy::hal::spi::open<typename SpiHandle::connector_type>(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode0, 1'000'000u, alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8, spi_peripheral_clock_hz()});

    std::array<std::uint8_t, 4> tx_buffer{0x9Fu, 0x00u, 0x00u, 0x00u};
    std::array<std::uint8_t, 4> rx_buffer{};

    [[maybe_unused]] const auto configure_result = port.configure();
    [[maybe_unused]] const auto transfer_result = port.transfer(tx_buffer, rx_buffer);
    [[maybe_unused]] const auto transmit_result = port.transmit(tx_buffer);
    [[maybe_unused]] const auto receive_result = port.receive(rx_buffer);
    [[maybe_unused]] const auto busy = port.is_busy();
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5, alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;
using SpiPort = decltype(alloy::hal::spi::open<SpiConnector>());
static_assert(SpiPort::valid);
static_assert(SpiPort::peripheral_name == std::string_view{"SPI1"});
static_assert(spi_is_usable<SpiPort>());
[[maybe_unused]] void compile_g071_spi_backend() {
    exercise_spi_backend<SpiPort>();
}
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5, alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;
using SpiPort = decltype(alloy::hal::spi::open<SpiConnector>());
static_assert(SpiPort::valid);
static_assert(SpiPort::peripheral_name == std::string_view{"SPI1"});
static_assert(spi_is_usable<SpiPort>());
[[maybe_unused]] void compile_f401_spi_backend() {
    exercise_spi_backend<SpiPort>();
}
#elif defined(ALLOY_BOARD_SAME70_XPLD)
using SpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI0,
    alloy::hal::connection::sck<alloy::device::PinId::PD22, alloy::device::SignalId::signal_spck>,
    alloy::hal::connection::miso<alloy::device::PinId::PD20,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PD21,
                                 alloy::device::SignalId::signal_mosi>>;
using SpiPort = decltype(alloy::hal::spi::open<SpiConnector>());
static_assert(SpiPort::valid);
static_assert(SpiPort::peripheral_name == std::string_view{"SPI0"});
static_assert(spi_is_usable<SpiPort>());
[[maybe_unused]] void compile_same70_spi_backend() {
    exercise_spi_backend<SpiPort>();
}
#endif
