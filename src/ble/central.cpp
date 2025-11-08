#include "central.hpp"

using alloy::core::i8;
using alloy::core::u16;
using alloy::core::u32;
using alloy::core::u8;

#ifdef ESP_PLATFORM
    #include <cstring>

    #include "esp_bt.h"
    #include "esp_bt_main.h"
    #include "esp_gap_ble_api.h"
    #include "esp_gattc_api.h"
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/event_groups.h"
    #include "nvs_flash.h"

static const char* TAG = "BLE_Central";

    #define SCAN_DONE_BIT BIT0

#endif  // ESP_PLATFORM

namespace alloy::ble {

#ifdef ESP_PLATFORM

// ============================================================================
// Implementation (ESP32)
// ============================================================================

// Central implementation structure
struct CentralImplData {
    bool initialized;
    bool scanning;
    ScanCallback scan_callback;
    ConnectionCallback conn_callback;
    ReadCallback read_callback;
    WriteCallback notify_callback;
    EventGroupHandle_t event_group;

    // Scan results storage
    DeviceInfo scan_results[32];
    u8 scan_count;

    CentralImplData()
        : initialized(false),
          scanning(false),
          scan_callback(nullptr),
          conn_callback(nullptr),
          read_callback(nullptr),
          notify_callback(nullptr),
          event_group(nullptr),
          scan_count(0) {
        event_group = xEventGroupCreate();
    }

    ~CentralImplData() {
        if (event_group) {
            vEventGroupDelete(event_group);
        }
    }
};

// Define Central::Impl
struct Central::Impl {
    CentralImplData data;
    Impl() = default;
};

// Static pointer for callbacks
static CentralImplData* g_impl = nullptr;

// GAP event handler
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    if (!g_impl)
        return;

    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Scan parameters set");
            break;

        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Scan started");
                g_impl->scanning = true;
            } else {
                ESP_LOGE(TAG, "Scan start failed: %d", param->scan_start_cmpl.status);
            }
            break;

        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t* scan_result = param;

            switch (scan_result->scan_rst.search_evt) {
                case ESP_GAP_SEARCH_INQ_RES_EVT: {
                    // New device found
                    if (g_impl->scan_count < 32) {
                        DeviceInfo& dev = g_impl->scan_results[g_impl->scan_count];

                        // Copy address
                        memcpy(dev.address.addr, scan_result->scan_rst.bda, 6);
                        dev.addr_type =
                            static_cast<AddressType>(scan_result->scan_rst.ble_addr_type);
                        dev.rssi = scan_result->scan_rst.rssi;
                        dev.adv_type = AdvType::ConnectableUndirected;  // Simplified

                        // Extract device name from adv data
                        u8* adv_name = NULL;
                        u8 adv_name_len = 0;
                        adv_name =
                            esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                     ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

                        if (adv_name && adv_name_len > 0 && adv_name_len < 32) {
                            memcpy(dev.name, adv_name, adv_name_len);
                            dev.name[adv_name_len] = '\0';
                        } else {
                            dev.name[0] = '\0';
                        }

                        // Copy raw adv data
                        dev.adv_data_len = scan_result->scan_rst.adv_data_len;
                        if (dev.adv_data_len > 31)
                            dev.adv_data_len = 31;
                        memcpy(dev.adv_data, scan_result->scan_rst.ble_adv, dev.adv_data_len);

                        g_impl->scan_count++;

                        // Call user callback
                        if (g_impl->scan_callback) {
                            g_impl->scan_callback(dev);
                        }
                    }
                    break;
                }

                case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                    ESP_LOGI(TAG, "Scan complete, found %d devices", g_impl->scan_count);
                    g_impl->scanning = false;
                    xEventGroupSetBits(g_impl->event_group, SCAN_DONE_BIT);
                    break;

                default:
                    break;
            }
            break;
        }

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if (param->scan_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Scan stopped");
                g_impl->scanning = false;
                xEventGroupSetBits(g_impl->event_group, SCAN_DONE_BIT);
            }
            break;

        default:
            break;
    }
}

// GATTC event handler (stub for now)
static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                esp_ble_gattc_cb_param_t* param) {
    (void)event;
    (void)gattc_if;
    (void)param;
    // TODO: Implement GATT client operations
}

#else

// ============================================================================
// Stub Implementation (Non-ESP32)
// ============================================================================

struct CentralImplData {
    bool initialized;
    bool scanning;
    ScanCallback scan_callback;
    ConnectionCallback conn_callback;
    ReadCallback read_callback;
    WriteCallback notify_callback;
    DeviceInfo scan_results[32];
    u8 scan_count;

    Impl()
        : initialized(false),
          scanning(false),
          scan_callback(nullptr),
          conn_callback(nullptr),
          read_callback(nullptr),
          notify_callback(nullptr),
          scan_count(0) {}
};

