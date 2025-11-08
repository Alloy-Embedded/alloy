/// RTOS Blink Example for ESP32 (Bare-Metal)
///
/// Demonstrates Alloy RTOS on ESP32 (Xtensa architecture).
/// Shows preemptive multitasking with multiple tasks at different priorities.
///
/// Tasks:
/// - Task 1: Fast blink (200ms) - High priority
/// - Task 2: Slow blink (1000ms) - Normal priority
/// - Idle: Runs when no other tasks ready
///
/// Hardware:
/// - ESP32 DevKit board
/// - Built-in LED on GPIO2
///
/// Note: This is a bare-metal version without ESP-IDF dependencies.

#include "rtos/rtos.hpp"

#include "core/types.hpp"

#include "esp32_devkit/board.hpp"

using namespace alloy;
using namespace alloy::rtos;

// Simple UART0 output for logging (bare-metal, no ESP-IDF)
namespace uart {
// ESP32 UART0 registers
constexpr core::u32 UART0_BASE = 0x3FF40000;
constexpr core::u32 UART_FIFO_REG = UART0_BASE + 0x00;
constexpr core::u32 UART_STATUS_REG = UART0_BASE + 0x1C;
constexpr core::u32 UART_TX_FIFO_CNT_MASK = 0xFF;
constexpr core::u32 UART_TX_FIFO_CNT_SHIFT = 16;

inline void putchar(char c) {
    volatile core::u32* fifo = reinterpret_cast<volatile core::u32*>(UART_FIFO_REG);
    volatile core::u32* status = reinterpret_cast<volatile core::u32*>(UART_STATUS_REG);

    // Wait for TX FIFO to have space
    while (((*status >> UART_TX_FIFO_CNT_SHIFT) & UART_TX_FIFO_CNT_MASK) >= 128)
        ;

    *fifo = c;
}

void puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            putchar('\r');
        }
        putchar(*str++);
    }
}

void log(const char* level, const char* msg) {
    puts("[");
    puts(level);
    puts("] ");
    puts(msg);
    puts("\n");
}
}  // namespace uart

// Task 1: Fast blink (high priority)
void task1_func() {
    uart::log("INFO", "Task1 started (High priority)");

    while (1) {
        Board::Led::on();
        RTOS::delay(200);  // 200ms

        Board::Led::off();
        RTOS::delay(200);
    }
}

// Task 2: Slow blink (normal priority)
void task2_func() {
    uart::log("INFO", "Task2 started (Normal priority)");

    while (1) {
        uart::log("INFO", "Task2: LED on");
        Board::Led::on();
        RTOS::delay(1000);  // 1000ms

        uart::log("INFO", "Task2: LED off");
        Board::Led::off();
        RTOS::delay(1000);
    }
}

// Idle task (runs when no other tasks ready)
void idle_task_func() {
    uart::log("INFO", "Idle task started");

    core::u32 idle_count = 0;

    while (1) {
        // Idle work - could put CPU to sleep here
        idle_count++;

        // Yield to give other tasks a chance
        RTOS::yield();
    }
}

// Create tasks with different priorities
Task<2048, Priority::High> task1(task1_func, "FastBlink");
Task<2048, Priority::Normal> task2(task2_func, "SlowBlink");
Task<1024, Priority::Idle> idle_task(idle_task_func, "Idle");

int main() {
    // Initialize board (includes GPIO)
    Board::initialize();

    // Initialize LED
    Board::Led::init();

    // UART is already initialized by ROM bootloader at 115200 baud
    uart::log("INFO", "=================================");
    uart::log("INFO", "Alloy RTOS ESP32 Demo");
    uart::log("INFO", "=================================");
    uart::log("INFO", "Starting RTOS with 3 tasks...");
    uart::log("INFO", "  Task1: High priority, 200ms");
    uart::log("INFO", "  Task2: Normal priority, 1000ms");
    uart::log("INFO", "  Idle: Idle priority");
    uart::log("INFO", "=================================");

    // Start RTOS (never returns)
    RTOS::start();

    return 0;  // Never reached
}
