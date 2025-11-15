/**
 * @file main.cpp
 * @brief Basic Delays Example - Demonstrates millisecond and microsecond delays
 *
 * This example showcases:
 * - Accurate millisecond delays using delay_ms()
 * - Accurate microsecond delays using delay_us()
 * - Timing accuracy validation with millis() and micros()
 * - Portable code that works on all boards
 *
 * Expected behavior:
 * - LED toggles every 500ms (should be visible as blinking)
 * - Console prints actual delay duration vs expected
 * - Timing accuracy should be within ±1% on all boards
 *
 * Hardware: Any board with LED (F401RE, F722ZE, G071RB, G0B1RE, SAME70)
 */

#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include <cstdio>

using namespace alloy::hal;
using namespace alloy::core;

/**
 * @brief Measure and display actual delay duration
 *
 * Demonstrates using micros() to validate delay accuracy.
 */
void demonstrate_ms_delay(u32 delay_ms) {
    printf("\n--- Testing %lu ms delay ---\n", delay_ms);

    // Capture start time
    u64 start_us = SysTickTimer::micros<board::BoardSysTick>();
    u32 start_ms = SysTickTimer::millis<board::BoardSysTick>();

    // Perform delay
    SysTickTimer::delay_ms<board::BoardSysTick>(delay_ms);

    // Measure actual duration
    u64 end_us = SysTickTimer::micros<board::BoardSysTick>();
    u32 end_ms = SysTickTimer::millis<board::BoardSysTick>();

    u64 actual_us = end_us - start_us;
    u32 actual_ms = end_ms - start_ms;

    // Calculate accuracy
    float expected_us = delay_ms * 1000.0f;
    float error_percent = ((actual_us - expected_us) / expected_us) * 100.0f;

    printf("  Expected: %lu ms (%lu us)\n", delay_ms, delay_ms * 1000UL);
    printf("  Actual:   %lu ms (%llu us)\n", actual_ms, actual_us);
    printf("  Error:    %.2f%%\n", error_percent);

    if (error_percent < 1.0f) {
        printf("  ✓ Timing within ±1%% (PASS)\n");
    } else {
        printf("  ✗ Timing outside ±1%% (FAIL)\n");
    }
}

/**
 * @brief Demonstrate microsecond delays with GPIO toggle
 *
 * Shows high-resolution timing by toggling LED rapidly.
 */
void demonstrate_us_delay(u32 delay_us) {
    printf("\n--- Testing %lu us delay ---\n", delay_us);
    printf("  Toggling LED 10 times...\n");

    // Capture start time
    u64 start = SysTickTimer::micros<board::BoardSysTick>();

    // Toggle LED 10 times with microsecond delays
    for (int i = 0; i < 10; i++) {
        board::led::toggle();
        SysTickTimer::delay_us<board::BoardSysTick>(delay_us);
    }

    // Measure actual duration
    u64 end = SysTickTimer::micros<board::BoardSysTick>();
    u64 actual = end - start;
    u64 expected = delay_us * 10;

    float error_percent = ((actual - expected) / static_cast<float>(expected)) * 100.0f;

    printf("  Expected: %llu us (10 toggles × %lu us)\n", expected, delay_us);
    printf("  Actual:   %llu us\n", actual);
    printf("  Error:    %.2f%%\n", error_percent);

    if (error_percent < 2.0f) {
        printf("  ✓ Timing within ±2%% (PASS)\n");
    } else {
        printf("  ✗ Timing outside ±2%% (FAIL)\n");
    }

    // Return LED to known state
    board::led::off();
}

/**
 * @brief Demonstrate continuous blinking with timing validation
 */
void demonstrate_continuous_blink() {
    printf("\n--- Continuous Blink Test (500ms period) ---\n");
    printf("LED should toggle every 500ms. Press reset to restart.\n\n");

    u32 toggle_count = 0;
    u64 start_time = SysTickTimer::micros<board::BoardSysTick>();

    while (true) {
        // Toggle LED
        board::led::toggle();
        toggle_count++;

        // Delay 500ms
        SysTickTimer::delay_ms<board::BoardSysTick>(500);

        // Every 10 toggles (5 seconds), report timing accuracy
        if (toggle_count % 10 == 0) {
            u64 current_time = SysTickTimer::micros<board::BoardSysTick>();
            u64 elapsed_us = current_time - start_time;
            u64 expected_us = toggle_count * 500 * 1000;

            float error_percent =
                ((elapsed_us - expected_us) / static_cast<float>(expected_us)) * 100.0f;

            printf("Toggles: %lu, Elapsed: %llu us, Error: %.3f%%\n",
                   toggle_count, elapsed_us, error_percent);

            // Reset baseline every 100 toggles to prevent accumulated error
            if (toggle_count % 100 == 0) {
                start_time = current_time;
                toggle_count = 0;
            }
        }
    }
}

int main() {
    // Initialize board (includes SysTick at 1ms)
    board::init();

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  Basic Delays Example - SysTick Timing Validation   ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Board: %s\n", BOARD_NAME);
    printf("System Clock: %lu MHz\n", board::ClockConfig::system_clock_hz / 1000000);
    printf("SysTick Resolution: 1 ms\n");
    printf("\n");

    // Test various millisecond delays
    demonstrate_ms_delay(1);      // 1ms
    demonstrate_ms_delay(10);     // 10ms
    demonstrate_ms_delay(100);    // 100ms
    demonstrate_ms_delay(500);    // 500ms
    demonstrate_ms_delay(1000);   // 1000ms

    // Test microsecond delays
    demonstrate_us_delay(100);    // 100us
    demonstrate_us_delay(500);    // 500us
    demonstrate_us_delay(1000);   // 1000us (1ms)
    demonstrate_us_delay(5000);   // 5000us (5ms)

    // Run continuous blink with timing validation
    demonstrate_continuous_blink();

    return 0;
}
