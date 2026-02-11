#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-G0B1RE
 *
 * Auto-generated from board.yaml
 * DO NOT EDIT MANUALLY - Run `ucore generate-board` to regenerate
 *
 * Board: Nucleo-G0B1RE
 * Vendor: STMicroelectronics
 * MCU: STM32G0B1RET6
 * Platform: stm32g0
 */

#include <cstdint>
#include "hal/vendors/st/stm32g0/gpio.hpp"
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace nucleo_g0b1re {

using namespace ucore::hal::st::stm32g0;
using namespace ucore::generated::stm32g0b1;

// =============================================================================
// Board Information
// =============================================================================

struct BoardInfo {
    static constexpr const char* name = "Nucleo-G0B1RE";
    static constexpr const char* vendor = "STMicroelectronics";
    static constexpr const char* version = "1.0.0";
    static constexpr const char* mcu = "STM32G0B1RET6";
    static constexpr const char* architecture = "cortex-m0+";
    static constexpr const char* description = "STM32 Nucleo-64 development board with STM32G0B1RE MCU";
    static constexpr const char* url = "https://www.st.com/en/evaluation-tools/nucleo-g0b1re.html";
};

// =============================================================================
// Clock Configuration
// =============================================================================

/**
 * @brief Clock configuration for Nucleo-G0B1RE
 *
 * Clock Source: HSI
 * System Clock: 64000000 Hz (64 MHz)
 *
 * Bus Clocks:
 *   AHB:  64000000 Hz / 1 = 64000000 Hz
 *   APB1: 64000000 Hz / 1 = 64000000 Hz
 *   APB2: 64000000 Hz / 1 = 64000000 Hz
 */
struct ClockConfig {

    /// Target system clock frequency
    static constexpr uint32_t system_clock_hz = 64000000;


    /// Flash latency (wait states) for 64 MHz
    static constexpr uint32_t flash_latency = 2;

    /// AHB prescaler (SYSCLK / ahb_prescaler)
    static constexpr uint32_t ahb_prescaler = 1;

    /// APB1 prescaler (AHB / apb1_prescaler)
    static constexpr uint32_t apb1_prescaler = 1;

    /// APB2 prescaler (AHB / apb2_prescaler)
    static constexpr uint32_t apb2_prescaler = 1;
};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// User LED LD4
    using led_green = GpioPin<peripherals::GPIOA, 5>;

    /// led_green is active HIGH
    static constexpr bool led_green_active_high = true;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// User button B1 (active LOW)
    using button_user = GpioPin<peripherals::GPIOC, 13>;

    /// button_user is active LOW
    static constexpr bool button_user_active_high = false;

};

// =============================================================================
// UART Configuration
// =============================================================================

struct UartConfig {
    /// ST-Link Virtual COM Port
    /// Instance: USART2, Baud: 115200
    using console_tx = GpioPin<peripherals::GPIOA, 2>;
    using console_rx = GpioPin<peripherals::GPIOA, 3>;

    static constexpr uint32_t console_baud_rate = 115200;
};

// =============================================================================
// SPI Configuration
// =============================================================================


// =============================================================================
// I2C Configuration
// =============================================================================


} // namespace nucleo_g0b1re
