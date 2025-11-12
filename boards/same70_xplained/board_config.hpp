#pragma once

/**
 * @file board_config.hpp
 * @brief SAME70 Xplained Ultra Board Configuration
 * 
 * Modern C++23 board abstraction with compile-time configuration
 * 
 * Features:
 * - Compile-time pin/peripheral configuration
 * - Zero runtime overhead
 * - Type-safe board resources
 * - Flexible initialization hooks
 */

#include <cstdint>

namespace board::same70_xplained {

// =============================================================================
// Clock Configuration
// =============================================================================

struct ClockConfig {
    static constexpr uint32_t cpu_freq_hz = 300'000'000;  // 300 MHz
    static constexpr uint32_t hclk_freq_hz = 300'000'000; // AHB clock
    static constexpr uint32_t pclk_freq_hz = 150'000'000; // Peripheral clock
    
    // Crystal oscillator
    static constexpr uint32_t xtal_freq_hz = 12'000'000;  // 12 MHz crystal
    
    // PLL configuration
    static constexpr uint32_t plla_mul = 25;    // 12 MHz * 25 = 300 MHz
    static constexpr uint32_t plla_div = 1;
};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    // LED0 (Green) - PC8
    static constexpr char led_green_port = 'C';
    static constexpr uint8_t led_green_pin = 8;
    static constexpr bool led_green_active_high = false;  // Active LOW
    
    // LED1 could be added here for future expansion
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    // SW0 (User button) - PA11
    static constexpr char button0_port = 'A';
    static constexpr uint8_t button0_pin = 11;
    static constexpr bool button0_active_high = false;  // Active LOW
};

// =============================================================================
// UART Configuration (Console/Debug)
// =============================================================================

struct UartConsoleConfig {
    // UART1 for console (EDBG virtual COM port)
    static constexpr uint8_t uart_instance = 1;  // UART1
    static constexpr uint32_t baudrate = 115'200;
    
    // Pins: PA9 (TX), PA10 (RX)
    static constexpr char tx_port = 'A';
    static constexpr uint8_t tx_pin = 9;
    static constexpr char rx_port = 'A';
    static constexpr uint8_t rx_pin = 10;
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
