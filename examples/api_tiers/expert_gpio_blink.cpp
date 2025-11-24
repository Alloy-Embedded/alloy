/**
 * @file expert_gpio_blink.cpp
 * @brief GPIO Expert API Example - High-Performance LED Blink
 *
 * Demonstrates the Expert tier API from hal/api/gpio_expert.hpp
 * Provides full control over all configuration parameters including
 * platform-specific advanced features.
 *
 * Hardware:
 * - LED on PA5 (Nucleo F401/G071/G0B1) or PB7 (Nucleo F722)
 *
 * Expected behavior:
 * - LED blinks with expert configuration (high-speed output)
 * - Compile-time validation of configuration
 *
 * @note Part of MicroCore API Tier Examples
 */

#include "board.hpp"
#include "hal/api/gpio_expert.hpp"

using namespace ucore::hal;
using namespace board::pins;

// Delay helper
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 10000; i++) {
        __asm volatile("nop");
    }
}

int main() {
    // Expert API: Full configuration control
    // Struct-based aggregate initialization
    GpioExpertConfig config = {
        .direction = PinDirection::Output,
        .pull = PinPull::None,
        .drive = PinDrive::PushPull,
        .active_high = true,
        .initial_state_on = false,

        // Advanced platform-specific features
        .drive_strength = 3,        // Maximum drive strength (0-3)
        .slew_rate_fast = true,     // Fast slew rate for high-speed switching
        .input_filter_enable = false,
        .filter_clock_div = 0
    };

    // Compile-time validation
    static_assert(config.is_valid(), config.error_message());

    // Create expert GPIO pin with full configuration
    auto led = GpioExpert<led_green>(config);

    // Alternative: Use factory method for common expert patterns
    // auto led = GpioExpert<led_green>(GpioExpertConfig::high_speed_output());

    while (true) {
        led.on().unwrap();
        delay_ms(500);

        led.off().unwrap();
        delay_ms(500);
    }

    return 0;
}
