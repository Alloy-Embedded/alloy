#pragma once

/**
 * @file board.hpp
 * @brief Board abstraction for Nucleo-F401RE
 *
 * Provides a unified API for board-level functionality:
 * - Board initialization
 * - LED control
 * - Button access
 * - SysTick timer
 */

#include "hal/gpio.hpp"
#include "hal/i2c.hpp"
#include "hal/spi.hpp"
#include "hal/systick.hpp"
#include "hal/uart.hpp"
#include "hal/vendors/st/stm32f4/systick_platform.hpp"

#include "board_config.hpp"

namespace board {

using namespace nucleo_f401re;
using namespace alloy::hal;
using namespace alloy::hal::st::stm32f4;

// Board-specific SysTick type (84 MHz)
using BoardSysTick = SysTick<ClockConfig::system_clock_hz>;

// RTOS Tick Source (must be 1ms tick for RTOS compatibility)
// Note: The 1ms tick period is configured at runtime via SysTickTimer::init_ms<BoardSysTick>(1)
using RTOSTick = BoardSysTick;

using DebugUartConnector = nucleo_f401re::UartConfig::debug_connector;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>());
using BoardI2cConnector = nucleo_f401re::I2cConfig::bus_connector;
using BoardI2c = decltype(alloy::hal::i2c::open<BoardI2cConnector>());
using BoardSpiConnector = nucleo_f401re::SpiConfig::bus_connector;
using BoardSpi = decltype(alloy::hal::spi::open<BoardSpiConnector>());

[[nodiscard]] inline auto make_debug_uart(alloy::hal::uart::Config config = {}) -> DebugUart {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = nucleo_f401re::UartConfig::peripheral_clock_hz;
    }
    return alloy::hal::uart::open<DebugUartConnector>(config);
}

[[nodiscard]] inline auto make_i2c(alloy::hal::i2c::Config config = {}) -> BoardI2c {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = nucleo_f401re::I2cConfig::peripheral_clock_hz;
    }
    return alloy::hal::i2c::open<BoardI2cConnector>(config);
}

[[nodiscard]] inline auto make_spi(alloy::hal::spi::Config config = {}) -> BoardSpi {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = nucleo_f401re::SpiConfig::peripheral_clock_hz;
    }
    return alloy::hal::spi::open<BoardSpiConnector>(config);
}

/**
 * @brief Initialize all board hardware
 *
 * This function:
 * 1. Configures system clock to 84 MHz
 * 2. Initializes SysTick timer (1ms tick)
 * 3. Initializes board resources through descriptor-driven drivers
 * 4. Enables interrupts
 */
void init();

/**
 * @brief LED control namespace
 */
namespace led {
/**
 * @brief Initialize LED GPIO
 */
void init();

/**
 * @brief Turn LED on
 */
void on();

/**
 * @brief Turn LED off
 */
void off();

/**
 * @brief Toggle LED state
 */
void toggle();
}  // namespace led

}  // namespace board
