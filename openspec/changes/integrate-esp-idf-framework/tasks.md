# Implementation Tasks

## ✅ Phase 1 Complete - Build System Foundation + WiFi Example (Sections 1.1-1.4 + 3.5.1)

**Status**: ✅ **Fully Implemented and Validated**
**Documentation**: See `ESP_IDF_INTEGRATION_STATUS.md` and `ESP32_WIFI_EXAMPLE_COMPLETE.md`

**Files Modified/Created**:
- `cmake/platform/esp32_integration.cmake` - Added `alloy_detect_esp_components()` and `alloy_esp32_component()`
- `examples/blink_esp32/CMakeLists.txt` - Updated ESP-IDF detection
- `examples/rtos_blink_esp32/CMakeLists.txt` - Updated ESP-IDF detection
- `examples/esp32_wifi_station/` - **NEW** Complete WiFi Station example with ESP-IDF structure
  - `CMakeLists.txt` - ESP-IDF project root
  - `main/CMakeLists.txt` - Component registration
  - `main/main.cpp` - Full WiFi + HTTP implementation (300+ lines)
  - `sdkconfig.defaults` - Optimized configuration
  - `README.md` - Complete documentation
  - `BUILD_SUCCESS.md` - Build validation
  - `build.sh` - Helper script
- `docs/ESP32_AUTOMATIC_COMPONENTS.md` - Complete auto-detection guide
- `ESP_IDF_INTEGRATION_STATUS.md` - Implementation status
- `ESP32_WIFI_EXAMPLE_COMPLETE.md` - Executive summary

**Key Features Implemented**:
- ✅ Automatic ESP-IDF component detection from source includes
- ✅ Component detection for WiFi, BLE, HTTP, MQTT, TLS
- ✅ ESP-IDF version validation (warns if < 5.0)
- ✅ Proper ESP-IDF project structure (hybrid build system)
- ✅ **Working WiFi Station example that compiles successfully**
- ✅ Comprehensive documentation with examples

**Build Validation**:
- ✅ esp32_wifi_station compiles: **831 KB binary, 0 errors, 0 warnings**
- ✅ All ESP-IDF components linked correctly
- ✅ Automatic component detection validated
- ✅ Ready for hardware testing

**Solution Implemented**:
- ✅ Hybrid build system (Option 1) successfully implemented
- ✅ ESP-IDF examples use proper ESP-IDF project structure
- ✅ Bare-metal examples keep traditional CMake structure
- ✅ Both approaches coexist in the repository

---

## 1. Build System Enhancement

### 1.1 Automatic Component Detection
- [x] 1.1.1 Implement CMake function to scan source files for ESP-IDF includes
- [x] 1.1.2 Create mapping from include headers to ESP-IDF component names
  - `esp_wifi.h` → `esp_wifi`, `esp_netif`, `nvs_flash`, `wpa_supplicant`
  - `esp_bt.h` → `bt`, `nvs_flash`
  - `esp_http_server.h` → `esp_http_server`
  - `mqtt_client.h` → `mqtt`
  - `esp_http_client.h` → `esp_http_client`
  - `esp_tls.h` → `esp-tls`, `mbedtls`
- [x] 1.1.3 Integrate auto-detection into helper function (available for examples to use)
- [x] 1.1.4 Add configuration option to disable auto-detection (`NO_AUTO_DETECT` flag in `alloy_esp32_component()`)
- [x] 1.1.5 Function tested and ready for use

### 1.2 Enhanced CMake Integration
- [x] 1.2.1 Create `alloy_esp32_component()` CMake helper function
  - Wraps `idf_component_register()` with CoreZero conventions
  - Handles automatic component detection
  - Provides consistent SRCS and INCLUDE_DIRS patterns
- [x] 1.2.2 Update `cmake/platform/esp32_integration.cmake` with new helpers
- [x] 1.2.3 Add validation for ESP-IDF version (require >= 5.0)
- [x] 1.2.4 Warning messages added for missing components or wrong versions

### 1.3 Build Configuration
- [x] 1.3.1 Create `sdkconfig.defaults` at project root with CoreZero optimizations
  - Configure FreeRTOS settings
  - Set compiler optimizations (size optimization)
  - Configure component defaults (WiFi, BLE, logging, etc.)
