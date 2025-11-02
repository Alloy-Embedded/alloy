/**
 * ESP32 WiFi Station Example - Alloy Framework
 *
 * This example demonstrates:
 * - Using Alloy's WiFi Station abstraction
 * - Clean C++ API with Result<T> error handling
 * - Connection callbacks for event handling
 * - Making HTTP GET request after connection
 * - Automatic reconnection handling
 *
 * Note: Update WIFI_SSID and WIFI_PASSWORD below with your credentials
 */

#include "wifi/station.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"

// WiFi Configuration - CHANGE THESE!
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

static const char *TAG = "wifi_example";

using alloy::wifi::Station;
using alloy::wifi::ConnectionInfo;
using alloy::core::Result;

// Global WiFi station instance
static Station wifi;

/**
 * Connection Callback
 *
 * Called when WiFi connection state changes
 */
void on_wifi_event(bool connected, const ConnectionInfo& info) {
    if (connected) {
        ESP_LOGI(TAG, "✓ WiFi Connected!");
        ESP_LOGI(TAG, "  IP:      %d.%d.%d.%d",
            info.ip.octet[0], info.ip.octet[1],
            info.ip.octet[2], info.ip.octet[3]);
        ESP_LOGI(TAG, "  Gateway: %d.%d.%d.%d",
            info.gateway.octet[0], info.gateway.octet[1],
            info.gateway.octet[2], info.gateway.octet[3]);
        ESP_LOGI(TAG, "  Netmask: %d.%d.%d.%d",
            info.netmask.octet[0], info.netmask.octet[1],
            info.netmask.octet[2], info.netmask.octet[3]);
        ESP_LOGI(TAG, "  MAC:     %02X:%02X:%02X:%02X:%02X:%02X",
            info.mac.bytes[0], info.mac.bytes[1], info.mac.bytes[2],
            info.mac.bytes[3], info.mac.bytes[4], info.mac.bytes[5]);
    } else {
        ESP_LOGE(TAG, "✗ WiFi Disconnected!");
    }
}

/**
 * HTTP Event Handler
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Print received data (limited to first 200 chars for brevity)
                int len = evt->data_len > 200 ? 200 : evt->data_len;
                printf("\n--- HTTP Response (first %d bytes) ---\n", len);
                printf("%.*s\n", len, (char*)evt->data);
                printf("--- End of Response ---\n\n");
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP request completed");
            break;
        default:
            break;
    }
    return ESP_OK;
}

/**
 * Perform HTTP GET Request
 */
static void http_get_example(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Performing HTTP GET to example.com...");

    // Configure HTTP client
    esp_http_client_config_t config = {};
    config.url = "http://example.com";
    config.event_handler = http_event_handler;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Perform GET request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Status = %d, Content-Length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/**
 * Print WiFi Connection Info
 */
static void print_connection_info(void)
{
    auto info_result = wifi.connection_info();
    if (info_result.is_ok()) {
        const auto& info = info_result.value();

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "=== Current WiFi Status ===");
        ESP_LOGI(TAG, "SSID:    %s", wifi.ssid());
        ESP_LOGI(TAG, "IP:      %d.%d.%d.%d",
            info.ip.octet[0], info.ip.octet[1],
            info.ip.octet[2], info.ip.octet[3]);

        // Get RSSI
        auto rssi_result = wifi.rssi();
        if (rssi_result.is_ok()) {
            ESP_LOGI(TAG, "RSSI:    %d dBm", rssi_result.value());
        }
        ESP_LOGI(TAG, "==========================");
        ESP_LOGI(TAG, "");
    }
}

/**
 * Main Application Entry Point
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Alloy WiFi Station Example");
    ESP_LOGI(TAG, "  Using Alloy WiFi API");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Step 1: Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    auto init_result = wifi.init();
    if (!init_result.is_ok()) {
        ESP_LOGE(TAG, "Failed to initialize WiFi!");
        return;
    }
    ESP_LOGI(TAG, "✓ WiFi initialized");

    // Step 2: Set connection callback
    wifi.set_connection_callback(on_wifi_event);

    // Step 3: Connect to WiFi
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Connecting to WiFi...");
    ESP_LOGI(TAG, "SSID: %s", WIFI_SSID);

    auto connect_result = wifi.connect(WIFI_SSID, WIFI_PASSWORD, 15000);

    if (!connect_result.is_ok()) {
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "✗ Failed to connect to WiFi!");
        ESP_LOGE(TAG, "  Please check:");
        ESP_LOGE(TAG, "  - SSID is correct");
        ESP_LOGE(TAG, "  - Password is correct");
        ESP_LOGE(TAG, "  - WiFi is in range");
        ESP_LOGE(TAG, "");
        return;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ WiFi connected successfully!");

    // Step 4: Print connection info
    print_connection_info();

    // Step 5: Perform HTTP request
    vTaskDelay(pdMS_TO_TICKS(1000));
    http_get_example();

    // Step 6: Main loop - monitor connection
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Entering main loop...");
    ESP_LOGI(TAG, "WiFi will automatically reconnect if connection is lost.");
    ESP_LOGI(TAG, "");

    int loop_count = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        if (wifi.is_connected()) {
            loop_count++;
            ESP_LOGI(TAG, "Still connected... (%d)", loop_count);

            // Print detailed status every 30 seconds
            if (loop_count % 3 == 0) {
                print_connection_info();
            }
        } else {
            ESP_LOGW(TAG, "WiFi disconnected! Attempting to reconnect...");

            auto reconnect_result = wifi.connect(WIFI_SSID, WIFI_PASSWORD, 15000);
            if (reconnect_result.is_ok()) {
                ESP_LOGI(TAG, "✓ Reconnected to WiFi!");
                print_connection_info();
            } else {
                ESP_LOGE(TAG, "✗ Reconnection failed, will retry...");
            }
        }
    }
}
