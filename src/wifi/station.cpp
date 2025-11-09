/**
 * @file station.cpp
 * @brief WiFi Station mode implementation
 */

#include "station.hpp"

#ifdef ESP_PLATFORM
    #include <cstring>

    #include "freertos/FreeRTOS.h"
    #include "freertos/event_groups.h"

namespace alloy::wifi {

// Event bits for WiFi connection
static constexpr int WIFI_CONNECTED_BIT = BIT0;
static constexpr int WIFI_FAIL_BIT = BIT1;

// Global event group for WiFi events (ESP-IDF limitation: single station instance)
static EventGroupHandle_t s_wifi_event_group = nullptr;
static Station* s_instance = nullptr;

Station::Station()
    : initialized_(false),
      state_(ConnectionState::Disconnected),
      conn_info_{},
      current_ssid_{},
      callback_(nullptr) {
    s_instance = this;
}

Station::~Station() {
    if (initialized_) {
        esp_wifi_stop();
        esp_wifi_deinit();

        if (s_wifi_event_group) {
            vEventGroupDelete(s_wifi_event_group);
            s_wifi_event_group = nullptr;
        }
    }
    s_instance = nullptr;
}

Result<void> Station::init() {
    if (initialized_) {
        return Ok();
    }

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_TRY(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_TRY(ret);

    // Create event group
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == nullptr) {
        return Err(ErrorCode::HardwareError);
    }

    // Initialize TCP/IP stack
    ESP_TRY(esp_netif_init());

    // Create default event loop
    ESP_TRY(esp_event_loop_create_default());

    // Create default WiFi station interface
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == nullptr) {
        return Err(ErrorCode::HardwareError);
    }

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_TRY(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_TRY(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Station::event_handler, this));

    ESP_TRY(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Station::event_handler, this));

    // Set WiFi mode to station
    ESP_TRY(esp_wifi_set_mode(WIFI_MODE_STA));

    initialized_ = true;
    return Ok();
}

Result<ConnectionInfo> Station::connect(const char* ssid, const char* password,
                                        uint32_t timeout_ms) {
    if (!initialized_) {
        return Err(ErrorCode::NotInitialized);
    }

    if (ssid == nullptr || strlen(ssid) > 32) {
        return Err(ErrorCode::InvalidParameter);
    }

    if (password != nullptr && strlen(password) > 63) {
        return Err(ErrorCode::InvalidParameter);
    }

    // Store SSID
    strncpy(current_ssid_, ssid, sizeof(current_ssid_) - 1);
    current_ssid_[sizeof(current_ssid_) - 1] = '\0';

    // Configure WiFi
    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid, sizeof(wifi_config.sta.ssid));

    if (password != nullptr) {
        strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password,
                sizeof(wifi_config.sta.password));
    }

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_TRY_T(ConnectionInfo, esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_TRY_T(ConnectionInfo, esp_wifi_start());

    // Clear event bits
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // Start connection
    state_ = ConnectionState::Connecting;
    ESP_TRY_T(ConnectionInfo, esp_wifi_connect());

    // Wait for connection result
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE,   // Clear bits on exit
                                           pdFALSE,  // Wait for any bit
                                           pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT) {
        state_ = ConnectionState::GotIP;

        // Trigger callback if set
        if (callback_) {
            callback_(true, conn_info_);
        }

        return Ok(conn_info_);
    } else if (bits & WIFI_FAIL_BIT) {
        state_ = ConnectionState::Failed;
        return Err(ErrorCode::CommunicationError);
    } else {
        // Timeout
        state_ = ConnectionState::Failed;
        esp_wifi_disconnect();
        return Err(ErrorCode::Timeout);
    }
}

Result<void> Station::disconnect() {
    if (!initialized_) {
        return Err(ErrorCode::NotInitialized);
    }

    ESP_TRY(esp_wifi_disconnect());

    state_ = ConnectionState::Disconnected;
    current_ssid_[0] = '\0';
    conn_info_ = ConnectionInfo{};

    // Trigger callback if set
    if (callback_) {
        callback_(false, conn_info_);
    }

    return Ok();
}

bool Station::is_connected() const {
    return state_ == ConnectionState::GotIP;
}

ConnectionState Station::state() const {
    return state_;
}

Result<ConnectionInfo> Station::connection_info() const {
    if (!is_connected()) {
        return Err(ErrorCode::NotInitialized);
    }
    return Ok(conn_info_);
}

Result<int8_t> Station::rssi() const {
    if (!is_connected()) {
        return Err(ErrorCode::NotInitialized);
    }

    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);

    if (err != ESP_OK) {
        return core::esp_result_error<int8_t>(err);
    }

    return Ok(ap_info.rssi);
}

void Station::set_connection_callback(ConnectionCallback callback) {
    callback_ = callback;
}

const char* Station::ssid() const {
    return current_ssid_;
}

// Static event handler (ESP-IDF callback)
void Station::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                            void* event_data) {
    Station* self = static_cast<Station*>(arg);

    if (event_base == WIFI_EVENT) {
        self->handle_wifi_event(event_id, event_data);
    } else if (event_base == IP_EVENT) {
        self->handle_ip_event(event_id, event_data);
    }
}

void Station::handle_wifi_event(int32_t event_id, void* event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            state_ = ConnectionState::Disconnected;
            break;

        case WIFI_EVENT_STA_CONNECTED: {
            state_ = ConnectionState::Connected;

            // Get AP info for BSSID
            (void)event_data;  // Reserved for future use (BSSID extraction)

            // Note: We don't have IP yet, wait for IP_EVENT_STA_GOT_IP
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: {
            state_ = ConnectionState::Disconnected;
            current_ssid_[0] = '\0';
            conn_info_ = ConnectionInfo{};

            // Signal failure
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

            // Trigger callback if set
            if (callback_) {
                callback_(false, conn_info_);
            }
            break;
        }

        default:
            break;
    }
}

void Station::handle_ip_event(int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);

        // Store IP information
        conn_info_.ip = IPAddress(event->ip_info.ip.addr);
        conn_info_.gateway = IPAddress(event->ip_info.gw.addr);
        conn_info_.netmask = IPAddress(event->ip_info.netmask.addr);

        // Get MAC address
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        conn_info_.mac = MacAddress(mac);

        state_ = ConnectionState::GotIP;

        // Signal success
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

}  // namespace alloy::wifi

#else  // !ESP_PLATFORM

namespace alloy::wifi {

// Stub implementation for non-ESP platforms
Station::Station()
    : initialized_(false),
      state_(ConnectionState::Disconnected),
      conn_info_{},
      current_ssid_{},
      callback_(nullptr) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Result<void> Station::init() {
    return Err(ErrorCode::NotSupported);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Result<ConnectionInfo> Station::connect([[maybe_unused]] const char* ssid,
                                        [[maybe_unused]] const char* password,
                                        [[maybe_unused]] uint32_t timeout_ms) {
    return Err(ErrorCode::NotSupported);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Result<void> Station::disconnect() {
    return Err(ErrorCode::NotSupported);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool Station::is_connected() const {
    return false;
}

ConnectionState Station::state() const {
    return state_;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Result<ConnectionInfo> Station::connection_info() const {
    return Err(ErrorCode::NotSupported);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Result<int8_t> Station::rssi() const {
    return Err(ErrorCode::NotSupported);
}

void Station::set_connection_callback([[maybe_unused]] ConnectionCallback callback) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
const char* Station::ssid() const {
    return "";
}

}  // namespace alloy::wifi

#endif  // ESP_PLATFORM
