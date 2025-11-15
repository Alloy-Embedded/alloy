/**
 * @file main.cpp
 * @brief Basic Timing Delays Example
 *
 * Demonstrates accurate millisecond and microsecond delays using SysTick timer.
 * Shows blocking delay patterns and timing accuracy validation.
 *
 * ## Hardware Setup
 * - Connect LED to board (uses built-in LED)
 * - Optional: Logic analyzer on LED pin to measure timing accuracy
 *
 * ## Expected Behavior
 * LED blinks with precise timing patterns:
 * 1. 1-second ON/OFF cycle (1000ms delays)
 * 2. 500ms ON/OFF cycle (demonstrates different delay values)
 * 3. 100ms rapid blink pattern
 * 4. Mixed ms/us delays showing microsecond precision
 *
 * ## Timing Accuracy
 * Expected accuracy: ±1% for millisecond delays, ±5% for microsecond delays
 * Actual timing can be measured with oscilloscope or logic analyzer.
 *
 * ## Learning Objectives
 * - Using SysTickTimer::delay_ms() for blocking delays
 * - Understanding delay accuracy and limitations
 * - Board-portable timing code using BoardSysTick type
 * - Compile-time clock frequency configuration
 */

#include "board/board.hpp"
#include "hal/api/systick_simple.hpp"

using namespace alloy::hal;

/**
 * @brief Demonstrate basic millisecond delays
 *
 * Shows different delay durations and their use cases.
 */
void demo_millisecond_delays() {
    // Long delay - 1 second (good for human-visible timing)
    board::led::on();
    SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    board::led::off();
    SysTickTimer::delay_ms<board::BoardSysTick>(1000);

    // Medium delay - 500ms (typical LED blink rate)
    board::led::on();
    SysTickTimer::delay_ms<board::BoardSysTick>(500);
    board::led::off();
    SysTickTimer::delay_ms<board::BoardSysTick>(500);

    // Short delay - 100ms (rapid blink)
    for (int i = 0; i < 5; i++) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(100);
    }
}

/**
 * @brief Demonstrate microsecond delays
 *
 * Shows high-precision timing for time-critical operations.
 * Note: Accuracy depends on CPU speed and interrupt latency.
 */
void demo_microsecond_delays() {
    // 1ms delay using microseconds (1000us = 1ms)
    board::led::on();
    SysTickTimer::delay_us<board::BoardSysTick>(1000);
    board::led::off();
    SysTickTimer::delay_us<board::BoardSysTick>(1000);

    // 500us delay (good for protocol timing)
    for (int i = 0; i < 10; i++) {
        board::led::toggle();
        SysTickTimer::delay_us<board::BoardSysTick>(500);
    }

    // 100us delay (high-frequency operations)
    // Note: At this precision, LED won't be visible, but timing is measurable
    for (int i = 0; i < 100; i++) {
        board::led::toggle();
        SysTickTimer::delay_us<board::BoardSysTick>(100);
    }
}

/**
 * @brief Demonstrate timing validation using millis()
 *
 * Shows how to measure actual delay duration and verify accuracy.
 */
void demo_timing_validation() {
    // Measure actual delay duration
    u32 start = SysTickTimer::millis<board::BoardSysTick>();

    board::led::on();
    SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    board::led::off();

    u32 elapsed = SysTickTimer::millis<board::BoardSysTick>() - start;

    // In a real application, you would log or assert elapsed ~= 1000ms
    // Expected: 1000ms ±10ms (±1% accuracy)
    (void)elapsed;  // Suppress unused warning in this example
}

/**
 * @brief Demonstrate mixed delay patterns
 *
 * Shows combining different delay types for complex timing.
 */
void demo_mixed_delays() {
    // Pattern: Quick pulse followed by long pause
    board::led::on();
    SysTickTimer::delay_us<board::BoardSysTick>(50);  // 50us pulse
    board::led::off();
    SysTickTimer::delay_ms<board::BoardSysTick>(500);  // 500ms pause

    // Pattern: Burst of rapid pulses
    for (int i = 0; i < 5; i++) {
        board::led::on();
        SysTickTimer::delay_us<board::BoardSysTick>(100);  // 100us ON
        board::led::off();
        SysTickTimer::delay_us<board::BoardSysTick>(100);  // 100us OFF
    }

    SysTickTimer::delay_ms<board::BoardSysTick>(1000);  // 1s pause between patterns
}

/**
 * @brief Main application entry point
 */
int main() {
    // Initialize board hardware (including SysTick at 1ms tick rate)
    board::init();

    // Infinite loop demonstrating different delay patterns
    while (true) {
        // Demo 1: Basic millisecond delays (visible LED patterns)
        demo_millisecond_delays();

        SysTickTimer::delay_ms<board::BoardSysTick>(2000);  // Pause between demos

        // Demo 2: Microsecond delays (for precision timing)
        demo_microsecond_delays();

        SysTickTimer::delay_ms<board::BoardSysTick>(2000);  // Pause between demos

        // Demo 3: Timing validation (measure accuracy)
        demo_timing_validation();

        SysTickTimer::delay_ms<board::BoardSysTick>(2000);  // Pause between demos

        // Demo 4: Mixed delay patterns (complex timing)
        demo_mixed_delays();

        SysTickTimer::delay_ms<board::BoardSysTick>(5000);  // Long pause before restart
    }

    return 0;
}