// Define Central::Impl for stub
struct Central::Impl {
    CentralImplData data;
    Impl() = default;
};

#endif  // ESP_PLATFORM

// ============================================================================
// Central Implementation
// ============================================================================

Central::Central() : impl_(new Impl()) {}

Central::~Central() {
    if (impl_) {
        deinit();
        delete impl_;
    }
}

Result<void> Central::init() {
#ifdef ESP_PLATFORM
    if (impl_->data.initialized) {
        return Result<void>::ok();
    }

    // Initialize NVS (required for BLE)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return Result<void>::error(ErrorCode::HardwareError);
    }

    // Release classic BT memory
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Initialize BT controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return Result<void>::error(ErrorCode::HardwareError);
    }

    // Enable BLE mode
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return Result<void>::error(ErrorCode::HardwareError);
    }

    // Initialize Bluedroid
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return Result<void>::error(ErrorCode::HardwareError);
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return Result<void>::error(ErrorCode::HardwareError);
    }

    // Register GAP callback
    g_impl = &impl_->data;
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return Result<void>::error(ErrorCode::HardwareError);
    }

    // Register GATTC callback
    ret = esp_ble_gattc_register_callback(gattc_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GATTC callback register failed: %s", esp_err_to_name(ret));
        return Result<void>::error(ErrorCode::HardwareError);
    }

    impl_->data.initialized = true;
    ESP_LOGI(TAG, "BLE Central initialized");
    return Result<void>::ok();
#else
    impl_->data.initialized = true;
    return Result<void>::ok();
#endif
}

Result<void> Central::deinit() {
#ifdef ESP_PLATFORM
    if (!impl_->data.initialized) {
        return Result<void>::ok();
    }

    stop_scan();

    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    g_impl = nullptr;
    impl_->data.initialized = false;
    ESP_LOGI(TAG, "BLE Central deinitialized");
#endif
    return Result<void>::ok();
}

Result<u8> Central::scan(u16 duration_ms) {
    ScanConfig config;
    config.duration = duration_ms;
    return scan(config);
}

Result<u8> Central::scan(const ScanConfig& config) {
#ifdef ESP_PLATFORM
    if (!impl_->data.initialized) {
        return Result<u8>::error(ErrorCode::NotInitialized);
    }

    if (impl_->data.scanning) {
        return Result<u8>::error(ErrorCode::Busy);
    }

    // Clear previous results
    impl_->data.scan_count = 0;
    xEventGroupClearBits(impl_->data.event_group, SCAN_DONE_BIT);

    // Configure scan parameters
    esp_ble_scan_params_t scan_params = {
        .scan_type =
            (config.type == ScanType::Active) ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = static_cast<esp_ble_scan_filter_t>(config.filter),
        .scan_interval = config.interval,
        .scan_window = config.window,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

    esp_err_t ret = esp_ble_gap_set_scan_params(&scan_params);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Set scan params failed: %s", esp_err_to_name(ret));
        return Result<u8>::error(ErrorCode::InvalidParameter);
    }

    // Start scan
    ret = esp_ble_gap_start_scanning(config.duration / 1000);  // Convert to seconds
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Start scanning failed: %s", esp_err_to_name(ret));
        return Result<u8>::error(ErrorCode::CommunicationError);
    }

    // Wait for scan to complete
    xEventGroupWaitBits(impl_->data.event_group, SCAN_DONE_BIT,
                        pdTRUE,   // Clear on exit
                        pdFALSE,  // Wait for any bit
                        pdMS_TO_TICKS(config.duration + 1000));

    return Result<u8>::ok(impl_->data.scan_count);
#else
    (void)config;
    return Result<u8>::error(ErrorCode::NotSupported);
#endif
}

