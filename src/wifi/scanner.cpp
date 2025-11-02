/**
 * @file scanner.cpp
 * @brief WiFi network scanner implementation
 */

#include "scanner.hpp"

#ifdef ESP_PLATFORM
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_netif.h"

namespace alloy::wifi {

// Event bit for scan completion
static constexpr int SCAN_DONE_BIT = BIT0;

// Global event group for scan events
static EventGroupHandle_t s_scan_event_group = nullptr;
static Scanner* s_scanner_instance = nullptr;

Scanner::Scanner()
    : initialized_(false)
    , scanning_(false)
    , result_count_(0)
    , callback_(nullptr)
{
    s_scanner_instance = this;
}

Scanner::~Scanner() {
    if (s_scan_event_group) {
        vEventGroupDelete(s_scan_event_group);
        s_scan_event_group = nullptr;
    }
    s_scanner_instance = nullptr;
}

Result<void> Scanner::init() {
    if (initialized_) {
        return Result<void>::ok();
    }

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_TRY(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_TRY(ret);

    // Create event group
    s_scan_event_group = xEventGroupCreate();
    if (s_scan_event_group == nullptr) {
        return Result<void>::error(ErrorCode::HardwareError);
    }

    // Initialize TCP/IP stack (if not already initialized)
    static bool netif_initialized = false;
    if (!netif_initialized) {
        ESP_TRY(esp_netif_init());
        netif_initialized = true;
    }

    // Create default event loop (if not already created)
    static bool event_loop_created = false;
    if (!event_loop_created) {
        esp_err_t err = esp_event_loop_create_default();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            return core::esp_result_error_void(err);
        }
        event_loop_created = true;
    }

    // Initialize WiFi if not already initialized
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t wifi_init_err = esp_wifi_init(&cfg);
    if (wifi_init_err != ESP_OK && wifi_init_err != ESP_ERR_WIFI_NOT_INIT) {
        // WiFi already initialized is OK
        ESP_TRY(wifi_init_err);
    }

    // Register event handler for scan done
    ESP_TRY(esp_event_handler_register(
        WIFI_EVENT,
        WIFI_EVENT_SCAN_DONE,
        &Scanner::event_handler,
        this
    ));

    // Set WiFi mode to station (required for scanning)
    wifi_mode_t current_mode;
    esp_wifi_get_mode(&current_mode);
    if (current_mode == WIFI_MODE_NULL) {
        ESP_TRY(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_TRY(esp_wifi_start());
    }

    initialized_ = true;
    return Result<void>::ok();
}

Result<uint8_t> Scanner::scan(uint32_t timeout_ms) {
    ScanConfig config;
    config.timeout_ms = timeout_ms;
    return scan(config);
}

Result<uint8_t> Scanner::scan(const ScanConfig& config) {
    if (!initialized_) {
        return Result<uint8_t>::error(ErrorCode::NotInitialized);
    }

    if (scanning_) {
        return Result<uint8_t>::error(ErrorCode::Busy);
    }

    // Configure scan
    wifi_scan_config_t scan_config = {};
    scan_config.ssid = (uint8_t*)config.ssid;
    scan_config.bssid = (uint8_t*)config.bssid;
    scan_config.channel = config.channel;
    scan_config.show_hidden = config.show_hidden;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

    // Clear event bit
    xEventGroupClearBits(s_scan_event_group, SCAN_DONE_BIT);

    // Start scan (blocking mode)
    scanning_ = true;
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);

    if (err != ESP_OK) {
        scanning_ = false;
        return core::esp_result_error<uint8_t>(err);
    }

    // Scan completed, get count
    uint16_t count = 0;
    ESP_TRY_T(uint8_t, esp_wifi_scan_get_ap_num(&count));

    result_count_ = (count > 255) ? 255 : (uint8_t)count;
    scanning_ = false;

    return Result<uint8_t>::ok(result_count_);
}

Result<void> Scanner::scan_async(const ScanConfig& config) {
    if (!initialized_) {
        return Result<void>::error(ErrorCode::NotInitialized);
    }

    if (scanning_) {
        return Result<void>::error(ErrorCode::Busy);
    }

    // Configure scan
    wifi_scan_config_t scan_config = {};
    scan_config.ssid = (uint8_t*)config.ssid;
    scan_config.bssid = (uint8_t*)config.bssid;
    scan_config.channel = config.channel;
    scan_config.show_hidden = config.show_hidden;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

    // Clear event bit
    xEventGroupClearBits(s_scan_event_group, SCAN_DONE_BIT);

    // Start scan (non-blocking mode)
    scanning_ = true;
    ESP_TRY(esp_wifi_scan_start(&scan_config, false));

    return Result<void>::ok();
}

Result<uint8_t> Scanner::get_results(AccessPointInfo* results, uint8_t max_results) {
    if (!initialized_) {
        return Result<uint8_t>::error(ErrorCode::NotInitialized);
    }

    if (results == nullptr || max_results == 0) {
        return Result<uint8_t>::error(ErrorCode::InvalidParameter);
    }

    if (result_count_ == 0) {
        return Result<uint8_t>::ok(0);
    }

    // Get actual results
    uint16_t count = (result_count_ < max_results) ? result_count_ : max_results;
    wifi_ap_record_t* ap_records = new wifi_ap_record_t[count];

    esp_err_t err = esp_wifi_scan_get_ap_records(&count, ap_records);
    if (err != ESP_OK) {
        delete[] ap_records;
        return core::esp_result_error<uint8_t>(err);
    }

    // Convert to our format
    for (uint16_t i = 0; i < count; i++) {
        strncpy(results[i].ssid, (char*)ap_records[i].ssid, 32);
        results[i].ssid[32] = '\0';
        results[i].bssid = MacAddress(ap_records[i].bssid);
        results[i].channel = ap_records[i].primary;
        results[i].rssi = ap_records[i].rssi;

        // Map auth mode
        switch (ap_records[i].authmode) {
            case WIFI_AUTH_OPEN:
                results[i].auth_mode = AuthMode::Open;
                break;
            case WIFI_AUTH_WEP:
                results[i].auth_mode = AuthMode::WEP;
                break;
            case WIFI_AUTH_WPA_PSK:
                results[i].auth_mode = AuthMode::WPA_PSK;
                break;
            case WIFI_AUTH_WPA2_PSK:
                results[i].auth_mode = AuthMode::WPA2_PSK;
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                results[i].auth_mode = AuthMode::WPA_WPA2_PSK;
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                results[i].auth_mode = AuthMode::WPA2_ENTERPRISE;
                break;
            case WIFI_AUTH_WPA3_PSK:
                results[i].auth_mode = AuthMode::WPA3_PSK;
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                results[i].auth_mode = AuthMode::WPA2_WPA3_PSK;
                break;
            default:
                results[i].auth_mode = AuthMode::Open;
                break;
        }
    }

    delete[] ap_records;
    return Result<uint8_t>::ok((uint8_t)count);
}

uint8_t Scanner::result_count() const {
    return result_count_;
}

bool Scanner::is_scanning() const {
    return scanning_;
}

void Scanner::set_scan_callback(ScanCallback callback) {
    callback_ = callback;
}

// Static event handler (ESP-IDF callback)
void Scanner::event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data
) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        Scanner* self = static_cast<Scanner*>(arg);
        self->handle_scan_done();
    }
}

