/**
 * ESP32 BLE Scanner Example - Alloy Framework
 *
 * Demonstrates:
 * - BLE initialization and scanning
 * - Device discovery with RSSI
 * - Advertisement data parsing
 * - Periodic scanning
 * - Device filtering and sorting
 */

#include "ble/central.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "ble_scanner";

using alloy::ble::Central;
using alloy::ble::DeviceInfo;
using alloy::ble::ScanConfig;

// Global BLE scanner instance
static Central scanner;

// ============================================================================
// Helper Functions
// ============================================================================

/// Format BLE address as string
void format_address(const alloy::ble::Address& addr, char* buffer, size_t size) {
    snprintf(buffer, size, "%02X:%02X:%02X:%02X:%02X:%02X",
        addr.addr[0], addr.addr[1], addr.addr[2],
        addr.addr[3], addr.addr[4], addr.addr[5]);
}

/// Get signal strength bar representation
const char* rssi_bars(i8 rssi) {
    if (rssi >= -50) return "\u2588\u2588\u2588\u2588";  // ████ Excellent
    if (rssi >= -60) return "\u2588\u2588\u2588\u2591";  // ███░ Good
    if (rssi >= -70) return "\u2588\u2588\u2591\u2591";  // ██░░ Fair
    if (rssi >= -80) return "\u2588\u2591\u2591\u2591";  // █░░░ Poor
    return "\u2591\u2591\u2591\u2591";                    // ░░░░ Very Poor
}

/// Get signal quality description
const char* rssi_quality(i8 rssi) {
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Poor";
    return "Very Poor";
}

/// Print device information
void print_device(const DeviceInfo& dev, u8 index) {
    char addr_str[18];
    format_address(dev.address, addr_str, sizeof(addr_str));

    ESP_LOGI(TAG, "Device #%d:", index + 1);
    ESP_LOGI(TAG, "  Address: %s", addr_str);

    if (dev.name[0] != '\0') {
        ESP_LOGI(TAG, "  Name:    %s", dev.name);
    } else {
        ESP_LOGI(TAG, "  Name:    (Unknown)");
    }

    ESP_LOGI(TAG, "  RSSI:    %d dBm %s [%s]",
        dev.rssi, rssi_bars(dev.rssi), rssi_quality(dev.rssi));
    ESP_LOGI(TAG, "");
}

