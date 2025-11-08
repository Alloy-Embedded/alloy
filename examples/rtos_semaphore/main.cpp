/// RTOS Semaphore Example
///
/// Demonstrates binary and counting semaphores:
/// - Binary semaphore: ISR-like event signaling
/// - Counting semaphore: Resource pool management
/// - LED blink patterns indicate semaphore activity
///
/// Hardware: STM32F103 (Bluepill)
/// - LED on PC13 shows different patterns based on semaphore state

#include "rtos/rtos.hpp"
#include "rtos/semaphore.hpp"

#include "core/types.hpp"

#include "stm32f103c8/board.hpp"

using namespace alloy;
using namespace alloy::rtos;

/// Binary semaphore for event signaling (simulates ISR → Task)
BinarySemaphore event_signal;

/// Counting semaphore for resource pool (3 resources)
CountingSemaphore<3> resource_pool(3);

/// Simulate an ISR that signals events every 500ms
///
/// In real application, this would be actual hardware interrupt.
/// Here we simulate it with a high-priority task.
void event_generator_task_func() {
    while (1) {
        // Simulate event occurring (e.g., data ready from sensor)
        RTOS::delay(500);

        // Signal event (like ISR would do)
        event_signal.give();

        // Quick blink to show event generated
        Board::Led::on();
        RTOS::delay(10);
        Board::Led::off();
    }
}

/// Event handler task - waits for binary semaphore
///
/// Blocks until event_signal is given.
/// Demonstrates typical ISR → Task synchronization pattern.
void event_handler_task_func() {
    while (1) {
        // Wait for event (blocks until signaled)
        if (event_signal.take(1000)) {
            // Event occurred - handle it
            // Show longer blink to indicate event processing
            Board::Led::on();
            RTOS::delay(100);
            Board::Led::off();

            // Simulate event processing time
            RTOS::delay(50);
        } else {
            // Timeout - no event in 1 second
            // Indicate timeout with 3 quick blinks
            for (int i = 0; i < 3; i++) {
                Board::Led::on();
                RTOS::delay(30);
                Board::Led::off();
                RTOS::delay(30);
            }
        }
    }
}

/// Resource user task 1 - competes for resources from pool
///
/// Uses counting semaphore to manage shared resource pool.
void resource_user1_task_func() {
    while (1) {
        // Try to get resource from pool
        if (resource_pool.take(200)) {
            // Got resource - use it
            // Show double blink to indicate resource in use
            Board::Led::on();
            RTOS::delay(50);
            Board::Led::off();
            RTOS::delay(30);
            Board::Led::on();
            RTOS::delay(50);
            Board::Led::off();

            // Simulate work with resource (200ms)
            RTOS::delay(200);

            // Return resource to pool
            resource_pool.give();
        }

        // Wait before trying again
        RTOS::delay(300);
    }
}

/// Resource user task 2 - also competes for resources
///
/// Demonstrates multiple tasks sharing limited resources via counting semaphore.
void resource_user2_task_func() {
    while (1) {
        // Try to get resource from pool
        if (resource_pool.take(200)) {
            // Got resource - different blink pattern
            // Triple blink for task 2
            for (int i = 0; i < 3; i++) {
                Board::Led::on();
                RTOS::delay(40);
                Board::Led::off();
                RTOS::delay(20);
            }

            // Simulate work with resource (150ms)
            RTOS::delay(150);

            // Return resource to pool
            resource_pool.give();
        }

        // Wait before trying again
        RTOS::delay(400);
    }
}

/// Idle task (runs when no other tasks ready)
void idle_task_func() {
    while (1) {
        // Enter low-power mode
        __asm volatile("wfi");
    }
}

// Create tasks with different priorities
// Event generator has highest priority (simulates ISR timing)
Task<256, Priority::Highest> event_gen_task(event_generator_task_func, "EventGen");
Task<512, Priority::High> event_handler_task(event_handler_task_func, "EventHandler");
Task<512, Priority::Normal> resource_user1_task(resource_user1_task_func, "ResUser1");
Task<512, Priority::Normal> resource_user2_task(resource_user2_task_func, "ResUser2");
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
