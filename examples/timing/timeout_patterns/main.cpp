/**
 * @file main.cpp
 * @brief Timeout Patterns Example
 *
 * Demonstrates non-blocking timeout handling patterns using SysTick timer.
 * Shows how to implement responsive, timeout-based control flow.
 *
 * ## Hardware Setup
 * - Connect LED to board (uses built-in LED)
 * - Optional: Button for user input (uses built-in button if available)
 *
 * ## Expected Behavior
 * LED demonstrates different timeout patterns:
 * 1. Blocking timeout with retry logic
 * 2. Non-blocking timeout for responsive code
 * 3. Multiple concurrent timeouts
 * 4. Timeout recovery and fallback patterns
 *
 * ## Learning Objectives
 * - Using is_timeout_ms() for non-blocking waits
 * - Implementing retry logic with timeouts
 * - Handling timeout expiration gracefully
 * - Combining multiple timeouts in one application
 * - Writing responsive embedded code
 */

#include "board/board.hpp"
#include "hal/api/systick_simple.hpp"

using namespace alloy::hal;

/**
 * @brief Simulate an operation that might fail
 *
 * In a real application, this could be:
 * - Waiting for sensor data ready
 * - Polling for I2C ACK
 * - Waiting for UART RX
 * - Checking for button press
 *
 * @param attempt Current attempt number (for demo purposes)
 * @return true if operation succeeds, false if still waiting
 */
bool simulate_operation(int attempt) {
    // Succeed on 3rd attempt (for demonstration)
    return attempt >= 3;
}

/**
 * @brief Pattern 1: Blocking timeout with retry logic
 *
 * Demonstrates:
 * - Waiting for an operation with a timeout
 * - Retrying on timeout
 * - Giving up after max retries
 *
 * Use case: Polling for peripheral ready state
 */
void demo_blocking_timeout_with_retry() {
    const u32 TIMEOUT_MS = 500;      // Wait up to 500ms per attempt
    const int MAX_RETRIES = 3;       // Try up to 3 times

    board::led::off();

    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        // Start timing
        u32 start = SysTickTimer::millis<board::BoardSysTick>();

        // Flash LED during attempt
        board::led::on();

        // Wait for operation to complete or timeout
        while (!simulate_operation(retry)) {
            // Check if timeout occurred
            if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(start, TIMEOUT_MS)) {
                // Timeout! Try next attempt
                board::led::off();
                SysTickTimer::delay_ms<board::BoardSysTick>(200);  // Brief pause before retry
                break;
            }

            // Still waiting, do other work or just yield
            SysTickTimer::delay_ms<board::BoardSysTick>(50);
        }

        // If operation succeeded, exit retry loop
        if (simulate_operation(retry)) {
            // Success! LED stays on briefly
            SysTickTimer::delay_ms<board::BoardSysTick>(1000);
            board::led::off();
            return;
        }
    }

    // All retries failed - indicate error with rapid blink
    for (int i = 0; i < 10; i++) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(100);
    }
}

/**
 * @brief Pattern 2: Non-blocking timeout for responsive code
 *
 * Demonstrates:
 * - State machine with timeout
 * - Remaining responsive during wait
 * - LED animation while waiting
 *
 * Use case: Waiting for network response while updating display
 */
void demo_nonblocking_timeout() {
    const u32 TIMEOUT_MS = 3000;  // 3 second timeout

    enum class State {
        WAITING,
        SUCCESS,
        TIMEOUT
    };

    State state = State::WAITING;
    u32 start = SysTickTimer::millis<board::BoardSysTick>();
    u32 last_blink = start;
    bool led_state = false;
    int operation_counter = 0;

    // Non-blocking wait loop
    while (state == State::WAITING) {
        // Check if timeout occurred
        if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(start, TIMEOUT_MS)) {
            state = State::TIMEOUT;
            break;
        }

        // Do other work while waiting (e.g., animate LED)
        u32 now = SysTickTimer::millis<board::BoardSysTick>();
        if (now - last_blink >= 250) {  // Blink every 250ms
            led_state = !led_state;
            if (led_state) {
                board::led::on();
            } else {
                board::led::off();
            }
            last_blink = now;
        }

        // Simulate checking operation status
        operation_counter++;
        if (simulate_operation(operation_counter / 20)) {  // Succeed after ~1 second
            state = State::SUCCESS;
            break;
        }

        // Yield briefly (in RTOS, this would be a task yield)
        SysTickTimer::delay_ms<board::BoardSysTick>(10);
    }

    // Handle result
    board::led::off();
    if (state == State::SUCCESS) {
        // Success: 3 quick flashes
        for (int i = 0; i < 3; i++) {
            board::led::on();
            SysTickTimer::delay_ms<board::BoardSysTick>(100);
            board::led::off();
            SysTickTimer::delay_ms<board::BoardSysTick>(100);
        }
    } else {
        // Timeout: long flash
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(2000);
        board::led::off();
    }
}

