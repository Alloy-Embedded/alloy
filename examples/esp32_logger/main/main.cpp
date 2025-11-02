/// ESP32 Logger Example - Multiple Sinks
///
/// Demonstrates CoreZero universal logger on ESP32 with:
/// - Multiple sinks (UART + ESP-IDF bridge)
/// - All log levels
/// - Integration with ESP-IDF logging system
/// - Periodic logging from FreeRTOS task
///
/// This example shows how CoreZero logs can coexist with
/// ESP-IDF's native logging system.

#include "logger/logger.hpp"
#include "logger/platform/uart_sink.hpp"
#include "logger/platform/esp_log_sink.hpp"
#include "hal/espressif/esp32/uart.hpp"
#include "hal/espressif/esp32/systick.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"

using namespace alloy;
using namespace alloy::hal::esp32;

// Tag for ESP-IDF logging
static const char* TAG = "CoreZero";

// UART0 type (console)
using Uart0 = UartDevice<UartId::UART0>;

/**
 * Demonstrate all log levels
 */
void demonstrate_log_levels() {
    LOG_INFO("=== Demonstrating Log Levels ===");

    LOG_TRACE("TRACE: Very detailed diagnostic information");
    LOG_DEBUG("DEBUG: Debug information for development");
    LOG_INFO("INFO: Normal operation information");
    LOG_WARN("WARN: Warning about potential issues");
    LOG_ERROR("ERROR: Error that occurred");

    LOG_INFO("=== All levels demonstrated ===");
}

/**
 * Demonstrate formatted logging
 */
void demonstrate_formatting() {
    LOG_INFO("=== Demonstrating Formatting ===");

    // Integer formatting
    int counter = 42;
    LOG_INFO("Integer: %d", counter);
    LOG_INFO("Hex: 0x%08x", counter);

    // Float formatting
    float temperature = 23.456f;
    LOG_INFO("Float: %.2f°C", temperature);

    // String formatting
    const char* status = "Running";
    LOG_INFO("Status: %s", status);

    // Multiple arguments
    LOG_INFO("Counter=%d, Temp=%.1f°C, Status=%s", counter, temperature, status);

    LOG_INFO("=== Formatting demonstrated ===");
}

/**
 * Demonstrate runtime level control
 */
void demonstrate_level_control() {
    LOG_INFO("=== Demonstrating Level Control ===");

    LOG_INFO("Current level allows INFO and above");
    LOG_DEBUG("This DEBUG message should appear");

    // Change to ERROR only
    LOG_INFO("Changing level to ERROR only...");
    logger::Logger::set_level(logger::Level::Error);

    LOG_INFO("This INFO should NOT appear");
    LOG_WARN("This WARN should NOT appear");
    LOG_ERROR("This ERROR should appear");

    // Reset to INFO
    logger::Logger::set_level(logger::Level::Info);
    LOG_INFO("Level reset to INFO");
}

/**
 * Logging task - logs periodically
 */
void logging_task(void* pvParameters) {
    (void)pvParameters;

    uint32_t counter = 0;

    while (true) {
        LOG_INFO("Task iteration: %lu", counter);

        // Every 5 iterations, show debug info
        if (counter % 5 == 0) {
            LOG_DEBUG("Debug: Free heap: %lu bytes", esp_get_free_heap_size());
        }

        // Every 10 iterations, show warning
        if (counter % 10 == 0) {
            LOG_WARN("Warning: Counter reached %lu", counter);
        }

        counter++;

        // Delay 2 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

extern "C" void app_main() {
    // Initialize NVS (required for ESP32)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Initialize SysTick for timestamps
    auto systick_result = SysTick::init();
    if (systick_result.is_error()) {
        printf("Failed to initialize SysTick\n");
        return;
    }

    // Initialize UART0 (console)
    auto uart_result = Uart0::init();
    if (uart_result.is_error()) {
        printf("Failed to initialize UART0\n");
        return;
    }

    // Configure UART: 115200 baud
    hal::UartConfig uart_config{
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = hal::Parity::None
    };

    auto config_result = Uart0::configure(uart_config);
    if (config_result.is_error()) {
        printf("Failed to configure UART0\n");
        return;
    }

    // Create sinks
    logger::UartSink<Uart0> uart_sink;
    logger::EspLogSink esp_sink("CoreZero");

    // Register both sinks - logs will go to both UART and ESP-IDF system
    logger::Logger::add_sink(&uart_sink);
    logger::Logger::add_sink(&esp_sink);

    // Configure logger to show all levels
    logger::Logger::set_level(logger::Level::Trace);

    // Log startup message
    LOG_INFO("========================================");
    LOG_INFO("  CoreZero Logger - ESP32 Example");
    LOG_INFO("========================================");
    LOG_INFO("Chip: %s", CONFIG_IDF_TARGET);
    LOG_INFO("Free heap: %lu bytes", esp_get_free_heap_size());
    LOG_INFO("Logger initialized with 2 sinks:");
    LOG_INFO("  - UART sink (direct output)");
    LOG_INFO("  - ESP-IDF sink (via esp_log)");
    LOG_INFO("========================================");

    // Demonstrate features
    vTaskDelay(pdMS_TO_TICKS(1000));
    demonstrate_log_levels();

    vTaskDelay(pdMS_TO_TICKS(1000));
    demonstrate_formatting();

    vTaskDelay(pdMS_TO_TICKS(1000));
    demonstrate_level_control();

    LOG_INFO("========================================");
    LOG_INFO("Starting periodic logging task...");
    LOG_INFO("========================================");

    // Create logging task
    xTaskCreate(
        logging_task,
        "logging_task",
        4096,  // Stack size
        nullptr,
        5,     // Priority
        nullptr
    );

    // Main task done - FreeRTOS scheduler continues
    LOG_INFO("Main task complete - logging task running");
}
