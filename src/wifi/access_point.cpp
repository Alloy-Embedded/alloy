/**
 * @file access_point.cpp
 * @brief WiFi Access Point mode implementation
 */

#include "access_point.hpp"

#ifdef ESP_PLATFORM
    #include <cstring>

namespace alloy::wifi {

static AccessPoint* s_ap_instance = nullptr;

AccessPoint::AccessPoint()
    : initialized_(false),
      running_(false),
      ap_info_{},
      current_ssid_{},
      callback_(nullptr) {
    s_ap_instance = this;
}

AccessPoint::~AccessPoint() {
    if (running_) {
        stop();
    }

    if (initialized_) {
        esp_wifi_stop();
        esp_wifi_deinit();
    }

    s_ap_instance = nullptr;
}

Result<void> AccessPoint::init() {
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

    // Initialize TCP/IP stack
    ESP_TRY(esp_netif_init());

    // Create default event loop
    ESP_TRY(esp_event_loop_create_default());

    // Create default WiFi AP interface
    esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == nullptr) {
        return Result<void>::error(ErrorCode::HardwareError);
    }

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_TRY(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_TRY(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &AccessPoint::event_handler,
                                       this));

    // Set WiFi mode to AP
    ESP_TRY(esp_wifi_set_mode(WIFI_MODE_AP));

    initialized_ = true;
    return Result<void>::ok();
}

Result<ConnectionInfo> AccessPoint::start(const char* ssid, const char* password) {
    APConfig config;
    config.ssid = ssid;
    config.password = password;
    config.channel = 1;
    config.max_connections = 4;
    config.auth_mode = (password != nullptr) ? AuthMode::WPA2_PSK : AuthMode::Open;
    config.ssid_hidden = false;

    return start(config);
}

Result<ConnectionInfo> AccessPoint::start(const APConfig& config) {
    if (!initialized_) {
        return Result<ConnectionInfo>::error(ErrorCode::NotInitialized);
    }

    if (config.ssid == nullptr || strlen(config.ssid) > 32) {
        return Result<ConnectionInfo>::error(ErrorCode::InvalidParameter);
    }

    if (config.password != nullptr) {
        size_t pwd_len = strlen(config.password);
        if (pwd_len < 8 || pwd_len > 63) {
            return Result<ConnectionInfo>::error(ErrorCode::InvalidParameter);
        }
    }

    if (config.channel < 1 || config.channel > 13) {
        return Result<ConnectionInfo>::error(ErrorCode::InvalidParameter);
    }

    if (config.max_connections < 1 || config.max_connections > 10) {
        return Result<ConnectionInfo>::error(ErrorCode::InvalidParameter);
    }

    // Store SSID
    strncpy(current_ssid_, config.ssid, sizeof(current_ssid_) - 1);
    current_ssid_[sizeof(current_ssid_) - 1] = '\0';

    // Configure WiFi
    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char*>(wifi_config.ap.ssid), config.ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(config.ssid);

    if (config.password != nullptr) {
        strncpy(reinterpret_cast<char*>(wifi_config.ap.password), config.password,
                sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.ap.channel = config.channel;
    wifi_config.ap.max_connection = config.max_connections;
    wifi_config.ap.ssid_hidden = config.ssid_hidden ? 1 : 0;

    ESP_TRY_T(ConnectionInfo, esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_TRY_T(ConnectionInfo, esp_wifi_start());

    // Get AP IP information
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            ap_info_.ip = IPAddress(ip_info.ip.addr);
            ap_info_.gateway = IPAddress(ip_info.gw.addr);
            ap_info_.netmask = IPAddress(ip_info.netmask.addr);

            // Get MAC address
            uint8_t mac[6];
            esp_wifi_get_mac(WIFI_IF_AP, mac);
            ap_info_.mac = MacAddress(mac);
        }
    }

    running_ = true;
    return Result<ConnectionInfo>::ok(ap_info_);
}

Result<void> AccessPoint::stop() {
    if (!initialized_) {
        return Result<void>::error(ErrorCode::NotInitialized);
    }

    if (!running_) {
        return Result<void>::ok();
    }

    ESP_TRY(esp_wifi_stop());

    running_ = false;
    current_ssid_[0] = '\0';
    ap_info_ = ConnectionInfo{};

    return Result<void>::ok();
}

bool AccessPoint::is_running() const {
    return running_;
}

Result<ConnectionInfo> AccessPoint::ap_info() const {
    if (!running_) {
        return Result<ConnectionInfo>::error(ErrorCode::NotInitialized);
    }
    return Result<ConnectionInfo>::ok(ap_info_);
}

Result<uint8_t> AccessPoint::station_count() const {
    if (!running_) {
        return Result<uint8_t>::error(ErrorCode::NotInitialized);
    }

    wifi_sta_list_t sta_list;
    esp_err_t err = esp_wifi_ap_get_sta_list(&sta_list);

    if (err != ESP_OK) {
        return core::esp_result_error<uint8_t>(err);
    }

    return Result<uint8_t>::ok(sta_list.num);
}

Result<uint8_t> AccessPoint::get_stations(StationInfo* stations, uint8_t max_stations) const {
    if (!running_) {
        return Result<uint8_t>::error(ErrorCode::NotInitialized);
    }

    if (stations == nullptr || max_stations == 0) {
        return Result<uint8_t>::error(ErrorCode::InvalidParameter);
    }

    wifi_sta_list_t sta_list;
    esp_err_t err = esp_wifi_ap_get_sta_list(&sta_list);

    if (err != ESP_OK) {
        return core::esp_result_error<uint8_t>(err);
    }

    uint8_t count = (sta_list.num < max_stations) ? sta_list.num : max_stations;

    for (uint8_t i = 0; i < count; i++) {
        stations[i].mac = MacAddress(sta_list.sta[i].mac);
        stations[i].rssi = sta_list.sta[i].rssi;
    }

    return Result<uint8_t>::ok(count);
}

void AccessPoint::set_station_callback(StationCallback callback) {
    callback_ = callback;
}

const char* AccessPoint::ssid() const {
    return current_ssid_;
}

// Static event handler (ESP-IDF callback)
void AccessPoint::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                                void* event_data) {
    AccessPoint* self = static_cast<AccessPoint*>(arg);

    if (event_base == WIFI_EVENT) {
        self->handle_wifi_event(event_id, event_data);
    }
}

