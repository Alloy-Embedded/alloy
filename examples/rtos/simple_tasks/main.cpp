/**
 * @file main.cpp
 * @brief Simple RTOS Multi-Task Example
 *
 * Demonstrates basic RTOS functionality with multiple tasks:
 * - Task priorities (High, Normal, Low)
 * - RTOS::delay() for task scheduling
 * - LED blinking patterns showing task execution
 * - SysTick integration for timing
 *
 * ## Hardware Setup
 * - Any supported board (Nucleo-F401RE, F722ZE, G071RB, G0B1RE, SAME70)
 * - Built-in LED
 *
 * ## Expected Behavior
 * The LED will show different blink patterns as tasks execute:
 * - Fast blink (100ms): High priority task running
 * - Medium blink (500ms): Normal priority task running
 * - Slow blink (1000ms): Low priority task running
 *
 * Tasks are scheduled based on priority. Higher priority tasks
 * preempt lower priority tasks.
 */

#include "board.hpp"
#include "rtos/rtos.hpp"

using namespace alloy::rtos;
using namespace alloy::core;

// =============================================================================
// Task Functions
// =============================================================================

/**
 * @brief High priority task - Fast LED blink
 *
 * This task runs every 500ms and toggles the LED.
 * Being high priority, it will preempt lower priority tasks.
 */
void high_priority_task_func() {
    // Signal we entered the task (1 very fast blink)
    board::led::on();
    for (volatile int j = 0; j < 100000; j++);
    board::led::off();
    for (volatile int j = 0; j < 100000; j++);

    while (true) {
        board::led::toggle();
        RTOS::delay(500);  // 500ms delay for visible blink
    }
}

/**
 * @brief Normal priority task - Does not toggle LED
 *
 * This task just delays to demonstrate multitasking.
 * It will be preempted by high priority task.
 */
void normal_priority_task_func() {
    while (true) {
        // Check RTOS tick count (for demonstration)
        u32 tick = RTOS::get_tick_count();

        // Just delay, don't toggle LED to avoid conflicts
        RTOS::delay(500);

        (void)tick;  // Suppress unused warning
    }
}

/**
 * @brief Low priority task - Does not toggle LED
 *
 * This task runs every 1000ms.
 * It has the lowest priority and runs when other tasks are idle.
 */
void low_priority_task_func() {
    while (true) {
        // Just delay, don't toggle LED to avoid conflicts
        RTOS::delay(1000);  // 1s delay
    }
}

/**
 * @brief Idle task - Runs when no other tasks are ready
 *
 * The idle task has the lowest priority (Priority::Idle).
 * It runs only when all other tasks are blocked or delayed.
 */
void idle_task_func() {
    while (true) {
        // Put CPU to sleep to save power
        #if defined(__ARM_ARCH)
            __asm volatile("wfi");  // Wait for interrupt
        #endif

        // Could also do background work here:
        // - Check for stack overflow
        // - Gather statistics
        // - Run garbage collection
    }
}

// =============================================================================
// Task Instances
// =============================================================================

/**
 * @brief Task declarations
 *
 * Each task is created with:
 * - Template parameter 1: Stack size in bytes (must be >= 256, 8-byte aligned)
 * - Template parameter 2: Priority level (Idle, Low, Normal, High, Critical)
 * - Constructor parameter 1: Task function pointer
 * - Constructor parameter 2: Task name (for debugging)
 */

// High priority task - 512 bytes stack
Task<512, Priority::High> high_task(high_priority_task_func, "HighTask");

// Normal priority task - 512 bytes stack
Task<512, Priority::Normal> normal_task(normal_priority_task_func, "NormalTask");

// Low priority task - 256 bytes stack (minimum)
Task<256, Priority::Low> low_task(low_priority_task_func, "LowTask");

// Idle task - runs when nothing else is ready
Task<256, Priority::Idle> idle_task(idle_task_func, "IdleTask");

// =============================================================================
// Main Application
// =============================================================================

/**
 * @brief Main entry point
 *
 * Initializes the board and starts the RTOS scheduler.
 * This function never returns - the RTOS scheduler takes over.
 */
int main() {
    // Initialize board hardware
    board::init();

    // Test 1: Board init works (3 quick blinks)
    for (int i = 0; i < 3; i++) {
        board::led::on();
        for (volatile int j = 0; j < 500000; j++);
        board::led::off();
        for (volatile int j = 0; j < 500000; j++);
    }

    // Delay before starting RTOS
    for (volatile int j = 0; j < 2000000; j++);

    // Test 2: About to start RTOS (2 slow blinks)
    for (int i = 0; i < 2; i++) {
        board::led::on();
        for (volatile int j = 0; j < 1000000; j++);
        board::led::off();
        for (volatile int j = 0; j < 1000000; j++);
    }

    // Start RTOS scheduler - should never return
    RTOS::start();

    // Test 3: If we get here, RTOS didn't start (fast blinking forever)
    while (1) {
        board::led::toggle();
        for (volatile int j = 0; j < 200000; j++);
    }

    return 0;
}

// =============================================================================
// Memory Footprint Analysis
// =============================================================================

/**
 * @brief Total RTOS memory usage:
 *
 * Task Control Blocks (TCBs):
 * - 4 tasks × 32 bytes = 128 bytes
 *
 * Task Stacks:
 * - high_task:   512 bytes
 * - normal_task: 512 bytes
 * - low_task:    256 bytes
 * - idle_task:   256 bytes
 * - Total:      1536 bytes
 *
 * RTOS Core:
 * - Ready queue:      ~36 bytes
 * - Scheduler state:  ~24 bytes
 * - Total:            ~60 bytes
 *
 * Grand Total: ~1724 bytes RAM
 *
 * This is computed at compile-time and validated to fit in available RAM.
 */

// =============================================================================
// Task Scheduling Example Timeline
// =============================================================================

/**
 * @brief Example scheduling timeline:
 *
 * Time (ms)  | Running Task | Action
 * -----------|--------------|---------------------------
 * 0          | high_task    | LED toggle, delay 100ms
 * 100        | high_task    | LED toggle, delay 100ms
 * 200        | high_task    | LED toggle, delay 100ms
 * ...        | ...          | ...
 *
 * When high_task delays, scheduler picks next highest priority ready task.
 *
 * The LED will appear to blink at the rate of the highest priority task
 * that is currently running.
 *
 * Note: This is a simplified example. In a real application:
 * - Tasks would do actual work (read sensors, process data, etc.)
 * - Inter-task communication would use queues, mutexes, semaphores
 * - Task priorities would be chosen based on deadlines and importance
 */
