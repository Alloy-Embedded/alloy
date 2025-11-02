/**
 * ESP32 WiFi Scanner Example - Alloy Framework
 *
 * This example demonstrates:
 * - Using Alloy's WiFi Scanner abstraction
 * - Scanning for nearby WiFi networks
 * - Displaying network information (SSID, RSSI, channel, auth mode)
 * - Both blocking and async scanning modes
 * - Filtering networks by signal strength
 * - Clean C++ API with Result<T> error handling
 *
 * This example continuously scans for WiFi networks every 10 seconds
 * and displays them sorted by signal strength.
 */

#include "wifi/scanner.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <algorithm>

static const char *TAG = "wifi_scanner";

using alloy::wifi::Scanner;
using alloy::wifi::AccessPointInfo;
using alloy::wifi::ScanConfig;
using alloy::wifi::AuthMode;
using alloy::core::Result;

// Global scanner instance
static Scanner scanner;

/**
 * Get authentication mode as string
 */
static const char* auth_mode_to_string(AuthMode mode) {
    switch (mode) {
        case AuthMode::Open:
            return "Open";
        case AuthMode::WEP:
            return "WEP";
        case AuthMode::WPA_PSK:
            return "WPA-PSK";
        case AuthMode::WPA2_PSK:
            return "WPA2-PSK";
        case AuthMode::WPA_WPA2_PSK:
            return "WPA/WPA2-PSK";
        case AuthMode::WPA2_ENTERPRISE:
            return "WPA2-Enterprise";
        case AuthMode::WPA3_PSK:
            return "WPA3-PSK";
        case AuthMode::WPA2_WPA3_PSK:
            return "WPA2/WPA3-PSK";
        default:
            return "Unknown";
    }
}

/**
 * Get signal strength quality descriptor
 */
static const char* rssi_to_quality(int8_t rssi) {
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Weak";
    return "Very Weak";
}

/**
 * Get signal bars representation
 */
static const char* rssi_to_bars(int8_t rssi) {
    if (rssi >= -50) return "▂▄▆█";  // 4 bars
    if (rssi >= -60) return "▂▄▆_";  // 3 bars
    if (rssi >= -70) return "▂▄__";  // 2 bars
    if (rssi >= -80) return "▂___";  // 1 bar
    return "____";                    // 0 bars
}

/**
 * Compare function for sorting by RSSI (strongest first)
 */
static bool compare_by_rssi(const AccessPointInfo& a, const AccessPointInfo& b) {
    return a.rssi > b.rssi;
}

/**
 * Print scan results
 */
static void print_scan_results(AccessPointInfo* networks, uint8_t count) {
    if (count == 0) {
        ESP_LOGW(TAG, "No networks found!");
        return;
    }

    // Sort by signal strength (strongest first)
    std::sort(networks, networks + count, compare_by_rssi);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║                         WiFi Networks Found: %-2d                          ║", count);
    ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════════════════════════════╣");

    for (uint8_t i = 0; i < count; i++) {
        const auto& ap = networks[i];

        // Format SSID (truncate if too long)
        char ssid[33];
        strncpy(ssid, ap.ssid, 32);
        ssid[32] = '\0';

        // Format BSSID
        char bssid[18];
        sprintf(bssid, "%02X:%02X:%02X:%02X:%02X:%02X",
            ap.bssid.bytes[0], ap.bssid.bytes[1], ap.bssid.bytes[2],
            ap.bssid.bytes[3], ap.bssid.bytes[4], ap.bssid.bytes[5]);

        ESP_LOGI(TAG, "║                                                                           ║");
        ESP_LOGI(TAG, "║ [%2d] %-32s                                  ║", i + 1, ssid);
        ESP_LOGI(TAG, "║      Signal: %s %-4d dBm  %-12s                           ║",
            rssi_to_bars(ap.rssi), ap.rssi, rssi_to_quality(ap.rssi));
        ESP_LOGI(TAG, "║      Channel: %-2d    Security: %-20s            ║",
            ap.channel, auth_mode_to_string(ap.auth_mode));
        ESP_LOGI(TAG, "║      BSSID: %s                                             ║", bssid);
    }

    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
}

/**
 * Perform blocking scan
 */
static void perform_blocking_scan(void) {
    ESP_LOGI(TAG, "Starting WiFi scan (blocking mode)...");

    // Perform scan with 5 second timeout
    auto result = scanner.scan(5000);

    if (!result.is_ok()) {
        ESP_LOGE(TAG, "Scan failed!");
        return;
    }

    uint8_t count = result.value();
    ESP_LOGI(TAG, "Scan completed! Found %d networks.", count);

    if (count > 0) {
        // Allocate space for results
        AccessPointInfo* networks = new AccessPointInfo[count];

        // Get results
        auto get_result = scanner.get_results(networks, count);

        if (get_result.is_ok()) {
            uint8_t actual_count = get_result.value();
            print_scan_results(networks, actual_count);
        }

        delete[] networks;
    } else {
        ESP_LOGW(TAG, "No networks found. Make sure WiFi is enabled nearby.");
    }
}

/**
 * Scan completion callback
 */
