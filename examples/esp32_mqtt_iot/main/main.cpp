/**
 * @file main.cpp
 * @brief ESP32 MQTT IoT Example
 *
 * Demonstrates:
 * - WiFi Station connection
 * - MQTT client with QoS levels
 * - Subscribe to topics with callbacks
 * - Publish periodic messages
 * - Connection state management
 * - Error handling with Result<T>
 *
 * Hardware: ESP32 DevKit, ESP32-C3, ESP32-S3, or similar
 * Broker: mqtt://broker.hivemq.com (public broker for testing)
 */

#include "wifi/station.hpp"
#include "mqtt/client.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <cstdio>

static const char* TAG = "MQTT_IOT";

// WiFi Configuration - CHANGE THESE!
constexpr const char* WIFI_SSID = "YOUR_WIFI_SSID";
constexpr const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// MQTT Configuration
constexpr const char* MQTT_BROKER = "mqtt://broker.hivemq.com";  // Public HiveMQ broker
constexpr const char* MQTT_CLIENT_ID = "esp32_corezero_client";

// MQTT Topics
constexpr const char* TOPIC_STATUS = "corezero/esp32/status";
constexpr const char* TOPIC_SENSOR = "corezero/esp32/sensor";
constexpr const char* TOPIC_CMD = "corezero/esp32/cmd";
constexpr const char* TOPIC_BROADCAST = "corezero/broadcast/#";

// Global state
static bool wifi_connected = false;
static bool mqtt_connected = false;

/**
 * @brief WiFi connection callback
 */
void on_wifi_event(bool connected, const WiFi::ConnectionInfo& info) {
    wifi_connected = connected;

    if (connected) {
        ESP_LOGI(TAG, "WiFi Connected!");
        ESP_LOGI(TAG, "  IP Address: " IPSTR, IP2STR(&info.ip));
        ESP_LOGI(TAG, "  Gateway: " IPSTR, IP2STR(&info.gateway));
    } else {
        ESP_LOGI(TAG, "WiFi Disconnected");
    }
}

/**
 * @brief MQTT connection callback
 */
void on_mqtt_connection(bool connected, MQTT::ErrorReason reason) {
    mqtt_connected = connected;

    if (connected) {
        ESP_LOGI(TAG, "MQTT Connected to broker");
    } else {
        ESP_LOGE(TAG, "MQTT Disconnected - reason: %d", static_cast<int>(reason));
    }
}

/**
 * @brief MQTT message callback for command topic
 */
void on_command_received(const MQTT::Message& msg) {
    ESP_LOGI(TAG, "Command received: %.*s", msg.length, msg.data);

    // Parse simple commands
    if (msg.payload() == "ping") {
        ESP_LOGI(TAG, "Received PING command - responding with PONG");
        // Response will be sent in main loop
    } else if (msg.payload() == "status") {
        ESP_LOGI(TAG, "Received STATUS command - will send status update");
    } else {
        ESP_LOGW(TAG, "Unknown command: %.*s", msg.length, msg.data);
    }
}

/**
 * @brief MQTT message callback for broadcast topic
 */
void on_broadcast_received(const MQTT::Message& msg) {
    ESP_LOGI(TAG, "Broadcast [%.*s]: %.*s",
             msg.topic.length(), msg.topic.data(),
             msg.length, msg.data);
}

/**
 * @brief Subscribe to MQTT topics
 */
bool mqtt_subscribe(MQTT::Client& client) {
    // Subscribe to command topic with specific callback
    auto result = client.subscribe(TOPIC_CMD, MQTT::QoS::AtLeastOnce, on_command_received);
    if (result.is_error()) {
        ESP_LOGE(TAG, "Failed to subscribe to command topic");
        return false;
    }
    ESP_LOGI(TAG, "Subscribed to: %s", TOPIC_CMD);

    // Subscribe to broadcast topic with wildcard
    result = client.subscribe(TOPIC_BROADCAST, MQTT::QoS::AtMostOnce, on_broadcast_received);
    if (result.is_error()) {
        ESP_LOGE(TAG, "Failed to subscribe to broadcast topic");
        return false;
    }
    ESP_LOGI(TAG, "Subscribed to: %s", TOPIC_BROADCAST);

    return true;
}

