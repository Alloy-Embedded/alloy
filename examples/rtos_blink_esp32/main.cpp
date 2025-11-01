/// RTOS Blink Example for ESP32
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

#include "esp32_devkit/board.hpp"
#include "rtos/rtos.hpp"
#include "core/types.hpp"

// ESP-IDF includes
#include "esp_log.h"

using namespace alloy;
using namespace alloy::rtos;

static const char* TAG = "RTOS_BLINK";

// Task 1: Fast blink (high priority)
void task1_func() {
    ESP_LOGI(TAG, "Task1 started (High priority)");

    while (1) {
        Board::Led::on();
        RTOS::delay(200);  // 200ms

        Board::Led::off();
        RTOS::delay(200);

        ESP_LOGD(TAG, "Task1 tick");
    }
}

// Task 2: Slow blink (normal priority)
void task2_func() {
    ESP_LOGI(TAG, "Task2 started (Normal priority)");

    while (1) {
        ESP_LOGI(TAG, "Task2: LED on");
        Board::Led::on();
        RTOS::delay(1000);  // 1000ms

        ESP_LOGI(TAG, "Task2: LED off");
        Board::Led::off();
        RTOS::delay(1000);
    }
}

// Idle task (runs when no other tasks ready)
void idle_task_func() {
    ESP_LOGI(TAG, "Idle task started");

    core::u32 idle_count = 0;

    while (1) {
        // Idle work - could put CPU to sleep here
        idle_count++;

        if (idle_count % 100000 == 0) {
            ESP_LOGD(TAG, "Idle count: %u", idle_count);
        }

        // Yield to give other tasks a chance
        RTOS::yield();
    }
}

// Create tasks with different priorities
Task<2048, Priority::High>   task1(task1_func, "FastBlink");
Task<2048, Priority::Normal> task2(task2_func, "SlowBlink");
Task<1024, Priority::Idle>   idle_task(idle_task_func, "Idle");

extern "C" void app_main() {
    // Initialize board (includes GPIO)
    Board::initialize();

    // Initialize LED
    Board::Led::init();

    ESP_LOGI(TAG, "Alloy RTOS ESP32 Demo");
    ESP_LOGI(TAG, "Starting RTOS with 3 tasks...");
    ESP_LOGI(TAG, "  - Task1: High priority, 200ms blink");
    ESP_LOGI(TAG, "  - Task2: Normal priority, 1000ms blink");
    ESP_LOGI(TAG, "  - Idle: Idle priority, runs when others blocked");

    // Start RTOS (never returns)
    RTOS::start();
}