/**
 * @brief Pattern 3: Multiple concurrent timeouts
 *
 * Demonstrates:
 * - Managing multiple independent timeouts
 * - Different timeout durations
 * - Coordinating multiple time-based events
 *
 * Use case: Sensor polling with watchdog timer
 */
void demo_multiple_timeouts() {
    const u32 FAST_BLINK_PERIOD = 200;   // 200ms LED blink
    const u32 SLOW_TIMEOUT = 5000;       // 5 second overall timeout

    u32 overall_start = SysTickTimer::millis<board::BoardSysTick>();
    u32 blink_start = overall_start;
    bool led_state = false;

    // Run fast blink for up to 5 seconds
    while (!SysTickTimer::is_timeout_ms<board::BoardSysTick>(overall_start, SLOW_TIMEOUT)) {
        // Check blink timeout
        if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(blink_start, FAST_BLINK_PERIOD)) {
            // Toggle LED
            led_state = !led_state;
            if (led_state) {
                board::led::on();
            } else {
                board::led::off();
            }

            // Reset blink timeout
            blink_start = SysTickTimer::millis<board::BoardSysTick>();
        }

        // Do other work...
        SysTickTimer::delay_ms<board::BoardSysTick>(10);
    }

    board::led::off();
}

/**
 * @brief Pattern 4: Timeout with fallback/degraded mode
 *
 * Demonstrates:
 * - Primary operation with timeout
 * - Fallback to alternative method on timeout
 * - Graceful degradation
 *
 * Use case: GPS with fallback to cell tower positioning
 */
void demo_timeout_with_fallback() {
    const u32 PRIMARY_TIMEOUT_MS = 1000;    // Try primary method for 1s
    const u32 FALLBACK_TIMEOUT_MS = 2000;   // Try fallback for 2s

    // Try primary method
    board::led::on();
    u32 start = SysTickTimer::millis<board::BoardSysTick>();

    // Simulate primary method (fast but might fail)
    bool primary_success = false;
    while (!SysTickTimer::is_timeout_ms<board::BoardSysTick>(start, PRIMARY_TIMEOUT_MS)) {
        // In real code, try to get GPS fix
        if (simulate_operation(5)) {  // Never succeeds in this demo
            primary_success = true;
            break;
        }
        SysTickTimer::delay_ms<board::BoardSysTick>(100);
    }

    board::led::off();

    if (primary_success) {
        // Primary method succeeded - single long flash
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(1000);
        board::led::off();
    } else {
        // Primary timeout - try fallback
        SysTickTimer::delay_ms<board::BoardSysTick>(500);

        // Indicate fallback mode with double blink
        for (int i = 0; i < 2; i++) {
            board::led::on();
            SysTickTimer::delay_ms<board::BoardSysTick>(200);
            board::led::off();
            SysTickTimer::delay_ms<board::BoardSysTick>(200);
        }

        // Try fallback method
        start = SysTickTimer::millis<board::BoardSysTick>();
        bool fallback_success = false;

        while (!SysTickTimer::is_timeout_ms<board::BoardSysTick>(start, FALLBACK_TIMEOUT_MS)) {
            // In real code, use cell tower positioning
            if (simulate_operation(1)) {  // Succeeds quickly
                fallback_success = true;
                break;
            }
            SysTickTimer::delay_ms<board::BoardSysTick>(100);
        }

        if (fallback_success) {
            // Fallback succeeded - triple flash
            for (int i = 0; i < 3; i++) {
                board::led::on();
                SysTickTimer::delay_ms<board::BoardSysTick>(200);
                board::led::off();
                SysTickTimer::delay_ms<board::BoardSysTick>(200);
            }
        } else {
            // Both methods failed - rapid error blink
            for (int i = 0; i < 10; i++) {
                board::led::toggle();
                SysTickTimer::delay_ms<board::BoardSysTick>(100);
            }
        }
    }

    board::led::off();
}

/**
 * @brief Main application entry point
 */
int main() {
    // Initialize board hardware (including SysTick at 1ms tick rate)
    board::init();

    // Infinite loop demonstrating different timeout patterns
    while (true) {
        // Pattern 1: Blocking timeout with retry
        demo_blocking_timeout_with_retry();
        SysTickTimer::delay_ms<board::BoardSysTick>(2000);

        // Pattern 2: Non-blocking timeout
        demo_nonblocking_timeout();
        SysTickTimer::delay_ms<board::BoardSysTick>(2000);

        // Pattern 3: Multiple concurrent timeouts
        demo_multiple_timeouts();
        SysTickTimer::delay_ms<board::BoardSysTick>(2000);

        // Pattern 4: Timeout with fallback
        demo_timeout_with_fallback();
        SysTickTimer::delay_ms<board::BoardSysTick>(5000);
    }

    return 0;
}