- [x] 1.3.2 Support per-example `sdkconfig` overrides (ESP-IDF handles this automatically)
- [x] 1.3.3 Document sdkconfig options in `ESP_IDF_INTEGRATION_PHASE1.md`

### 1.4 Flash and Monitor Targets
- [x] 1.4.1 Flash targets already exist in `build-esp32.sh` and examples
- [x] 1.4.2 Monitor capability already available via `idf.py monitor`
- [x] 1.4.3 Combined flash-monitor available via example scripts
- [x] 1.4.4 Existing bare-metal flash targets preserved

## 2. Core Result Type Implementation

### 2.1 Result<T, Error> Type
- [ ] 2.1.1 Implement `Result<T, E>` template class in `src/corezero/core/result.hpp`
  - `Ok(T)` and `Err(E)` factory functions
  - `is_ok()`, `is_err()` query methods
  - `unwrap()`, `unwrap_or()`, `expect()` access methods
- [ ] 2.1.2 Implement monadic operations
  - `map<U>(fn: T -> U) -> Result<U, E>`
  - `and_then<U>(fn: T -> Result<U, E>) -> Result<U, E>`
  - `or_else<F>(fn: E -> F) -> Result<T, F>`
- [ ] 2.1.3 Add move semantics and perfect forwarding
- [ ] 2.1.4 Write comprehensive unit tests for Result type

### 2.2 Error Type
- [ ] 2.2.1 Define `Error` class in `src/corezero/core/error.hpp`
  - Error code (from ESP-IDF `esp_err_t` or custom)
  - Error message string
  - Source location (file, line)
- [ ] 2.2.2 Implement `Error::from_esp(esp_err_t)` conversion
- [ ] 2.2.3 Implement error code categories (WiFi, BLE, HTTP, MQTT, etc.)
- [ ] 2.2.4 Add formatting and debug output for errors

## 3. WiFi Driver Abstraction

### 3.1 WiFi Core Infrastructure
- [ ] 3.1.1 Create `src/corezero/wifi/station.hpp` header
- [ ] 3.1.2 Create `src/drivers/esp32/wifi_impl.cpp` implementation
- [ ] 3.1.3 Implement WiFi initialization wrapper
  - Initialize NVS
  - Create event loop
  - Initialize netif
  - Initialize WiFi driver
- [ ] 3.1.4 Implement event handler registration system

### 3.2 WiFi Station Mode
- [ ] 3.2.1 Implement `WiFi::Station::connect(ssid, password)` → `Result<IP, Error>`
  - Configure station mode
  - Set credentials
  - Start WiFi
  - Wait for connection with timeout
- [ ] 3.2.2 Implement `WiFi::Station::disconnect()`
- [ ] 3.2.3 Implement `WiFi::Station::isConnected()` → `bool`
- [ ] 3.2.4 Implement `WiFi::Station::getIP()` → `std::optional<IP>`
- [ ] 3.2.5 Implement connection event callbacks
  - `onConnected(callback)`
  - `onDisconnected(callback)`
  - `onGotIP(callback)`

### 3.3 WiFi Access Point Mode
- [ ] 3.3.1 Create `src/corezero/wifi/ap.hpp` header
- [ ] 3.3.2 Implement `WiFi::AP::start(ssid, password, channel)` → `Result<void, Error>`
- [ ] 3.3.3 Implement `WiFi::AP::stop()`
- [ ] 3.3.4 Implement `WiFi::AP::getConnectedStations()` → `std::vector<StationInfo>`
- [ ] 3.3.5 Implement AP event callbacks
  - `onStationConnected(callback)`
  - `onStationDisconnected(callback)`

### 3.4 WiFi Scanning
- [ ] 3.4.1 Create `src/corezero/wifi/scan.hpp` header
- [ ] 3.4.2 Implement `WiFi::scan()` → `Result<std::vector<AccessPoint>, Error>`
- [ ] 3.4.3 Define `AccessPoint` struct with SSID, BSSID, RSSI, channel, auth mode
- [ ] 3.4.4 Implement async scan with callback

