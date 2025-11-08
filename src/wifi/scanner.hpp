/**
 * @file scanner.hpp
 * @brief WiFi network scanner
 *
 * Provides a clean C++ interface for scanning WiFi networks.
 * Can be used in both Station and AP modes to discover nearby access points.
 *
 * Example:
 * @code
 * #include "wifi/scanner.hpp"
 *
 * using alloy::wifi::Scanner;
 *
 * Scanner scanner;
 * scanner.init();
 *
 * auto result = scanner.scan();
 * if (result.is_ok()) {
 *     uint8_t count = result.value();
 *     AccessPointInfo networks[count];
 *     scanner.get_results(networks, count);
 *
 *     for (uint8_t i = 0; i < count; i++) {
 *         printf("SSID: %s, RSSI: %d\n", networks[i].ssid, networks[i].rssi);
 *     }
 * }
 * @endcode
 */

#pragma once

#include "core/error.hpp"
#include "core/esp_error.hpp"

#include "types.hpp"

#ifdef ESP_PLATFORM
    #include "esp_event.h"
    #include "esp_wifi.h"
#endif

namespace alloy::wifi {

using core::ErrorCode;
using core::Result;

/**
 * @brief WiFi scan configuration
 */
struct ScanConfig {
    const char* ssid;      ///< Target SSID (nullptr for all networks)
    const uint8_t* bssid;  ///< Target BSSID (nullptr for all)
    uint8_t channel;       ///< Target channel (0 for all channels)
    bool show_hidden;      ///< Include hidden networks
    uint32_t timeout_ms;   ///< Scan timeout in milliseconds

    ScanConfig()
        : ssid(nullptr),
          bssid(nullptr),
          channel(0),
          show_hidden(false),
          timeout_ms(5000) {}
};

/**
 * @brief WiFi network scanner
 *
 * Scans for available WiFi networks and provides information about them.
 * Can be used alongside Station or AP modes.
 *
 * Note: Scanning may temporarily interrupt WiFi connectivity in Station mode.
 */
class Scanner {
   public:
    /**
     * @brief Scan completion callback function type
     * @param success true if scan completed successfully
     * @param count Number of networks found
     */
    using ScanCallback = void (*)(bool success, uint8_t count);

    /**
     * @brief Constructor
     */
    Scanner();

    /**
     * @brief Destructor
     */
    ~Scanner();

    // Prevent copying
    Scanner(const Scanner&) = delete;
    Scanner& operator=(const Scanner&) = delete;

    /**
     * @brief Initialize scanner
     *
     * Must be called before scan(). Initializes WiFi if not already initialized.
     *
     * @return Result<void> - Ok if successful, error otherwise
     */
    Result<void> init();

    /**
     * @brief Scan for WiFi networks (blocking)
     *
     * Performs a blocking scan for all available networks.
     * Returns the number of networks found.
     *
     * @param timeout_ms Scan timeout in milliseconds (default: 5000)
     * @return Result<uint8_t> - Number of networks found
     */
    Result<uint8_t> scan(uint32_t timeout_ms = 5000);

    /**
     * @brief Scan with full configuration (blocking)
     *
     * @param config Scan configuration
     * @return Result<uint8_t> - Number of networks found
     */
    Result<uint8_t> scan(const ScanConfig& config);

    /**
     * @brief Start asynchronous scan
     *
     * Starts a non-blocking scan. Use set_scan_callback() to be notified
     * when the scan completes.
     *
     * @param config Scan configuration
     * @return Result<void> - Ok if scan started
     */
    Result<void> scan_async(const ScanConfig& config = ScanConfig());

    /**
     * @brief Get scan results
     *
     * Retrieves the results from the last completed scan.
     *
     * @param results Array to store access point info
     * @param max_results Maximum number of results to retrieve
     * @return Result<uint8_t> - Actual number of results retrieved
     */
    Result<uint8_t> get_results(AccessPointInfo* results, uint8_t max_results);

    /**
     * @brief Get number of networks found in last scan
     *
     * @return Number of networks found
     */
    uint8_t result_count() const;

    /**
     * @brief Check if scan is in progress
     *
     * @return true if scan is currently running
     */
    bool is_scanning() const;

    /**
     * @brief Set scan completion callback
     *
     * Called when an asynchronous scan completes.
     *
     * @param callback Function to call when scan completes
     */
    void set_scan_callback(ScanCallback callback);

   private:
    bool initialized_;
    bool scanning_;
    uint8_t result_count_;
    ScanCallback callback_;

#ifdef ESP_PLATFORM
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                              void* event_data);

    void handle_scan_done();
#endif
};

}  // namespace alloy::wifi
