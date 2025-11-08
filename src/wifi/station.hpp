/**
 * @file station.hpp
 * @brief WiFi Station (client) mode
 *
 * Provides a clean C++ interface for ESP32 WiFi station mode.
 * Station mode connects the ESP32 to an existing WiFi access point.
 *
 * Example:
 * @code
 * #include "wifi/station.hpp"
 *
 * using alloy::wifi::Station;
 *
 * Station wifi;
 * auto result = wifi.connect("MyNetwork", "password123");
 * if (result.is_ok()) {
 *     auto info = wifi.connection_info();
 *     // Use info.ip, info.gateway, etc.
 * }
 * @endcode
 */

#pragma once

#include "core/error.hpp"
#include "core/esp_error.hpp"

#include "types.hpp"

#ifdef ESP_PLATFORM
    #include "esp_event.h"
    #include "esp_netif.h"
    #include "esp_wifi.h"
    #include "nvs_flash.h"
#endif

namespace alloy::wifi {

using core::ErrorCode;
using core::Result;

/**
 * @brief WiFi Station mode manager
 *
 * Manages WiFi connection as a station (client) to an access point.
 * Handles initialization, connection, disconnection, and status queries.
 *
 * Note: This class is a singleton-style manager. Only one instance should
 * be used per application due to ESP-IDF WiFi driver limitations.
 */
class Station {
   public:
    /**
     * @brief Connection callback function type
     * @param connected true if connected, false if disconnected
     * @param info Connection information (valid only if connected)
     */
    using ConnectionCallback = void (*)(bool connected, const ConnectionInfo& info);

    /**
     * @brief Constructor
     */
    Station();

    /**
     * @brief Destructor
     */
    ~Station();

    // Prevent copying
    Station(const Station&) = delete;
    Station& operator=(const Station&) = delete;

    /**
     * @brief Initialize WiFi station
     *
     * Must be called before connect(). Initializes NVS, network interface,
     * event loop, and WiFi driver.
     *
     * @return Result<void> - Ok if successful, error otherwise
     */
    Result<void> init();

    /**
     * @brief Connect to WiFi access point
     *
     * Connects to the specified SSID with the given password.
     * Blocks until connection succeeds or fails.
     *
     * @param ssid Network SSID (max 32 characters)
     * @param password Network password (max 63 characters)
     * @param timeout_ms Connection timeout in milliseconds (default: 10000)
     * @return Result<ConnectionInfo> - Connection info if successful
     */
    Result<ConnectionInfo> connect(const char* ssid, const char* password,
                                   uint32_t timeout_ms = 10000);

    /**
     * @brief Disconnect from WiFi
     *
     * @return Result<void> - Ok if successful
     */
    Result<void> disconnect();

    /**
     * @brief Check if currently connected
     *
     * @return true if connected to AP and has IP address
     */
    bool is_connected() const;

    /**
     * @brief Get connection state
     *
     * @return Current connection state
     */
    ConnectionState state() const;

    /**
     * @brief Get connection information
     *
     * @return Result<ConnectionInfo> - Connection info if connected
     */
    Result<ConnectionInfo> connection_info() const;

    /**
     * @brief Get signal strength (RSSI)
     *
     * @return Result<int8_t> - RSSI in dBm if connected
     */
    Result<int8_t> rssi() const;

    /**
     * @brief Set connection callback
     *
     * Called when connection state changes (connected/disconnected).
     *
     * @param callback Function to call on state change
     */
    void set_connection_callback(ConnectionCallback callback);

    /**
     * @brief Get current SSID
     *
     * @return SSID of connected network, or empty string if not connected
     */
    const char* ssid() const;

   private:
    bool initialized_;
    ConnectionState state_;
    ConnectionInfo conn_info_;
    char current_ssid_[33];
    ConnectionCallback callback_;

#ifdef ESP_PLATFORM
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                              void* event_data);

    void handle_wifi_event(int32_t event_id, void* event_data);
    void handle_ip_event(int32_t event_id, void* event_data);
#endif
};

}  // namespace alloy::wifi