/// Print scan summary
void print_scan_summary(const DeviceInfo* devices, u8 count) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Scan Complete - Found %d device(s)", count);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    if (count == 0) {
        ESP_LOGI(TAG, "No devices found. Make sure:");
        ESP_LOGI(TAG, "  - Bluetooth is enabled on your phone/device");
        ESP_LOGI(TAG, "  - Device is in discoverable mode");
        ESP_LOGI(TAG, "  - You're within range");
        return;
    }

    // Sort devices by RSSI (strongest first)
    DeviceInfo sorted[32];
    memcpy(sorted, devices, count * sizeof(DeviceInfo));

    for (u8 i = 0; i < count - 1; i++) {
        for (u8 j = i + 1; j < count; j++) {
            if (sorted[j].rssi > sorted[i].rssi) {
                DeviceInfo temp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }

    // Print sorted devices
    for (u8 i = 0; i < count; i++) {
        print_device(sorted[i], i);
    }
}

/// Scan callback - called for each discovered device
void on_device_found(const DeviceInfo& dev) {
    char addr_str[18];
    format_address(dev.address, addr_str, sizeof(addr_str));

    if (dev.name[0] != '\0') {
        ESP_LOGI(TAG, "Found: %s (%s) RSSI: %d dBm",
            dev.name, addr_str, dev.rssi);
    } else {
        ESP_LOGI(TAG, "Found: %s RSSI: %d dBm",
            addr_str, dev.rssi);
    }
}

// ============================================================================
// Scanning Functions
// ============================================================================

/// Perform single blocking scan
void do_single_scan() {
    ESP_LOGI(TAG, "Starting 10-second scan...");
    ESP_LOGI(TAG, "");

    // Configure scan
    ScanConfig config;
    config.duration = 10000;  // 10 seconds
    config.type = alloy::ble::ScanType::Active;

    // Perform scan
    auto result = scanner.scan(config);

    if (!result.is_ok()) {
        ESP_LOGE(TAG, "Scan failed");
        return;
    }

    // Get and print results
    DeviceInfo devices[32];
    auto get_result = scanner.get_scan_results(devices, 32);

    if (get_result.is_ok()) {
        print_scan_summary(devices, get_result.value());
    }
}

/// Perform continuous scanning
void do_continuous_scan() {
    ESP_LOGI(TAG, "Starting continuous scanning (5s intervals)");
    ESP_LOGI(TAG, "");

    u8 scan_num = 1;

    while (true) {
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Scan #%d - Starting...", scan_num);
        ESP_LOGI(TAG, "========================================");

        // Configure scan
        ScanConfig config;
        config.duration = 5000;  // 5 seconds
        config.type = alloy::ble::ScanType::Active;

        // Perform scan
        auto result = scanner.scan(config);

        if (result.is_ok()) {
            // Get results
            DeviceInfo devices[32];
            auto get_result = scanner.get_scan_results(devices, 32);

            if (get_result.is_ok()) {
                u8 count = get_result.value();
                ESP_LOGI(TAG, "Found %d device(s)", count);

                // Print only new/changed devices
                for (u8 i = 0; i < count && i < 5; i++) {  // Show top 5
                    char addr_str[18];
                    format_address(devices[i].address, addr_str, sizeof(addr_str));

                    if (devices[i].name[0] != '\0') {
                        ESP_LOGI(TAG, "  %d. %s (%d dBm)", i + 1, devices[i].name, devices[i].rssi);
                    } else {
                        ESP_LOGI(TAG, "  %d. %s (%d dBm)", i + 1, addr_str, devices[i].rssi);
                    }
                }

                if (count > 5) {
                    ESP_LOGI(TAG, "  ... and %d more", count - 5);
                }
            }
        } else {
            ESP_LOGE(TAG, "Scan failed");
        }

        ESP_LOGI(TAG, "");
        scan_num++;

        // Wait 2 seconds before next scan
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/// Perform async scan with callback
void do_async_scan() {
    ESP_LOGI(TAG, "Starting async scan with live updates...");
    ESP_LOGI(TAG, "");

    // Set callback for live updates
    scanner.set_scan_callback(on_device_found);

    // Configure scan
    ScanConfig config;
    config.duration = 15000;  // 15 seconds
    config.type = alloy::ble::ScanType::Active;

    // Start async scan
    auto result = scanner.scan_async(config);

    if (!result.is_ok()) {
        ESP_LOGE(TAG, "Failed to start async scan");
        return;
    }

    ESP_LOGI(TAG, "Async scan started, waiting for results...");

    // Wait for scan to complete
    while (scanner.is_scanning()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Async scan complete");

    // Get final results
    DeviceInfo devices[32];
    auto get_result = scanner.get_scan_results(devices, 32);

    if (get_result.is_ok()) {
        print_scan_summary(devices, get_result.value());
    }
}

// ============================================================================
// Main Application
// ============================================================================

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Alloy BLE Scanner Example");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Initialize BLE scanner
    ESP_LOGI(TAG, "Initializing BLE...");
    auto init_result = scanner.init();

    if (!init_result.is_ok()) {
        ESP_LOGE(TAG, "Failed to initialize BLE");
        return;
    }

    ESP_LOGI(TAG, "✓ BLE initialized successfully");
    ESP_LOGI(TAG, "");

    // Uncomment one of the following to select scanning mode:

    // Mode 1: Single scan
    // do_single_scan();

    // Mode 2: Continuous scanning
    do_continuous_scan();

    // Mode 3: Async scan with callbacks
    // do_async_scan();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Example finished");
    ESP_LOGI(TAG, "========================================");
}
