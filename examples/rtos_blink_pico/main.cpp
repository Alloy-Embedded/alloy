/// RTOS Blink Example for Raspberry Pi Pico
///
/// Demonstrates Alloy RTOS on RP2040 (ARM Cortex-M0+).
/// Shows preemptive multitasking with multiple tasks at different priorities.
///
/// Tasks:
/// - Task 1: Fast blink (100ms) - High priority
/// - Task 2: Slow blink (500ms) - Normal priority
/// - Task 3: Status print (2000ms) - Low priority
/// - Idle: Runs when no other tasks ready
///
/// Hardware:
/// - Raspberry Pi Pico board
/// - Built-in LED on GPIO25

#include "raspberry_pi_pico/board.hpp"
#include "rtos/rtos.hpp"
#include "core/types.hpp"

using namespace alloy;
using namespace alloy::rtos;

// Task statistics
static volatile core::u32 task1_count = 0;
static volatile core::u32 task2_count = 0;
static volatile core::u32 idle_count = 0;

// Task 1: Fast blink (high priority)
void task1_func() {
    while (1) {
        Board::Led::on();
        RTOS::delay(100);  // 100ms

        Board::Led::off();
        RTOS::delay(100);

        task1_count++;
    }
}

// Task 2: Slow blink (normal priority)
void task2_func() {
    while (1) {
        Board::Led::on();
        RTOS::delay(500);  // 500ms

        Board::Led::off();
        RTOS::delay(500);

        task2_count++;
    }
}

// Task 3: Status printer (low priority)
void task3_func() {
    while (1) {
        // In a real application, this could print via UART/USB
        // For now, just increment counter
        RTOS::delay(2000);  // 2000ms
    }
}

// Idle task (runs when no other tasks ready)
void idle_task_func() {
    while (1) {
        // Idle work - could put CPU to low power mode here
        idle_count++;

        // Yield to give scheduler a chance
        RTOS::yield();
    }
}

// Create tasks with different priorities and stack sizes
Task<512, Priority::High>    task1(task1_func, "FastBlink");
Task<512, Priority::Normal>  task2(task2_func, "SlowBlink");
Task<256, Priority::Low>     task3(task3_func, "Status");
Task<256, Priority::Idle>    idle_task(idle_task_func, "Idle");

int main() {
    // Initialize board (clocks, GPIO)
    Board::initialize();

    // Initialize LED
    Board::Led::init();

    // Quick startup blink to show we're alive
    for (int i = 0; i < 3; i++) {
        Board::Led::on();
        for (volatile int j = 0; j < 1000000; j++);
        Board::Led::off();
        for (volatile int j = 0; j < 1000000; j++);
    }

    // Start RTOS (never returns)
    RTOS::start();

    return 0;
}