### 3.5 WiFi Examples
- [x] 3.5.1 Create `examples/esp32_wifi_station/` example ✅ **COMPLETE**
  - Connect to WiFi ✅
  - Print IP address ✅
  - Perform HTTP GET request ✅
  - Handle disconnection and reconnection ✅
  - Full ESP-IDF project structure ✅
  - Compiles successfully (831 KB, 0 errors) ✅
  - Comprehensive documentation ✅
- [ ] 3.5.2 Create `examples/esp32_wifi_ap/` example
  - Start access point
  - Handle station connections
  - Print connected devices
- [x] 3.5.3 Update examples' CMakeLists.txt to use automatic component detection ✅
  - `examples/blink_esp32/CMakeLists.txt` updated ✅
  - `examples/rtos_blink_esp32/CMakeLists.txt` updated ✅
  - `examples/esp32_wifi_station/main/CMakeLists.txt` uses idf_component_register ✅

## 4. Bluetooth Low Energy Abstraction

### 4.1 BLE Core Infrastructure
- [ ] 4.1.1 Create `src/corezero/bluetooth/ble.hpp` header
- [ ] 4.1.2 Create `src/drivers/esp32/ble_impl.cpp` implementation
- [ ] 4.1.3 Implement BLE initialization wrapper
  - Initialize BT controller
  - Initialize Bluedroid stack
  - Register GAP/GATT callbacks

### 4.2 BLE Peripheral (Server)
- [ ] 4.2.1 Create `src/corezero/bluetooth/peripheral.hpp` header
- [ ] 4.2.2 Implement `BLE::Peripheral::startAdvertising(advData)` → `Result<void, Error>`
- [ ] 4.2.3 Implement GATT service registration
  - `addService(uuid, characteristics)` → `ServiceHandle`
  - `addCharacteristic(service, uuid, properties, callbacks)` → `CharHandle`
- [ ] 4.2.4 Implement characteristic read/write/notify operations
- [ ] 4.2.5 Implement connection event callbacks

### 4.3 BLE Central (Client)
- [ ] 4.3.1 Create `src/corezero/bluetooth/central.hpp` header
- [ ] 4.3.2 Implement `BLE::Central::scan(duration)` → `Result<std::vector<Device>, Error>`
- [ ] 4.3.3 Implement `BLE::Central::connect(address)` → `Result<Connection, Error>`
- [ ] 4.3.4 Implement service discovery and characteristic access

### 4.4 BLE Examples
- [ ] 4.4.1 Create `examples/esp32_ble_peripheral/` example
  - Advertise as BLE peripheral
  - Provide readable/writable characteristics
  - Send notifications on value changes
- [ ] 4.4.2 Create `examples/esp32_ble_scanner/` example
  - Scan for BLE devices
  - Print device names, addresses, RSSI
  - Filter by service UUID

## 5. HTTP Server Abstraction

### 5.1 HTTP Server Core
- [ ] 5.1.1 Create `src/corezero/http/server.hpp` header
- [ ] 5.1.2 Create `src/drivers/esp32/http_impl.cpp` implementation
- [ ] 5.1.3 Implement `HTTP::Server` class with RAII pattern
  - Constructor initializes esp_http_server
  - Destructor stops server and cleans up
- [ ] 5.1.4 Implement route registration methods
  - `get(path, handler)`
  - `post(path, handler)`
  - `put(path, handler)`
  - `delete_(path, handler)` (delete is keyword)

### 5.2 Request/Response Abstractions
- [ ] 5.2.1 Create `HTTP::Request` class wrapping `httpd_req_t`
  - `method()` → HTTP method
  - `path()` → request path
  - `query(key)` → query parameter value
  - `header(key)` → header value
  - `body()` → request body as string
  - `json()` → parse body as JSON (requires JSON library)
- [ ] 5.2.2 Create `HTTP::Response` class
  - `status(code)` → set status code
  - `header(key, value)` → set response header
  - `send(body)` → send response body
  - `json(object)` → serialize and send JSON

### 5.3 WebSocket Support
- [ ] 5.3.1 Implement WebSocket upgrade handler
- [ ] 5.3.2 Create `HTTP::WebSocket` class
  - `send(data)` → send text/binary frame
  - `onMessage(callback)` → receive messages
  - `close()` → close connection

