/**
 * @file main.cpp
 * @brief Timeout Patterns Example - Non-blocking timeout handling
 *
 * This example demonstrates:
 * - Blocking timeout with retry logic
 * - Non-blocking timeout for polling operations
 * - Multiple concurrent timeouts
 * - Real-world patterns for communication timeouts
 *
 * Scenarios:
 * 1. Simulated sensor polling with timeout and retry
 * 2. Non-blocking state machine with timeout
 * 3. Multiple concurrent operations with different timeouts
 *
 * Hardware: Any board with LED (F401RE, F722ZE, G071RB, G0B1RE, SAME70)
 */

#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include <cstdio>

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Simulated Sensor (responds after N attempts)
// ============================================================================

class SimulatedSensor {
private:
    u32 attempts_before_ready_;
    u32 current_attempts_;
    bool data_ready_;

public:
    SimulatedSensor(u32 attempts_before_ready)
        : attempts_before_ready_(attempts_before_ready),
          current_attempts_(0),
          data_ready_(false) {}

    void reset() {
        current_attempts_ = 0;
        data_ready_ = false;
    }

    bool poll() {
        current_attempts_++;
        if (current_attempts_ >= attempts_before_ready_) {
            data_ready_ = true;
        }
        return data_ready_;
    }

    bool is_ready() const { return data_ready_; }
    u32 get_attempts() const { return current_attempts_; }
};

// ============================================================================
// Pattern 1: Blocking Timeout with Retry
// ============================================================================

/**
 * @brief Wait for sensor with timeout and retry logic
 *
 * Pattern: Try up to max_retries times, each with timeout_ms.
 * This is common for unreliable communication (I2C, SPI, UART).
 */
bool wait_for_sensor_with_retry(SimulatedSensor& sensor,
                                  u32 timeout_ms,
                                  u32 max_retries) {
    printf("\n--- Pattern 1: Blocking Timeout with Retry ---\n");
    printf("Timeout: %lu ms, Max retries: %lu\n", timeout_ms, max_retries);

    for (u32 retry = 0; retry < max_retries; retry++) {
        printf("  Attempt %lu/%lu... ", retry + 1, max_retries);

        u32 start_time = SysTickTimer::millis<board::BoardSysTick>();

        // Poll sensor until ready or timeout
        while (true) {
            if (sensor.poll()) {
                printf("SUCCESS (after %lu attempts)\n", sensor.get_attempts());
                return true;
            }

            // Check timeout
            if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(start_time, timeout_ms)) {
                printf("TIMEOUT\n");
                break;
            }

            // Small delay between polls to avoid busy-waiting
            SysTickTimer::delay_ms<board::BoardSysTick>(10);
        }

        // Reset sensor for next retry
        sensor.reset();

        // Small delay between retries
        if (retry < max_retries - 1) {
            SysTickTimer::delay_ms<board::BoardSysTick>(50);
        }
    }

    printf("  ✗ All retries exhausted. Operation failed.\n");
    return false;
}

// ============================================================================
// Pattern 2: Non-Blocking Timeout
// ============================================================================

enum class PollingState {
    Idle,
    WaitingForSensor,
    ProcessingData,
    Error
};

/**
 * @brief Non-blocking sensor polling with timeout
 *
 * Pattern: Use state machine to avoid blocking.
 * Allows doing other work while waiting.
 */
void demonstrate_non_blocking_timeout() {
    printf("\n--- Pattern 2: Non-Blocking Timeout ---\n");
    printf("Demonstrates state machine pattern with timeout.\n");

    SimulatedSensor sensor(25);  // Ready after 25 polls
    PollingState state = PollingState::Idle;
    u32 timeout_start = 0;
    const u32 TIMEOUT_MS = 1000;
    u32 work_counter = 0;

    printf("Starting non-blocking operation...\n");

    // Run for 5 seconds
    u32 demo_start = SysTickTimer::millis<board::BoardSysTick>();
    while (SysTickTimer::elapsed_ms<board::BoardSysTick>(demo_start) < 5000) {

        switch (state) {
            case PollingState::Idle:
                printf("  [State: Idle] Starting sensor poll...\n");
                sensor.reset();
                timeout_start = SysTickTimer::millis<board::BoardSysTick>();
                state = PollingState::WaitingForSensor;
                break;

            case PollingState::WaitingForSensor:
                // Check if sensor is ready
                if (sensor.poll()) {
                    printf("  [State: WaitingForSensor] Sensor ready! Processing...\n");
                    state = PollingState::ProcessingData;
                }
                // Check timeout
                else if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(
                             timeout_start, TIMEOUT_MS)) {
                    printf("  [State: WaitingForSensor] TIMEOUT! Error state.\n");
                    state = PollingState::Error;
                }
                // Do other work while waiting (non-blocking!)
                else {
                    work_counter++;
                    if (work_counter % 100 == 0) {
                        printf("    (doing other work: %lu iterations)\n", work_counter);
                        board::led::toggle();
                    }
                }
                break;

            case PollingState::ProcessingData:
                printf("  [State: ProcessingData] Data processed successfully!\n");
                work_counter = 0;
                state = PollingState::Idle;
                SysTickTimer::delay_ms<board::BoardSysTick>(2000);  // Wait before next cycle
                break;

            case PollingState::Error:
                printf("  [State: Error] Handling error... Retrying.\n");
                state = PollingState::Idle;
                SysTickTimer::delay_ms<board::BoardSysTick>(500);
                break;
        }

        // Small delay to avoid 100% CPU usage
        SysTickTimer::delay_ms<board::BoardSysTick>(1);
    }

    printf("Non-blocking demo complete. Work iterations: %lu\n", work_counter);
    board::led::off();
}

