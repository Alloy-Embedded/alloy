#pragma once

/**
 * @file board_config.hpp
 * @brief SAME70 Xplained Ultra Board Configuration
 *
 * Modern C++23 board abstraction with compile-time configuration.
 * Uses generated peripheral addresses and type-safe GPIO pins.
 *
 * Features:
 * - Compile-time pin/peripheral configuration
 * - Zero runtime overhead
 * - Type-safe board resources using actual HAL types
 * - No magic numbers - all addresses from generated peripherals.hpp
 */

#include <cstdint>
#include "hal/vendors/arm/same70/gpio.hpp"
#include "hal/vendors/arm/same70/clock.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace board::same70_xplained {

using namespace alloy::hal::same70;
using namespace alloy::generated::atsame70q21b;

// =============================================================================
// Clock Configuration
// =============================================================================

struct ClockConfig {
    // IMPORTANT: PLL is not working (see docs/KNOWN_ISSUES.md)
    // Using 12 MHz RC oscillator without PLL as workaround
    static constexpr uint32_t cpu_freq_hz = 12'000'000;   // 12 MHz RC
    static constexpr uint32_t hclk_freq_hz = 12'000'000;  // AHB clock
    static constexpr uint32_t pclk_freq_hz = 12'000'000;  // Peripheral clock

    // Use the workaround clock config from platform layer
    static constexpr const alloy::hal::same70::ClockConfig& clock_init_config = CLOCK_CONFIG_12MHZ_RC;
};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    // LED0 (Green) - PC8, active LOW
    // Type-safe GPIO pin using platform layer templates
    using led_green = GpioPin<peripherals::PIOC, 8>;

    static constexpr bool led_green_active_high = false;  // Active LOW

    // LED1 could be added here for future expansion
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    // SW0 (User button) - PA11, active LOW
    // Type-safe GPIO pin using platform layer templates
    using button0 = GpioPin<peripherals::PIOA, 11>;

    static constexpr bool button0_active_high = false;  // Active LOW
};

// =============================================================================
// UART Configuration (Console/Debug)
// =============================================================================

struct UartConsoleConfig {
    // UART1 for console (EDBG virtual COM port)
    // Pins: PA9 (TX), PA10 (RX)
    static constexpr uint8_t uart_instance = 1;  // UART1
    static constexpr uint32_t baudrate = 115'200;

    // Type-safe GPIO pins for UART
    using tx_pin = GpioPin<peripherals::PIOA, 9>;
    using rx_pin = GpioPin<peripherals::PIOA, 10>;
};

// =============================================================================
// SysTick Configuration
// =============================================================================

struct SysTickConfig {
    static constexpr uint32_t tick_freq_hz = 1'000;  // 1 kHz (1ms ticks)
};

// =============================================================================
// Board Info
// =============================================================================

struct BoardInfo {
    static constexpr const char* name = "SAME70 Xplained Ultra";
    static constexpr const char* mcu = "ATSAME70Q21B";
    static constexpr const char* vendor = "Microchip/Atmel";
    static constexpr const char* architecture = "ARM Cortex-M7";
};

} // namespace board::same70_xplained