### 5.4 HTTP Server Example
- [ ] 5.4.1 Create `examples/esp32_http_server/` example
  - Start WiFi (AP or Station)
  - Create HTTP server
  - Register REST API endpoints
  - Serve static content
  - WebSocket echo endpoint

## 6. MQTT Client Abstraction

### 6.1 MQTT Client Core
- [ ] 6.1.1 Create `src/corezero/mqtt/client.hpp` header
- [ ] 6.1.2 Create `src/drivers/esp32/mqtt_impl.cpp` implementation
- [ ] 6.1.3 Implement `MQTT::Client` class with RAII pattern
  - Constructor initializes client with broker URL
  - Destructor disconnects and cleans up
- [ ] 6.1.4 Implement connection methods
  - `connect()` → `Result<void, Error>`
  - `disconnect()`
  - `isConnected()` → `bool`

### 6.2 MQTT Pub/Sub Operations
- [ ] 6.2.1 Implement `publish(topic, payload, qos)` → `Result<void, Error>`
- [ ] 6.2.2 Implement `subscribe(topic, callback)` → `Result<void, Error>`
- [ ] 6.2.3 Implement `unsubscribe(topic)`
- [ ] 6.2.4 Implement event callbacks
  - `onConnected(callback)`
  - `onDisconnected(callback)`
  - `onError(callback)`

### 6.3 MQTT Security
- [ ] 6.3.1 Implement SSL/TLS configuration
  - CA certificate
  - Client certificate and key
- [ ] 6.3.2 Implement username/password authentication
- [ ] 6.3.3 Support mqtts:// scheme

### 6.4 MQTT Example
- [ ] 6.4.1 Create `examples/esp32_mqtt_iot/` example
  - Connect to WiFi
  - Connect to MQTT broker (mqtt.eclipseprojects.io or similar)
  - Subscribe to topic
  - Publish periodic messages
  - Handle incoming messages

## 7. HAL ESP-IDF Backend Integration

### 7.1 GPIO HAL Backend
- [ ] 7.1.1 Add `USE_ESP_IDF_DRIVERS` configuration option
- [ ] 7.1.2 Implement GPIO HAL using `driver/gpio.h` when enabled
  - `configure()` uses `gpio_config()`
  - `set_high()` / `set_low()` use `gpio_set_level()`
  - `read()` uses `gpio_get_level()`
- [ ] 7.1.3 Add interrupt support via `gpio_install_isr_service()`
- [ ] 7.1.4 Maintain same interface as bare-metal GPIO HAL

### 7.2 UART HAL Backend
- [ ] 7.2.1 Implement UART HAL using `driver/uart.h` when enabled
  - `init()` uses `uart_driver_install()` and `uart_param_config()`
  - `write()` uses `uart_write_bytes()`
  - `read()` uses `uart_read_bytes()`
- [ ] 7.2.2 Add buffered I/O support
- [ ] 7.2.3 Add async operations via UART events

### 7.3 SPI HAL Backend
- [ ] 7.3.1 Implement SPI HAL using `driver/spi_master.h` when enabled
  - `init()` uses `spi_bus_initialize()` and `spi_bus_add_device()`
  - `transfer()` uses `spi_device_transmit()`
- [ ] 7.3.2 Add DMA support for large transfers
- [ ] 7.3.3 Add transaction queuing

### 7.4 I2C HAL Backend
- [ ] 7.4.1 Implement I2C HAL using `driver/i2c.h` when enabled
  - `init()` uses `i2c_driver_install()` and `i2c_param_config()`
  - `write()` / `read()` use `i2c_master_cmd_begin()`
- [ ] 7.4.2 Add timeout handling

## 8. Documentation and Examples

### 8.1 API Documentation
- [ ] 8.1.1 Document WiFi API in `docs/api/wifi.md`
- [ ] 8.1.2 Document BLE API in `docs/api/bluetooth.md`
- [ ] 8.1.3 Document HTTP Server API in `docs/api/http.md`
- [ ] 8.1.4 Document MQTT Client API in `docs/api/mqtt.md`
- [ ] 8.1.5 Document Result<T, Error> type in `docs/api/result.md`

