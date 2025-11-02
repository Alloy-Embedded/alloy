/**
 * ESP32 WiFi Station Example
 *
 * This example demonstrates:
 * - Connecting to a WiFi access point
 * - Handling WiFi events (connected, disconnected, got IP)
 * - Making HTTP GET request after connection
 * - Automatic ESP-IDF component detection
 *
 * Note: Update WIFI_SSID and WIFI_PASSWORD below with your credentials
 */

#include <cstring>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"

// WiFi Configuration - CHANGE THESE!
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

// Maximum retry attempts
#define MAX_RETRY_COUNT 5

static const char *TAG = "wifi_station";

// Event group for WiFi status
static EventGroupHandle_t s_wifi_event_group;

// Event bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Retry counter
static int s_retry_count = 0;

/**
 * WiFi Event Handler
 *
 * Handles WiFi and IP events:
 * - WIFI_EVENT_STA_START: WiFi started, begin connection
 * - WIFI_EVENT_STA_DISCONNECTED: Connection lost, retry
 * - IP_EVENT_STA_GOT_IP: Successfully obtained IP address
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started, connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < MAX_RETRY_COUNT) {
            esp_wifi_connect();
            s_retry_count++;
            ESP_LOGI(TAG, "Retrying connection... (attempt %d/%d)", s_retry_count, MAX_RETRY_COUNT);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect after %d attempts", MAX_RETRY_COUNT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * Initialize WiFi in Station Mode
 *
 * Sets up:
 * - NVS (required by WiFi)
 * - Network interface
 * - Event loop
 * - WiFi driver
 * - Station configuration
 */
static bool wifi_init_sta(void)
{
    // Create event group
    s_wifi_event_group = xEventGroupCreate();

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &wifi_event_handler,
                                                         NULL,
                                                         &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                         IP_EVENT_STA_GOT_IP,
                                                         &wifi_event_handler,
                                                         NULL,
                                                         &instance_got_ip));

    // Configure WiFi connection
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization complete. Waiting for connection...");

    // Wait for connection or failure
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    // Check result
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Successfully connected to SSID: %s", WIFI_SSID);
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s", WIFI_SSID);
        return false;
    } else {
        ESP_LOGE(TAG, "Unexpected event");
        return false;
    }
}

/**
 * HTTP Event Handler
 * Handles HTTP client events (connected, data received, etc.)
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_HEADERS_COMPLETE:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
            break;
        case HTTP_EVENT_ON_STATUS_CODE:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_STATUS_CODE, status=%d", esp_http_client_get_status_code(evt->client));
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Print received data (limited to first 200 chars for brevity)
                int len = evt->data_len > 200 ? 200 : evt->data_len;
                printf("Received data: %.*s\n", len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

/**
 * Perform HTTP GET Request
 *
 * Makes a simple HTTP GET to example.com to demonstrate
 * that we have working internet connectivity
 */
static void http_get_task(void)
{
    ESP_LOGI(TAG, "Starting HTTP GET request...");

    // Configure HTTP client
    esp_http_client_config_t config = {};
    config.url = "http://example.com";
    config.event_handler = http_event_handler;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Perform GET request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    // Cleanup
    esp_http_client_cleanup(client);
}

/**
 * Main Application Entry Point
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  CoreZero ESP32 WiFi Station Example");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "");

    // Initialize and connect to WiFi
    if (wifi_init_sta()) {
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "WiFi connected successfully!");
        ESP_LOGI(TAG, "Now performing HTTP GET request...");
        ESP_LOGI(TAG, "");

        // Wait a bit for connection to stabilize
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Perform HTTP request
        http_get_task();

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Example complete! WiFi is connected and working.");
        ESP_LOGI(TAG, "");
    } else {
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "Failed to connect to WiFi!");
        ESP_LOGE(TAG, "Please check your SSID and password in main.cpp");
        ESP_LOGE(TAG, "");
    }

    // Main loop - just keep WiFi alive
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Still connected...");
    }
}