// ============================================================================
// Pattern 3: Multiple Concurrent Timeouts
// ============================================================================

/**
 * @brief Track multiple operations with different timeouts
 *
 * Pattern: Independent timeout tracking for concurrent operations.
 * Common in communication protocols with multiple transaction types.
 */
void demonstrate_multiple_timeouts() {
    printf("\n--- Pattern 3: Multiple Concurrent Timeouts ---\n");
    printf("Tracking two operations with different timeouts.\n");

    // Operation 1: Fast sensor (50ms timeout)
    u32 op1_start = SysTickTimer::millis<board::BoardSysTick>();
    const u32 OP1_TIMEOUT = 50;
    bool op1_complete = false;

    // Operation 2: Slow sensor (200ms timeout)
    u32 op2_start = SysTickTimer::millis<board::BoardSysTick>();
    const u32 OP2_TIMEOUT = 200;
    bool op2_complete = false;

    u32 iteration = 0;

    printf("Operation 1: 50ms timeout\n");
    printf("Operation 2: 200ms timeout\n\n");

    while (!op1_complete || !op2_complete) {
        iteration++;

        // Check Operation 1
        if (!op1_complete) {
            u32 op1_elapsed = SysTickTimer::elapsed_ms<board::BoardSysTick>(op1_start);

            if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(op1_start, OP1_TIMEOUT)) {
                printf("  [%lu ms] Operation 1: TIMEOUT (expected ~50ms)\n", op1_elapsed);
                op1_complete = true;
            } else if (iteration % 10 == 0) {
                printf("  [%lu ms] Operation 1: waiting... (%lu/%lu ms)\n",
                       op1_elapsed, op1_elapsed, OP1_TIMEOUT);
            }
        }

        // Check Operation 2
        if (!op2_complete) {
            u32 op2_elapsed = SysTickTimer::elapsed_ms<board::BoardSysTick>(op2_start);

            if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(op2_start, OP2_TIMEOUT)) {
                printf("  [%lu ms] Operation 2: TIMEOUT (expected ~200ms)\n", op2_elapsed);
                op2_complete = true;
            } else if (iteration % 10 == 0) {
                printf("  [%lu ms] Operation 2: waiting... (%lu/%lu ms)\n",
                       op2_elapsed, op2_elapsed, OP2_TIMEOUT);
            }
        }

        // Toggle LED to show activity
        if (iteration % 5 == 0) {
            board::led::toggle();
        }

        // Small delay
        SysTickTimer::delay_ms<board::BoardSysTick>(5);
    }

    printf("\n  ✓ Both operations completed (or timed out).\n");
    board::led::off();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    board::init();

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║   Timeout Patterns Example - Non-Blocking Timeouts  ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Board: %s\n", BOARD_NAME);
    printf("System Clock: %lu MHz\n", board::ClockConfig::system_clock_hz / 1000000);
    printf("\n");

    // Pattern 1: Blocking timeout with retry (succeeds on 2nd attempt)
    SimulatedSensor sensor1(15);  // Ready after 15 polls (~150ms at 10ms/poll)
    bool result = wait_for_sensor_with_retry(sensor1, 100, 3);
    printf("  Result: %s\n", result ? "SUCCESS" : "FAILED");

    SysTickTimer::delay_ms<board::BoardSysTick>(1000);

    // Pattern 2: Non-blocking timeout with state machine
    demonstrate_non_blocking_timeout();

    SysTickTimer::delay_ms<board::BoardSysTick>(1000);

    // Pattern 3: Multiple concurrent timeouts
    demonstrate_multiple_timeouts();

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║              All patterns demonstrated!             ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Blink LED slowly to indicate completion
    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    }

    return 0;
}
