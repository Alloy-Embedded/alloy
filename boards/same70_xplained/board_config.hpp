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
#include "hal/connect.hpp"

namespace board::same70_xplained {

// =============================================================================
// Clock Configuration
// =============================================================================

struct ClockConfig {
    // IMPORTANT: PLL is not working (see docs/KNOWN_ISSUES.md)
    // Using 12 MHz RC oscillator without PLL as workaround
    static constexpr uint32_t cpu_freq_hz = 12'000'000;   // 12 MHz RC
    static constexpr uint32_t hclk_freq_hz = 12'000'000;  // AHB clock
    static constexpr uint32_t pclk_freq_hz = 12'000'000;  // Peripheral clock

};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    using led_green = alloy::hal::pin<"PC8">;

    static constexpr bool led_green_active_high = false;  // Active LOW

    // LED1 could be added here for future expansion
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    using button0 = alloy::hal::pin<"PA11">;

    static constexpr bool button0_active_high = false;  // Active LOW
};

// =============================================================================
// UART Configuration (Console/Debug)
// =============================================================================

struct UartConsoleConfig {
    static constexpr uint8_t uart_instance = 0;  // USART0 on EDBG virtual COM
    static constexpr uint32_t baudrate = 115'200;

    using tx_pin = alloy::hal::pin<"PB1">;
    using rx_pin = alloy::hal::pin<"PB0">;
    using debug_connector =
        decltype(alloy::hal::connect<alloy::hal::peripheral<"USART0">,
                                     alloy::hal::tx<tx_pin>,
                                     alloy::hal::rx<rx_pin>>());
    static constexpr uint32_t peripheral_clock_hz = ClockConfig::pclk_freq_hz;
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