void Scanner::handle_scan_done() {
    scanning_ = false;

    // Get result count
    uint16_t count = 0;
    esp_err_t err = esp_wifi_scan_get_ap_num(&count);

    bool success = (err == ESP_OK);
    result_count_ = (count > 255) ? 255 : (uint8_t)count;

    // Set event bit
    xEventGroupSetBits(s_scan_event_group, SCAN_DONE_BIT);

    // Trigger callback if set
    if (callback_) {
        callback_(success, result_count_);
    }
}

} // namespace alloy::wifi

#else // !ESP_PLATFORM

namespace alloy::wifi {

// Stub implementation for non-ESP platforms
Scanner::Scanner()
    : initialized_(false)
    , scanning_(false)
    , result_count_(0)
    , callback_(nullptr)
{}

Scanner::~Scanner() {}

Result<void> Scanner::init() {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<uint8_t> Scanner::scan(uint32_t) {
    return Result<uint8_t>::error(ErrorCode::NotSupported);
}

Result<uint8_t> Scanner::scan(const ScanConfig&) {
    return Result<uint8_t>::error(ErrorCode::NotSupported);
}

Result<void> Scanner::scan_async(const ScanConfig&) {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<uint8_t> Scanner::get_results(AccessPointInfo*, uint8_t) {
    return Result<uint8_t>::error(ErrorCode::NotSupported);
}

uint8_t Scanner::result_count() const {
    return 0;
}

bool Scanner::is_scanning() const {
    return false;
}

void Scanner::set_scan_callback(ScanCallback) {}

} // namespace alloy::wifi

#endif // ESP_PLATFORM