### 8.2 User Guides
- [ ] 8.2.1 Update `docs/ESP32_IDF_INTEGRATION.md` with new features
- [ ] 8.2.2 Create `docs/ESP32_WIFI_GUIDE.md` with WiFi examples and patterns
- [ ] 8.2.3 Create `docs/ESP32_BLE_GUIDE.md` with BLE examples and patterns
- [ ] 8.2.4 Create `docs/ESP32_IOT_GUIDE.md` covering HTTP, MQTT, cloud connectivity

### 8.3 Example Documentation
- [ ] 8.3.1 Add README.md to each example with:
  - Description
  - Hardware requirements
  - Build and flash instructions
  - Expected output
- [ ] 8.3.2 Add QUICKSTART.md for each complex example

### 8.4 Migration Guide
- [ ] 8.4.1 Create `docs/MIGRATION_ESP_IDF.md` for users migrating from bare ESP-IDF
- [ ] 8.4.2 Document differences between CoreZero and ESP-IDF APIs
- [ ] 8.4.3 Provide code comparison examples

## 9. Testing and Validation

### 9.1 Unit Tests
- [ ] 9.1.1 Write unit tests for Result<T, Error> type
- [ ] 9.1.2 Write unit tests for Error class
- [ ] 9.1.3 Mock ESP-IDF functions for testing wrappers

### 9.2 Integration Tests
- [ ] 9.2.1 Create integration test for WiFi connection (requires AP)
- [ ] 9.2.2 Create integration test for BLE advertising (requires scanner)
- [ ] 9.2.3 Create integration test for HTTP server (requires client)

### 9.3 Build Tests
- [ ] 9.3.1 Test ESP-IDF mode builds in CI
- [ ] 9.3.2 Test bare-metal mode builds in CI
- [ ] 9.3.3 Test both modes produce working binaries
- [ ] 9.3.4 Test with multiple ESP-IDF versions (5.0, 5.1, 5.2)

### 9.4 Example Validation
- [ ] 9.4.1 Manually test WiFi examples on real hardware
- [ ] 9.4.2 Manually test BLE examples on real hardware
- [ ] 9.4.3 Manually test HTTP server on real hardware
- [ ] 9.4.4 Manually test MQTT client on real hardware

## 10. CI/CD Integration

### 10.1 Docker Build Environment
- [ ] 10.1.1 Update Dockerfile with latest ESP-IDF
- [ ] 10.1.2 Ensure all tools available (esptool, idf.py, etc.)
- [ ] 10.1.3 Test Docker builds locally

### 10.2 GitHub Actions
- [ ] 10.2.1 Create CI workflow for ESP32 builds
- [ ] 10.2.2 Build all ESP32 examples in ESP-IDF mode
- [ ] 10.2.3 Build all ESP32 examples in bare-metal mode
- [ ] 10.2.4 Upload build artifacts (.bin files)
- [ ] 10.2.5 Run unit tests

### 10.3 Documentation Deployment
- [ ] 10.3.1 Add step to build and deploy docs to GitHub Pages
- [ ] 10.3.2 Ensure API docs are generated from code comments

## Dependencies and Parallelization

### Can be done in parallel:
- Section 2 (Result type) - independent
- Section 3 (WiFi) and Section 4 (BLE) and Section 5 (HTTP) and Section 6 (MQTT) - independent after Result type is done
- Section 7 (HAL backends) - independent of driver abstractions
- Section 8 (Documentation) - can start early, updated as features are completed

### Must be sequential:
- Section 1 (Build system) must be done first or in parallel with Section 2
- Section 2 (Result type) must be done before Sections 3-6 (driver abstractions)
- Section 9 (Testing) requires implementations from Sections 3-7
- Section 10 (CI/CD) requires examples from Sections 3-6

## Estimated Effort

- Section 1: 8 hours
- Section 2: 4 hours
- Section 3: 12 hours
- Section 4: 10 hours
- Section 5: 8 hours
- Section 6: 6 hours
- Section 7: 10 hours
- Section 8: 8 hours
- Section 9: 12 hours
- Section 10: 4 hours

**Total: ~82 hours** (approximately 2 weeks full-time or 4 weeks part-time)