void AccessPoint::handle_wifi_event(int32_t event_id, void* event_data) {
    switch (event_id) {
        case WIFI_EVENT_AP_START:
            running_ = true;
            break;

        case WIFI_EVENT_AP_STOP:
            running_ = false;
            break;

        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t* event =
                static_cast<wifi_event_ap_staconnected_t*>(event_data);

            MacAddress mac(event->mac);

            // Trigger callback if set
            if (callback_) {
                callback_(true, mac);
            }
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t* event =
                static_cast<wifi_event_ap_stadisconnected_t*>(event_data);

            MacAddress mac(event->mac);

            // Trigger callback if set
            if (callback_) {
                callback_(false, mac);
            }
            break;
        }

        default:
            break;
    }
}

}  // namespace alloy::wifi

#else  // !ESP_PLATFORM

namespace alloy::wifi {

// Stub implementation for non-ESP platforms
AccessPoint::AccessPoint()
    : initialized_(false),
      running_(false),
      ap_info_{},
      current_ssid_{},
      callback_(nullptr) {}

AccessPoint::~AccessPoint() {}

Result<void> AccessPoint::init() {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<ConnectionInfo> AccessPoint::start(const char*, const char*) {
    return Result<ConnectionInfo>::error(ErrorCode::NotSupported);
}

Result<ConnectionInfo> AccessPoint::start(const APConfig&) {
    return Result<ConnectionInfo>::error(ErrorCode::NotSupported);
}

Result<void> AccessPoint::stop() {
    return Result<void>::error(ErrorCode::NotSupported);
}

bool AccessPoint::is_running() const {
    return false;
}

Result<ConnectionInfo> AccessPoint::ap_info() const {
    return Result<ConnectionInfo>::error(ErrorCode::NotSupported);
}

Result<uint8_t> AccessPoint::station_count() const {
    return Result<uint8_t>::error(ErrorCode::NotSupported);
}

Result<uint8_t> AccessPoint::get_stations(StationInfo*, uint8_t) const {
    return Result<uint8_t>::error(ErrorCode::NotSupported);
}

void AccessPoint::set_station_callback(StationCallback) {}

const char* AccessPoint::ssid() const {
    return "";
}

}  // namespace alloy::wifi

#endif  // ESP_PLATFORM
