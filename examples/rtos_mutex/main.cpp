/// RTOS Mutex Example - Priority Inheritance Demonstration
///
/// Demonstrates mutex with priority inheritance to prevent priority inversion.
///
/// Scenario:
/// - Low-priority task acquires mutex and holds it for 200ms
/// - Medium-priority task runs CPU-intensive work
/// - High-priority task tries to acquire mutex (blocks)
///
/// Without priority inheritance:
/// - High-priority task would be blocked by medium-priority task
/// - This is "priority inversion" - breaks real-time guarantees
///
/// With priority inheritance:
/// - Low-priority task inherits high priority while holding mutex
/// - Low-priority task (now high) preempts medium-priority task
/// - High-priority task gets mutex quickly
///
/// Hardware: STM32F103 (Bluepill)
/// - LED blink patterns indicate which task is running

#include "rtos/mutex.hpp"
#include "rtos/rtos.hpp"

#include "core/types.hpp"

#include "stm32f103c8/board.hpp"

using namespace alloy;
using namespace alloy::rtos;

/// Shared mutex protecting critical resource
Mutex resource_mutex;

/// Simulated shared resource (e.g., UART, SPI, etc.)
static volatile core::u32 shared_resource = 0;

/// Low-priority task - acquires mutex and holds it
///
/// This task simulates a low-priority background task that
/// occasionally needs exclusive access to a shared resource.
void low_priority_task_func() {
    while (1) {
        // Every 1 second, acquire mutex
        RTOS::delay(1000);

        // Use RAII lock guard for exception safety
        {
            LockGuard guard(resource_mutex);

            if (guard.is_locked()) {
                // Indicate low-priority task has mutex (1 long blink)
                Board::Led::on();
                RTOS::delay(300);
                Board::Led::off();

                // Hold mutex for 200ms (simulating long operation)
                // During this time, if high-priority task requests mutex,
                // priority inheritance will boost this task's priority
                for (int i = 0; i < 20; i++) {
                    shared_resource++;  // Access shared resource
                    RTOS::delay(10);
                }
            }
            // Mutex automatically unlocked when guard goes out of scope
        }

        // Small delay before next cycle
        RTOS::delay(100);
    }
}

/// Medium-priority task - CPU-intensive work, no mutex
///
/// This task does CPU-intensive work without needing the mutex.
/// Without priority inheritance, this task would prevent
/// low-priority task from finishing and releasing mutex,
/// causing high-priority task to wait indefinitely.
void medium_priority_task_func() {
    while (1) {
        // Do CPU-intensive work (simulate with delays)
        // 2 quick blinks indicate medium-priority task running
        Board::Led::on();
        RTOS::delay(50);
        Board::Led::off();
        RTOS::delay(50);
        Board::Led::on();
        RTOS::delay(50);
        Board::Led::off();

        // Work for 300ms total
        RTOS::delay(200);
    }
}

/// High-priority task - needs mutex urgently
///
/// This task represents a high-priority operation that needs
/// quick access to the shared resource. Priority inheritance
/// ensures it doesn't wait too long.
void high_priority_task_func() {
    while (1) {
        // Every 800ms, try to acquire mutex
        RTOS::delay(800);

        core::u32 start = systick::micros();

        // Acquire mutex (will trigger priority inheritance if needed)
        if (resource_mutex.lock(500)) {  // 500ms timeout
            core::u32 wait_time_us = systick::micros_since(start);

            // Indicate high-priority task has mutex (3 rapid blinks)
            for (int i = 0; i < 3; i++) {
                Board::Led::on();
                RTOS::delay(30);
                Board::Led::off();
                RTOS::delay(30);
            }

            // Quick access to shared resource
            shared_resource += 1000;

            // Show wait time via LED pattern
            // Short wait (<50ms): 1 blink
            // Medium wait (50-100ms): 2 blinks
            // Long wait (>100ms): 3 blinks
            RTOS::delay(100);

            core::u32 wait_time_ms = wait_time_us / 1000;
            int blink_count = (wait_time_ms < 50) ? 1 : (wait_time_ms < 100) ? 2 : 3;

            for (int i = 0; i < blink_count; i++) {
                Board::Led::on();
                RTOS::delay(40);
                Board::Led::off();
                RTOS::delay(40);
            }

            // Release mutex
            resource_mutex.unlock();
        } else {
            // Timeout - indicate with 5 rapid blinks
            for (int i = 0; i < 5; i++) {
                Board::Led::on();
                RTOS::delay(20);
                Board::Led::off();
                RTOS::delay(20);
            }
        }

        // Small delay before next try
        RTOS::delay(100);
    }
}

/// Idle task
void idle_task_func() {
    while (1) {
        __asm volatile("wfi");
    }
}

// Create tasks with different priorities
// Note the priority levels - this creates the priority inversion scenario
Task<512, Priority::Low> low_task(low_priority_task_func, "LowPri");
Task<512, Priority::Normal> medium_task(medium_priority_task_func, "MediumPri");
Task<512, Priority::Highest> high_task(high_priority_task_func, "HighPri");
Task<256, Priority::Idle> idle_task(idle_task_func, "Idle");

int main() {
    // Initialize board (includes SysTick)
    Board::initialize();

    // Initialize LED
    Board::Led::init();

    // Start RTOS (never returns)
    RTOS::start();

    return 0;
}

// Weak symbols for startup code
extern "C" {
void SystemInit() {
    // Running on default HSI (8MHz)
}
}
