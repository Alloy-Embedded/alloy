#include <array>
#include <cstdint>
#include <string_view>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/i2c.hpp"

#include "device/traits.hpp"

namespace {

constexpr auto i2c_peripheral_clock_hz() -> std::uint32_t {
#if defined(ALLOY_BOARD_NUCLEO_G071RB)
    return 64'000'000u;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    return 42'000'000u;
#elif defined(ALLOY_BOARD_SAME70_XPLD)
    return 12'000'000u;
#else
    return 0u;
#endif
}

template <typename I2cHandle>
consteval auto i2c_is_usable() -> bool {
    if constexpr (!I2cHandle::valid) {
        return false;
    }

    return !I2cHandle::peripheral_name.empty() && I2cHandle::base_address() != 0u &&
           I2cHandle::operations().size() >= 2;
}

template <typename I2cHandle>
void exercise_i2c_backend() {
    auto port = alloy::hal::i2c::open<typename I2cHandle::connector_type>(
        alloy::hal::i2c::Config{alloy::hal::I2cSpeed::Fast, alloy::hal::I2cAddressing::SevenBit,
                                i2c_peripheral_clock_hz()});

    std::array<std::uint8_t, 2> write_buffer{0x00u, 0x55u};
    std::array<std::uint8_t, 4> read_buffer{};
    std::array<std::uint8_t, 16> found_devices{};

    [[maybe_unused]] const auto configure_result = port.configure();
    [[maybe_unused]] const auto write_result = port.write(0x50u, write_buffer);
    [[maybe_unused]] const auto read_result = port.read(0x50u, read_buffer);
    [[maybe_unused]] const auto write_read_result =
        port.write_read(0x50u, write_buffer, read_buffer);
    [[maybe_unused]] const auto scan_result = port.scan_bus(found_devices);
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_SAME70_XPLD)
using I2cConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"TWIHS0">, alloy::device::runtime::PeripheralId::TWIHS0,
    alloy::hal::connection::runtime_binding<alloy::hal::scl<alloy::hal::pin<"PA4">>,
                                            alloy::device::runtime::PinId::PA4,
                                            alloy::device::runtime::SignalId::signal_twck0>,
    alloy::hal::connection::runtime_binding<alloy::hal::sda<alloy::hal::pin<"PA3">>,
                                            alloy::device::runtime::PinId::PA3,
                                            alloy::device::runtime::SignalId::signal_twd0>>;
using I2cPort = decltype(alloy::hal::i2c::open<I2cConnector>());
static_assert(I2cPort::valid);
static_assert(I2cPort::peripheral_name == std::string_view{"TWIHS0"});
static_assert(i2c_is_usable<I2cPort>());
[[maybe_unused]] void compile_same70_i2c_backend() {
    exercise_i2c_backend<I2cPort>();
}
#endif