/**
 * @brief Publish sensor data to MQTT
 */
void publish_sensor_data(MQTT::Client& client, int counter) {
    // Simulate sensor reading
    float temperature = 25.0f + (counter % 10) * 0.5f;
    float humidity = 60.0f + (counter % 15) * 2.0f;

    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer),
                      "{\"temperature\":%.1f,\"humidity\":%.1f,\"counter\":%d}",
                      temperature, humidity, counter);

    auto result = client.publish(TOPIC_SENSOR, buffer, len, MQTT::QoS::AtLeastOnce);
    if (result.is_ok()) {
        ESP_LOGI(TAG, "Published sensor data: %s", buffer);
    } else {
        ESP_LOGE(TAG, "Failed to publish sensor data");
    }
}

/**
 * @brief Main application task
 */
extern "C" void app_main() {
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "=== ESP32 MQTT IoT Example ===");
    ESP_LOGI(TAG, "Connecting to WiFi...");

    // Initialize WiFi Station
    WiFi::Station wifi;
    wifi.set_connection_callback(on_wifi_event);

    // Connect to WiFi
    auto wifi_result = wifi.connect(WIFI_SSID, WIFI_PASSWORD);
    if (wifi_result.is_error()) {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        return;
    }

    // Wait for WiFi connection
    int retry_count = 0;
    while (!wifi_connected && retry_count < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }

    if (!wifi_connected) {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return;
    }

    ESP_LOGI(TAG, "Initializing MQTT client...");

    // Configure MQTT client
    MQTT::Config mqtt_config{
        .broker_uri = MQTT_BROKER,
        .client_id = MQTT_CLIENT_ID,
        .keepalive = 120,
        .clean_session = true,
        // Last Will and Testament - notify others if we disconnect unexpectedly
        .lwt_topic = TOPIC_STATUS,
        .lwt_message = "offline",
        .lwt_qos = MQTT::QoS::AtLeastOnce,
        .lwt_retain = true
    };

    // Create MQTT client
    MQTT::Client mqtt(mqtt_config);
    mqtt.set_connection_callback(on_mqtt_connection);

    // Connect to MQTT broker
    auto mqtt_result = mqtt.connect();
    if (mqtt_result.is_error()) {
        ESP_LOGE(TAG, "Failed to connect to MQTT broker");
        return;
    }

    // Wait for MQTT connection
    retry_count = 0;
    while (!mqtt_connected && retry_count < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }

    if (!mqtt_connected) {
        ESP_LOGE(TAG, "MQTT connection timeout");
        return;
    }

    // Publish online status
    mqtt.publish(TOPIC_STATUS, "online", MQTT::QoS::AtLeastOnce, true);

    // Subscribe to topics
    if (!mqtt_subscribe(mqtt)) {
        ESP_LOGE(TAG, "Failed to subscribe to topics");
        return;
    }

    ESP_LOGI(TAG, "Setup complete. Starting main loop...");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Try these commands:");
    ESP_LOGI(TAG, "  mosquitto_pub -h broker.hivemq.com -t '%s' -m 'ping'", TOPIC_CMD);
    ESP_LOGI(TAG, "  mosquitto_pub -h broker.hivemq.com -t '%s' -m 'status'", TOPIC_CMD);
    ESP_LOGI(TAG, "  mosquitto_sub -h broker.hivemq.com -t '%s'", TOPIC_SENSOR);
    ESP_LOGI(TAG, "");

    // Main loop
    int message_counter = 0;
    while (true) {
        if (mqtt_connected) {
            // Publish sensor data every 10 seconds
            publish_sensor_data(mqtt, message_counter++);
        } else {
            ESP_LOGW(TAG, "MQTT disconnected, waiting for reconnection...");
        }

        // Wait before next iteration
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
