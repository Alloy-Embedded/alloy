/// RTOS Blink Example
///
/// Demonstrates basic RT OS functionality with two tasks:
/// - Task 1: Blinks LED fast (100ms on/off) - High priority
/// - Task 2: Blinks LED slow (500ms on/off) - Low priority
///
/// This shows preemptive multitasking where high priority task
/// interrupts low priority task.

#include "rtos/rtos.hpp"

#include "core/types.hpp"

#include "stm32f103c8/board.hpp"

using namespace alloy;
using namespace alloy::rtos;

// Task 1: Fast blink (high priority)
void task1_func() {
    while (1) {
        Board::Led::on();
        RTOS::delay(100);  // 100ms
        Board::Led::off();
        RTOS::delay(100);
    }
}

// Task 2: Slow blink (low priority)
void task2_func() {
    while (1) {
        Board::Led::on();
        RTOS::delay(500);  // 500ms
        Board::Led::off();
        RTOS::delay(500);
    }
}

// Idle task (runs when no other tasks ready)
void idle_task_func() {
    while (1) {
        // Just wait for interrupt
        __asm volatile("wfi");
    }
}

// Create tasks with different priorities
Task<512, Priority::High> task1(task1_func, "Fast");
Task<512, Priority::Normal> task2(task2_func, "Slow");
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
    // Optional: Configure clocks here
    // For now, running on default HSI (8MHz)
}
}