void on_scan_complete(bool success, uint8_t count) {
    if (success) {
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "✓ Async scan completed! Found %d networks.", count);

        if (count > 0) {
            AccessPointInfo* networks = new AccessPointInfo[count];

            auto result = scanner.get_results(networks, count);
            if (result.is_ok()) {
                print_scan_results(networks, result.value());
            }

            delete[] networks;
        }
    } else {
        ESP_LOGE(TAG, "✗ Async scan failed!");
    }
}

/**
 * Perform async scan
 */
static void perform_async_scan(void) {
    ESP_LOGI(TAG, "Starting WiFi scan (async mode)...");

    // Set callback
    scanner.set_scan_callback(on_scan_complete);

    // Configure scan
    ScanConfig config;
    config.timeout_ms = 5000;
    config.show_hidden = true;  // Include hidden networks

    // Start async scan
    auto result = scanner.scan_async(config);

    if (!result.is_ok()) {
        ESP_LOGE(TAG, "Failed to start async scan!");
    }
}

/**
 * Demonstrate targeted scan (specific SSID)
 */
static void scan_for_specific_network(const char* target_ssid) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Scanning for specific network: %s", target_ssid);

    ScanConfig config;
    config.ssid = target_ssid;
    config.timeout_ms = 3000;

    auto result = scanner.scan(config);

    if (result.is_ok() && result.value() > 0) {
        AccessPointInfo network;
        auto get_result = scanner.get_results(&network, 1);

        if (get_result.is_ok()) {
            ESP_LOGI(TAG, "✓ Found target network!");
            ESP_LOGI(TAG, "  SSID:     %s", network.ssid);
            ESP_LOGI(TAG, "  Channel:  %d", network.channel);
            ESP_LOGI(TAG, "  RSSI:     %d dBm (%s)", network.rssi, rssi_to_quality(network.rssi));
            ESP_LOGI(TAG, "  Security: %s", auth_mode_to_string(network.auth_mode));
        }
    } else {
        ESP_LOGW(TAG, "✗ Network '%s' not found", target_ssid);
    }
}

/**
 * Print statistics
 */
static void print_scan_statistics(AccessPointInfo* networks, uint8_t count) {
    if (count == 0) return;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Scan Statistics ===");

    // Count by security type
    int open = 0, wep = 0, wpa = 0, wpa2 = 0, wpa3 = 0;

    for (uint8_t i = 0; i < count; i++) {
        switch (networks[i].auth_mode) {
            case AuthMode::Open:
                open++;
                break;
            case AuthMode::WEP:
                wep++;
                break;
            case AuthMode::WPA_PSK:
            case AuthMode::WPA_WPA2_PSK:
                wpa++;
                break;
            case AuthMode::WPA2_PSK:
            case AuthMode::WPA2_ENTERPRISE:
                wpa2++;
                break;
            case AuthMode::WPA3_PSK:
            case AuthMode::WPA2_WPA3_PSK:
                wpa3++;
                break;
        }
    }

    ESP_LOGI(TAG, "Total networks:    %d", count);
    ESP_LOGI(TAG, "Open networks:     %d", open);
    ESP_LOGI(TAG, "WEP:               %d", wep);
    ESP_LOGI(TAG, "WPA/Mixed:         %d", wpa);
    ESP_LOGI(TAG, "WPA2:              %d", wpa2);
    ESP_LOGI(TAG, "WPA3:              %d", wpa3);
    ESP_LOGI(TAG, "======================");
    ESP_LOGI(TAG, "");
}

/**
 * Main Application Entry Point
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Alloy WiFi Scanner Example");
    ESP_LOGI(TAG, "  Using Alloy WiFi API");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Step 1: Initialize scanner
    ESP_LOGI(TAG, "Initializing WiFi scanner...");
    auto init_result = scanner.init();
    if (!init_result.is_ok()) {
        ESP_LOGE(TAG, "Failed to initialize WiFi scanner!");
        return;
    }
    ESP_LOGI(TAG, "✓ WiFi scanner initialized");
    ESP_LOGI(TAG, "");

    // Step 2: Perform initial scan
    perform_blocking_scan();

    // Get statistics
    uint8_t count = scanner.result_count();
    if (count > 0) {
        AccessPointInfo* networks = new AccessPointInfo[count];
        auto result = scanner.get_results(networks, count);
        if (result.is_ok()) {
            print_scan_statistics(networks, result.value());
        }
        delete[] networks;
    }

    // Step 3: Demonstrate targeted scan (optional - uncomment to use)
    // scan_for_specific_network("YOUR_NETWORK_NAME");

    // Step 4: Main loop - continuous scanning
    ESP_LOGI(TAG, "Starting continuous scanning mode...");
    ESP_LOGI(TAG, "Scanning every 10 seconds. Press reset to stop.");
    ESP_LOGI(TAG, "");

    int scan_count = 1;
    bool use_async = false;  // Alternate between blocking and async

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        ESP_LOGI(TAG, "Scan #%d - %s mode", scan_count++, use_async ? "Async" : "Blocking");
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

        if (use_async) {
            perform_async_scan();
            // Wait for async scan to complete
            vTaskDelay(pdMS_TO_TICKS(6000));
        } else {
            perform_blocking_scan();
        }

        // Alternate scan mode
        use_async = !use_async;
    }
}
