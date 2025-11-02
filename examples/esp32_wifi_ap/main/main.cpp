/**
 * ESP32 WiFi Access Point Example - Alloy Framework
 *
 * This example demonstrates:
 * - Using Alloy's WiFi Access Point abstraction
 * - Creating a WiFi network that others can connect to
 * - Monitoring connected stations
 * - Station connection/disconnection callbacks
 * - Clean C++ API with Result<T> error handling
 *
 * After running this example:
 * 1. Look for WiFi network "Alloy_AP" (password: "alloy12345")
 * 2. Connect from your phone/laptop
 * 3. Monitor the serial output to see connection events
 *
 * Note: The ESP32's default AP IP is 192.168.4.1
 */

#include "wifi/access_point.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Access Point Configuration
#define AP_SSID         "Alloy_AP"
#define AP_PASSWORD     "alloy12345"
#define AP_CHANNEL      1
#define AP_MAX_CONN     4

static const char *TAG = "wifi_ap";

using alloy::wifi::AccessPoint;
using alloy::wifi::APConfig;
using alloy::wifi::ConnectionInfo;
using alloy::wifi::MacAddress;
using alloy::wifi::StationInfo;
using alloy::core::Result;

// Global AP instance
static AccessPoint ap;

/**
 * Format MAC address as string
 */
static void format_mac(const MacAddress& mac, char* buffer) {
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac.bytes[0], mac.bytes[1], mac.bytes[2],
        mac.bytes[3], mac.bytes[4], mac.bytes[5]);
}

/**
 * Station Connection Callback
 *
 * Called when a station connects or disconnects from the AP
 */
void on_station_event(bool connected, const MacAddress& mac) {
    char mac_str[18];
    format_mac(mac, mac_str);

    if (connected) {
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "✓ Station Connected!");
        ESP_LOGI(TAG, "  MAC: %s", mac_str);

        // Get total station count
        auto count_result = ap.station_count();
        if (count_result.is_ok()) {
            ESP_LOGI(TAG, "  Total stations: %d", count_result.value());
        }
        ESP_LOGI(TAG, "");
    } else {
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "✗ Station Disconnected!");
        ESP_LOGI(TAG, "  MAC: %s", mac_str);

        // Get remaining station count
        auto count_result = ap.station_count();
        if (count_result.is_ok()) {
            ESP_LOGI(TAG, "  Remaining stations: %d", count_result.value());
        }
        ESP_LOGI(TAG, "");
    }
}

/**
 * Print AP Information
 */
static void print_ap_info(void)
{
    auto info_result = ap.ap_info();
    if (info_result.is_ok()) {
        const auto& info = info_result.value();

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "=== Access Point Information ===");
        ESP_LOGI(TAG, "SSID:     %s", ap.ssid());
        ESP_LOGI(TAG, "Password: %s", AP_PASSWORD);
        ESP_LOGI(TAG, "Channel:  %d", AP_CHANNEL);
        ESP_LOGI(TAG, "IP:       %d.%d.%d.%d",
            info.ip.octet[0], info.ip.octet[1],
            info.ip.octet[2], info.ip.octet[3]);
        ESP_LOGI(TAG, "Gateway:  %d.%d.%d.%d",
            info.gateway.octet[0], info.gateway.octet[1],
            info.gateway.octet[2], info.gateway.octet[3]);
        ESP_LOGI(TAG, "MAC:      %02X:%02X:%02X:%02X:%02X:%02X",
            info.mac.bytes[0], info.mac.bytes[1], info.mac.bytes[2],
            info.mac.bytes[3], info.mac.bytes[4], info.mac.bytes[5]);
        ESP_LOGI(TAG, "================================");
        ESP_LOGI(TAG, "");
    }
}

/**
 * Print Connected Stations
 */
static void print_connected_stations(void)
{
    auto count_result = ap.station_count();
    if (!count_result.is_ok()) {
        ESP_LOGE(TAG, "Failed to get station count");
        return;
    }

    uint8_t count = count_result.value();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Connected Stations: %d ===", count);

    if (count == 0) {
        ESP_LOGI(TAG, "(No stations connected)");
    } else {
        StationInfo stations[10];
        auto result = ap.get_stations(stations, 10);

        if (result.is_ok()) {
            uint8_t actual_count = result.value();

            for (uint8_t i = 0; i < actual_count; i++) {
                char mac_str[18];
                format_mac(stations[i].mac, mac_str);

                ESP_LOGI(TAG, "Station %d:", i + 1);
                ESP_LOGI(TAG, "  MAC:  %s", mac_str);
                ESP_LOGI(TAG, "  RSSI: %d dBm", stations[i].rssi);
            }
        }
    }

    ESP_LOGI(TAG, "=============================");
    ESP_LOGI(TAG, "");
}

/**
 * Main Application Entry Point
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Alloy WiFi Access Point Example");
    ESP_LOGI(TAG, "  Using Alloy WiFi API");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Step 1: Initialize WiFi AP
    ESP_LOGI(TAG, "Initializing WiFi Access Point...");
    auto init_result = ap.init();
    if (!init_result.is_ok()) {
        ESP_LOGE(TAG, "Failed to initialize WiFi AP!");
        return;
    }
    ESP_LOGI(TAG, "✓ WiFi AP initialized");

    // Step 2: Set station callback
    ap.set_station_callback(on_station_event);

    // Step 3: Configure and start AP
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Starting Access Point...");
    ESP_LOGI(TAG, "SSID:     %s", AP_SSID);
    ESP_LOGI(TAG, "Password: %s", AP_PASSWORD);
    ESP_LOGI(TAG, "Channel:  %d", AP_CHANNEL);
    ESP_LOGI(TAG, "Max Conn: %d", AP_MAX_CONN);

    // Create custom configuration
    APConfig config;
    config.ssid = AP_SSID;
    config.password = AP_PASSWORD;
    config.channel = AP_CHANNEL;
    config.max_connections = AP_MAX_CONN;
    config.ssid_hidden = false;

    auto start_result = ap.start(config);

    if (!start_result.is_ok()) {
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "✗ Failed to start Access Point!");
        return;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Access Point started successfully!");

    // Step 4: Print AP info
    print_ap_info();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Access Point is ready!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Connect from your device:");
    ESP_LOGI(TAG, "  1. Look for WiFi network: %s", AP_SSID);
    ESP_LOGI(TAG, "  2. Enter password: %s", AP_PASSWORD);
    ESP_LOGI(TAG, "  3. Your device will get an IP like 192.168.4.2");
    ESP_LOGI(TAG, "  4. You can ping this ESP32 at 192.168.4.1");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Step 5: Main loop - monitor connections
    ESP_LOGI(TAG, "Monitoring connections...");
    ESP_LOGI(TAG, "");

    int loop_count = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(15000));

        loop_count++;
        ESP_LOGI(TAG, "--- Status Update (%d) ---", loop_count);

        if (ap.is_running()) {
            print_connected_stations();
        } else {
            ESP_LOGW(TAG, "Warning: AP stopped unexpectedly!");
            break;
        }
    }

    ESP_LOGI(TAG, "Access Point example ended.");
}
