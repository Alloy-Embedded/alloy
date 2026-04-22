#include <array>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/i2c.hpp"

#if defined(ALLOY_BOARD_SAME70_XPLD)
using I2cConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::TWIHS0,
    alloy::hal::connection::scl<alloy::device::PinId::PA4, alloy::device::SignalId::signal_twck0>,
    alloy::hal::connection::sda<alloy::device::PinId::PA3, alloy::device::SignalId::signal_twd0>>;

void compile_i2c_shared_bus() {
    auto bus = alloy::hal::i2c::open_shared_bus<I2cConnector>(
        {.speed = alloy::hal::I2cSpeed::Fast,
         .addressing = alloy::hal::I2cAddressing::SevenBit,
         .peripheral_clock_hz = 12'000'000u});
    auto eeprom = bus.device(
        {.speed = alloy::hal::I2cSpeed::Fast,
         .addressing = alloy::hal::I2cAddressing::SevenBit,
         .peripheral_clock_hz = 12'000'000u});
    auto sensor = bus.device(
        {.speed = alloy::hal::I2cSpeed::Standard,
         .addressing = alloy::hal::I2cAddressing::SevenBit,
         .peripheral_clock_hz = 12'000'000u});

    std::array<std::uint8_t, 2> write_buffer{0x00u, 0x55u};
    std::array<std::uint8_t, 4> read_buffer{};

    [[maybe_unused]] const auto bus_result = bus.configure();
    [[maybe_unused]] const auto eeprom_result = eeprom.write(0x50u, write_buffer);
    [[maybe_unused]] const auto sensor_result = sensor.write_read(0x68u, write_buffer, read_buffer);
}
#endif
