#include <string_view>

#include "device/runtime.hpp"
#include "hal/gpio.hpp"
#include "hal/i2c.hpp"
#include "hal/spi.hpp"
#include "hal/uart.hpp"

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
using DebugUartRoute = alloy::hal::uart::route<
    alloy::dev::periph::USART2,
    alloy::hal::tx<alloy::dev::pin::PA2>,
    alloy::hal::rx<alloy::dev::pin::PA3>>;
using LegacyNamespaceRoute = alloy::hal::uart::route<
    alloy::dev::periph::USART2,
    alloy::hal::connection::tx<alloy::dev::pin::PA2>,
    alloy::hal::connection::rx<alloy::dev::pin::PA3>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartRoute>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
using LegacyNamespaceUart = decltype(alloy::hal::uart::open<LegacyNamespaceRoute>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(LegacyNamespaceUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART2"});

using LedHandle = decltype(alloy::hal::gpio::open<alloy::dev::pin::PA5>(
    {.direction = alloy::hal::PinDirection::Output}));
static_assert(LedHandle::valid);
static_assert(LedHandle::line_index == 5);

#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using DebugUartRoute = alloy::hal::uart::route<
    alloy::dev::periph::USART2,
    alloy::hal::tx<alloy::dev::pin::PA2>,
    alloy::hal::rx<alloy::dev::pin::PA3>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartRoute>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART2"});

using SpiRoute = alloy::hal::spi::route<
    alloy::dev::periph::SPI1,
    alloy::hal::sck<alloy::dev::pin::PA5>,
    alloy::hal::miso<alloy::dev::pin::PA6>,
    alloy::hal::mosi<alloy::dev::pin::PA7>>;
using SpiHandle = decltype(alloy::hal::spi::open<SpiRoute>({}));
static_assert(SpiHandle::valid);

#elif defined(ALLOY_BOARD_SAME70_XPLD)
using DebugUartRoute = alloy::hal::uart::route<
    alloy::dev::periph::USART1,
    alloy::hal::tx<alloy::dev::pin::PB4, alloy::dev::sig::signal_txd1>,
    alloy::hal::rx<alloy::dev::pin::PA21, alloy::dev::sig::signal_rxd1>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartRoute>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART1"});

using I2cRoute = alloy::hal::i2c::route<
    alloy::dev::periph::TWIHS0,
    alloy::hal::scl<alloy::dev::pin::PA4, alloy::dev::sig::signal_twck0>,
    alloy::hal::sda<alloy::dev::pin::PA3, alloy::dev::sig::signal_twd0>>;
using I2cHandle = decltype(alloy::hal::i2c::open<I2cRoute>({}));
static_assert(I2cHandle::valid);

using LedHandle = decltype(alloy::hal::gpio::open<alloy::dev::pin::PA23>(
    {.direction = alloy::hal::PinDirection::Output}));
static_assert(LedHandle::valid);
static_assert(LedHandle::line_index == 23);
#endif