Result<void> Central::scan_async(const ScanConfig& config) {
#ifdef ESP_PLATFORM
    if (!impl_->data.initialized) {
        return Result<void>::error(ErrorCode::NotInitialized);
    }

    if (impl_->data.scanning) {
        return Result<void>::error(ErrorCode::Busy);
    }

    // Clear previous results
    impl_->data.scan_count = 0;

    // Configure and start scan (same as synchronous)
    esp_ble_scan_params_t scan_params = {
        .scan_type =
            (config.type == ScanType::Active) ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = static_cast<esp_ble_scan_filter_t>(config.filter),
        .scan_interval = config.interval,
        .scan_window = config.window,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

    esp_err_t ret = esp_ble_gap_set_scan_params(&scan_params);
    if (ret != ESP_OK) {
        return Result<void>::error(ErrorCode::InvalidParameter);
    }

    ret = esp_ble_gap_start_scanning(config.duration / 1000);
    if (ret != ESP_OK) {
        return Result<void>::error(ErrorCode::CommunicationError);
    }

    return Result<void>::ok();
#else
    (void)config;
    return Result<void>::error(ErrorCode::NotSupported);
#endif
}

Result<void> Central::stop_scan() {
#ifdef ESP_PLATFORM
    if (impl_->data.scanning) {
        esp_ble_gap_stop_scanning();
    }
#endif
    return Result<void>::ok();
}

bool Central::is_scanning() const {
    return impl_->data.scanning;
}

Result<u8> Central::get_scan_results(DeviceInfo* devices, u8 max_devices) const {
    if (!devices || max_devices == 0) {
        return Result<u8>::error(ErrorCode::InvalidParameter);
    }

    u8 count = (impl_->data.scan_count < max_devices) ? impl_->data.scan_count : max_devices;
    memcpy(devices, impl_->data.scan_results, count * sizeof(DeviceInfo));

    return Result<u8>::ok(count);
}

void Central::clear_scan_results() {
    impl_->data.scan_count = 0;
}

// Connection management stubs
Result<ConnHandle> Central::connect(const Address& address, u16 timeout_ms) {
    (void)address;
    (void)timeout_ms;
    return Result<ConnHandle>::error(ErrorCode::NotSupported);
}

Result<ConnHandle> Central::connect(const Address& address, const ConnParams& params,
                                    u16 timeout_ms) {
    (void)address;
    (void)params;
    (void)timeout_ms;
    return Result<ConnHandle>::error(ErrorCode::NotSupported);
}

Result<void> Central::disconnect(ConnHandle conn_handle) {
    (void)conn_handle;
    return Result<void>::error(ErrorCode::NotSupported);
}

bool Central::is_connected(ConnHandle conn_handle) const {
    (void)conn_handle;
    return false;
}

ConnState Central::get_conn_state(ConnHandle conn_handle) const {
    (void)conn_handle;
    return ConnState::Disconnected;
}

// Service/characteristic discovery stubs
Result<u8> Central::discover_services(ConnHandle conn_handle) {
    (void)conn_handle;
    return Result<u8>::error(ErrorCode::NotSupported);
}

Result<ServiceHandle> Central::discover_service(ConnHandle conn_handle, const UUID& service_uuid) {
    (void)conn_handle;
    (void)service_uuid;
    return Result<ServiceHandle>::error(ErrorCode::NotSupported);
}

Result<u8> Central::get_services(ConnHandle conn_handle, ServiceHandle* services,
                                 u8 max_services) const {
    (void)conn_handle;
    (void)services;
    (void)max_services;
    return Result<u8>::error(ErrorCode::NotSupported);
}

Result<u8> Central::discover_characteristics(ConnHandle conn_handle, const ServiceHandle& service) {
    (void)conn_handle;
    (void)service;
    return Result<u8>::error(ErrorCode::NotSupported);
}

Result<CharHandle> Central::discover_characteristic(ConnHandle conn_handle,
                                                    const ServiceHandle& service,
                                                    const UUID& char_uuid) {
    (void)conn_handle;
    (void)service;
    (void)char_uuid;
    return Result<CharHandle>::error(ErrorCode::NotSupported);
}

Result<u8> Central::get_characteristics(ConnHandle conn_handle, const ServiceHandle& service,
                                        CharHandle* characteristics, u8 max_chars) const {
    (void)conn_handle;
    (void)service;
    (void)characteristics;
    (void)max_chars;
    return Result<u8>::error(ErrorCode::NotSupported);
}

// Characteristic operations stubs
Result<u16> Central::read_char(ConnHandle conn_handle, const CharHandle& characteristic, u8* buffer,
                               u16 buffer_size) {
    (void)conn_handle;
    (void)characteristic;
    (void)buffer;
    (void)buffer_size;
    return Result<u16>::error(ErrorCode::NotSupported);
}

Result<void> Central::write_char(ConnHandle conn_handle, const CharHandle& characteristic,
                                 const u8* data, u16 length) {
    (void)conn_handle;
    (void)characteristic;
    (void)data;
    (void)length;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Central::write_char_no_response(ConnHandle conn_handle,
                                             const CharHandle& characteristic, const u8* data,
                                             u16 length) {
    (void)conn_handle;
    (void)characteristic;
    (void)data;
    (void)length;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Central::subscribe(ConnHandle conn_handle, const CharHandle& characteristic) {
    (void)conn_handle;
    (void)characteristic;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Central::unsubscribe(ConnHandle conn_handle, const CharHandle& characteristic) {
    (void)conn_handle;
    (void)characteristic;
    return Result<void>::error(ErrorCode::NotSupported);
}

// Callbacks
void Central::set_scan_callback(ScanCallback callback) {
    impl_->data.scan_callback = callback;
}

void Central::set_connection_callback(ConnectionCallback callback) {
    impl_->data.conn_callback = callback;
}

void Central::set_read_callback(ReadCallback callback) {
    impl_->data.read_callback = callback;
}

void Central::set_notify_callback(WriteCallback callback) {
    impl_->data.notify_callback = callback;
}

}  // namespace alloy::ble
