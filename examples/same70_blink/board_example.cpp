/**
 * @file board_example.cpp
 * @brief SAME70 LED Blink Example using board_v2.hpp
 *
 * This example demonstrates the new board abstraction layer that provides
 * application-friendly names and convenience functions on top of the
 * template-based HAL.
 *
 * Features:
 *   - Uses board:: namespace for peripheral access
 *   - Type-safe LED and button control
 *   - Clean, readable application code
 *   - Zero overhead - compiles to direct register access
 *
 * Hardware: ATSAME70 Xplained Ultra
 * LED: PC8 (green LED, active LOW)
 * Button: PA11 (SW0, active LOW with pull-up)
 */

#include "../../boards/atmel_same70_xpld/board_v2.hpp"

// Vector table (minimal for this example)
extern "C" {
extern uint32_t _estack;
void Reset_Handler() __attribute__((noreturn));
void Default_Handler() {
    while (1) {}
}
}

// Vector table in .isr_vector section
__attribute__((section(".isr_vector"))) const void* vector_table[] = {
    &_estack,
    (void*)Reset_Handler,
};

/**
 * @brief Main program entry point
 *
 * Demonstrates three usage patterns:
 * 1. Simple LED blinking
 * 2. Button-controlled LED
 * 3. Board namespace convenience functions
 */
void Reset_Handler() {
    // ========================================================================
    // Initialize board
    // ========================================================================

    auto result = board::initialize();
    if (result.is_error()) {
        while (1) {}  // Initialization failed
    }

    // Initialize LED
    result = board::led::init();
    if (result.is_error()) {
        while (1) {}  // LED init failed
    }

    // Initialize button
    result = board::button::init();
    if (result.is_error()) {
        while (1) {}  // Button init failed
    }

    // ========================================================================
    // Pattern 1: Simple blink using board namespace
    // ========================================================================

    // Blink 5 times to show initialization succeeded
    for (int i = 0; i < 5; i++) {
        board::led::on();
        board::delay_ms(100);
        board::led::off();
        board::delay_ms(100);
    }

    board::delay_ms(500);

    // ========================================================================
    // Pattern 2: Direct GPIO control using type aliases
    // ========================================================================

    // Access LED through type alias
    auto green_led = board::led_green{};

    // Blink 3 times with direct GPIO control
    for (int i = 0; i < 3; i++) {
        green_led.clear();  // Active LOW - turn ON
        board::delay_ms(200);
        green_led.set();  // Active LOW - turn OFF
        board::delay_ms(200);
    }

    board::delay_ms(500);

    // ========================================================================
    // Pattern 3: Button-controlled LED
    // ========================================================================

    // LED follows button state
    // Press button = LED on, release = LED off
    // Do this for ~5 seconds
    for (int i = 0; i < 50; i++) {
        auto pressed = board::button::is_pressed();

        if (pressed.is_ok() && pressed.value()) {
            board::led::on();
        } else {
            board::led::off();
        }

        board::delay_ms(100);
    }

    // ========================================================================
    // Main loop: Toggle LED every 500ms
    // ========================================================================

    while (1) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
