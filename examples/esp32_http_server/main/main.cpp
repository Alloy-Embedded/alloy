/**
 * ESP32 HTTP Server Example - Alloy Framework
 *
 * Demonstrates:
 * - WiFi Station connection
 * - HTTP Server with REST API endpoints
 * - GET, POST, PUT, DELETE routes
 * - JSON responses
 * - Query parameters and headers
 */

#include "wifi/station.hpp"
#include "http/server.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <cstdio>

// WiFi Configuration
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

static const char *TAG = "http_example";

using alloy::wifi::Station;
using alloy::http::Server;
using alloy::http::Request;
using alloy::http::Response;
using alloy::http::Status;

// Global instances
static Station wifi;
static Server server(80);

// ============================================================================
// API Handlers
// ============================================================================

Status handle_root(Request& req, Response& res) {
    const char* html = R"(
<!DOCTYPE html>
<html>
<head><title>Alloy HTTP Server</title></head>
<body>
    <h1>Alloy HTTP Server Example</h1>
    <p>Try these endpoints:</p>
    <ul>
        <li><a href="/api/status">GET /api/status</a> - Server status</li>
        <li><a href="/api/hello?name=World">GET /api/hello?name=World</a> - Greeting</li>
        <li>POST /api/echo - Echo request body</li>
    </ul>
</body>
</html>
)";
    res.html(html);
    return Status::OK;
}

Status handle_status(Request& req, Response& res) {
    const char* json = R"({
    "status": "running",
    "uptime": 12345,
    "free_heap": 100000
})";
    res.json(json);
    return Status::OK;
}

Status handle_hello(Request& req, Response& res) {
    char name[64] = "World";

    // Get name from query parameter
    auto result = req.query("name", name, sizeof(name));
    if (!result.is_ok() || result.value() == 0) {
        strcpy(name, "World");
    }

    char response[256];
    snprintf(response, sizeof(response),
        "{\"message\": \"Hello, %s!\", \"timestamp\": %lld}",
        name, esp_timer_get_time() / 1000);

    res.json(response);
    return Status::OK;
}

Status handle_echo(Request& req, Response& res) {
    char body[512];
    auto result = req.body(body, sizeof(body));

    if (!result.is_ok()) {
        res.status(Status::BadRequest)
           .json("{\"error\": \"Failed to read body\"}");
        return Status::BadRequest;
    }

    ESP_LOGI(TAG, "Received: %s", body);

    char response[600];
    snprintf(response, sizeof(response),
        "{\"echo\": \"%s\", \"length\": %zu}",
        body, result.value());

    res.json(response);
    return Status::OK;
}

Status handle_not_found(Request& req, Response& res) {
    char json[128];
    snprintf(json, sizeof(json),
        "{\"error\": \"Not Found\", \"uri\": \"%s\"}",
        req.uri());

    res.status(Status::NotFound).json(json);
    return Status::NotFound;
}

// ============================================================================
// Main Application
// ============================================================================

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Alloy HTTP Server Example");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Step 1: Initialize and connect WiFi
    ESP_LOGI(TAG, "Connecting to WiFi...");
    auto wifi_init = wifi.init();
    if (!wifi_init.is_ok()) {
        ESP_LOGE(TAG, "Failed to initialize WiFi");
        return;
    }

    auto connect_result = wifi.connect(WIFI_SSID, WIFI_PASSWORD, 15000);
    if (!connect_result.is_ok()) {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        return;
    }

    auto conn_info = connect_result.value();
    ESP_LOGI(TAG, "✓ WiFi connected!");
    ESP_LOGI(TAG, "  IP: %d.%d.%d.%d",
        conn_info.ip.octet[0], conn_info.ip.octet[1],
        conn_info.ip.octet[2], conn_info.ip.octet[3]);
    ESP_LOGI(TAG, "");

    // Step 2: Start HTTP Server
    ESP_LOGI(TAG, "Starting HTTP server on port 80...");
    auto server_result = server.start();
    if (!server_result.is_ok()) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    // Step 3: Register routes
    server.get("/", handle_root);
    server.get("/api/status", handle_status);
    server.get("/api/hello", handle_hello);
    server.post("/api/echo", handle_echo);

    ESP_LOGI(TAG, "✓ HTTP server started!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Server is ready!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Visit: http://%d.%d.%d.%d/",
        conn_info.ip.octet[0], conn_info.ip.octet[1],
        conn_info.ip.octet[2], conn_info.ip.octet[3]);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Main loop
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        if (!wifi.is_connected()) {
            ESP_LOGW(TAG, "WiFi disconnected, attempting reconnect...");
            wifi.connect(WIFI_SSID, WIFI_PASSWORD, 15000);
        }
    }
}
