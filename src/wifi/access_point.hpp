/**
 * @file access_point.hpp
 * @brief WiFi Access Point (AP) mode
 *
 * Provides a clean C++ interface for ESP32 WiFi access point mode.
 * AP mode allows the ESP32 to act as a WiFi access point that other devices can connect to.
 *
 * Example:
 * @code
 * #include "wifi/access_point.hpp"
 *
 * using alloy::wifi::AccessPoint;
 *
 * AccessPoint ap;
 * auto result = ap.start("MyESP32", "password123");
 * if (result.is_ok()) {
 *     auto info = ap.ap_info();
 *     // ESP32 is now acting as an access point
 * }
 * @endcode
 */

#pragma once

#include "core/error.hpp"
#include "core/esp_error.hpp"
#include "core/result.hpp"

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
 * @brief WiFi Access Point configuration
 */
struct APConfig {
    const char* ssid;         ///< AP SSID (max 32 characters)
    const char* password;     ///< AP password (min 8, max 63 characters, or nullptr for open)
    uint8_t channel;          ///< WiFi channel (1-13)
    uint8_t max_connections;  ///< Maximum number of stations (1-10)
    AuthMode auth_mode;       ///< Authentication mode
    bool ssid_hidden;         ///< Hide SSID

    APConfig()
        : ssid(nullptr),
          password(nullptr),
          channel(1),
          max_connections(4),
          auth_mode(AuthMode::WPA2_PSK),
          ssid_hidden(false) {}
};

/**
 * @brief WiFi Access Point manager
 *
 * Manages WiFi access point mode, allowing the ESP32 to act as an AP.
 * Other devices can connect to the ESP32 and communicate with it.
 *
 * Note: This class is a singleton-style manager. Only one instance should
 * be used per application due to ESP-IDF WiFi driver limitations.
 */
class AccessPoint {
   public:
    /**
     * @brief Station connection callback function type
     * @param connected true if station connected, false if disconnected
     * @param mac MAC address of the station
     */
    using StationCallback = void (*)(bool connected, const MacAddress& mac);

    /**
     * @brief Constructor
     */
    AccessPoint();

    /**
     * @brief Destructor
     */
    ~AccessPoint() = default;

    // Prevent copying
    AccessPoint(const AccessPoint&) = delete;
    AccessPoint& operator=(const AccessPoint&) = delete;

    /**
     * @brief Initialize WiFi access point
     *
     * Must be called before start(). Initializes NVS, network interface,
     * event loop, and WiFi driver.
     *
     * @return Result<void> - Ok if successful, error otherwise
     */
    Result<void> init();

    /**
     * @brief Start access point with simple configuration
     *
     * Starts the AP with default settings (channel 1, max 4 connections).
     *
     * @param ssid AP SSID (max 32 characters)
     * @param password AP password (min 8, max 63 characters, or nullptr for open network)
     * @return Result<ConnectionInfo> - AP connection info if successful
     */
    Result<ConnectionInfo> start(const char* ssid, const char* password = nullptr);

    /**
     * @brief Start access point with full configuration
     *
     * @param config Full AP configuration
     * @return Result<ConnectionInfo> - AP connection info if successful
     */
    Result<ConnectionInfo> start(const APConfig& config);

    /**
     * @brief Stop access point
     *
     * @return Result<void> - Ok if successful
     */
    Result<void> stop();

    /**
     * @brief Check if AP is running
     *
     * @return true if AP is active and running
     */
    bool is_running() const;

    /**
     * @brief Get AP connection information
     *
     * Returns the AP's IP address, gateway, and netmask.
     *
     * @return Result<ConnectionInfo> - Connection info if AP is running
     */
    Result<ConnectionInfo> ap_info() const;

    /**
     * @brief Get number of connected stations
     *
     * @return Result<uint8_t> - Number of connected stations
     */
    Result<uint8_t> station_count() const;

    /**
     * @brief Get list of connected stations
     *
     * @param stations Array to store station info
     * @param max_stations Maximum number of stations to retrieve
     * @return Result<uint8_t> - Actual number of stations retrieved
     */
    Result<uint8_t> get_stations(StationInfo* stations, uint8_t max_stations) const;

    /**
     * @brief Set station connection callback
     *
     * Called when a station connects or disconnects from the AP.
     *
     * @param callback Function to call on station state change
     */
    void set_station_callback(StationCallback callback);

    /**
     * @brief Get current SSID
     *
     * @return SSID of the access point, or empty string if not running
     */
    const char* ssid() const;

   private:
    bool initialized_;
    bool running_;
    ConnectionInfo ap_info_;
    char current_ssid_[33];
    StationCallback callback_;

#ifdef ESP_PLATFORM
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                              void* event_data);

    void handle_wifi_event(int32_t event_id, void* event_data);
#endif
};

}  // namespace alloy::wifi
