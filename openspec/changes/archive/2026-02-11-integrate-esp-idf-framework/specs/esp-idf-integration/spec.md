# ESP-IDF Integration Specification

## ADDED Requirements

### Requirement: Automatic ESP-IDF Component Registration
The build system SHALL automatically register Alloy examples and applications as ESP-IDF components when ESP-IDF is detected.

#### Scenario: ESP-IDF detected via IDF_PATH
- **WHEN** `$IDF_PATH` environment variable is set
- **AND** `$IDF_PATH/tools/cmake/idf.cmake` exists
- **THEN** the build system SHALL use `idf_component_register()` instead of `add_executable()`
- **AND** SHALL register Alloy source files as component SRCS
- **AND** SHALL declare ESP-IDF component dependencies via REQUIRES

#### Scenario: Bare-metal fallback when ESP-IDF not detected
- **WHEN** `$IDF_PATH` is not set or ESP-IDF is not found
- **THEN** the build system SHALL use traditional CMake `add_executable()`
- **AND** SHALL compile in bare-metal mode without ESP-IDF dependencies
- **AND** SHALL not require ESP-IDF to be installed

#### Scenario: Component dependencies automatically resolved
- **WHEN** Alloy code includes ESP-IDF headers (e.g., `esp_wifi.h`)
- **THEN** the build system SHALL automatically add corresponding component to REQUIRES
- **AND** SHALL link against ESP-IDF component libraries
- **AND** SHALL include component header paths

### Requirement: ESP-IDF Driver Access
The system SHALL provide seamless access to ESP-IDF drivers and components through Alloy abstractions.

#### Scenario: WiFi driver access
- **WHEN** application code includes `wifi/station.hpp`
- **THEN** the build system SHALL link `esp_wifi` component
- **AND** SHALL link `esp_netif` component
- **AND** SHALL link `nvs_flash` component (required for WiFi)
- **AND** Alloy SHALL provide C++ wrapper around `esp_wifi` APIs

#### Scenario: Bluetooth Low Energy access
- **WHEN** application code includes `bluetooth/ble.hpp`
- **THEN** the build system SHALL link `bt` component
- **AND** Alloy SHALL provide C++ wrapper around ESP-IDF BLE APIs

#### Scenario: HTTP server access
- **WHEN** application code includes `http/server.hpp`
- **THEN** the build system SHALL link `esp_http_server` component
- **AND** Alloy SHALL provide modern C++ interface for HTTP server

#### Scenario: MQTT client access
- **WHEN** application code includes `mqtt/client.hpp`
- **THEN** the build system SHALL link `mqtt` component
- **AND** Alloy SHALL provide async C++ wrapper for MQTT

### Requirement: Component Include Detection
The build system SHALL automatically detect required ESP-IDF components based on header includes.

#### Scenario: Automatic component detection from includes
- **WHEN** CMake scans source files before build
- **AND** finds `#include "esp_wifi.h"` in any source file
- **THEN** it SHALL add `esp_wifi` to component REQUIRES list
- **AND** SHALL recursively resolve component dependencies

#### Scenario: User override of automatic detection
- **WHEN** user explicitly lists components in `idf_component.yml`
- **THEN** automatic detection SHALL be disabled
- **AND** only user-specified components SHALL be linked

### Requirement: Configuration Integration
The system SHALL integrate ESP-IDF's sdkconfig with Alloy's build configuration.

#### Scenario: Default sdkconfig provided
- **WHEN** building ESP32 project for first time
- **THEN** Alloy SHALL provide sensible default `sdkconfig.defaults`
- **AND** SHALL include configurations for Alloy RTOS integration
- **AND** SHALL optimize for Alloy's use cases (e.g., disable FreeRTOS if using Alloy RTOS)

#### Scenario: User sdkconfig customization
- **WHEN** user creates `sdkconfig` or `sdkconfig.defaults` in project root
- **THEN** user settings SHALL override Alloy defaults
- **AND** SHALL be merged with ESP-IDF defaults

#### Scenario: Configuration validation
- **WHEN** incompatible configurations are detected (e.g., using both FreeRTOS and Alloy RTOS)
- **THEN** build system SHALL emit clear warning
- **AND** SHALL suggest correct configuration

### Requirement: Alloy HAL ESP-IDF Backend
The Alloy HAL SHALL optionally use ESP-IDF drivers as implementation backend.

#### Scenario: GPIO HAL using ESP-IDF driver
- **WHEN** `USE_ESP_IDF_DRIVERS=ON` is set
- **AND** GPIO operations are called
- **THEN** Alloy GPIO HAL SHALL delegate to `driver/gpio.h`
- **AND** SHALL maintain Alloy's C++ interface
- **AND** SHALL provide same behavior as bare-metal implementation

#### Scenario: UART HAL using ESP-IDF driver
- **WHEN** `USE_ESP_IDF_DRIVERS=ON` is set
- **AND** UART operations are called
- **THEN** Alloy UART HAL SHALL delegate to `driver/uart.h`
- **AND** SHALL support ESP-IDF's buffered I/O
- **AND** SHALL support async operations via ESP-IDF events

#### Scenario: SPI HAL using ESP-IDF driver
- **WHEN** `USE_ESP_IDF_DRIVERS=ON` is set
- **AND** SPI operations are called
- **THEN** Alloy SPI HAL SHALL delegate to `driver/spi_master.h`
- **AND** SHALL support DMA transfers
- **AND** SHALL support transactions

### Requirement: Zero-Configuration WiFi Setup
The system SHALL provide zero-configuration WiFi setup for common use cases.

#### Scenario: WiFi Station mode initialization
- **WHEN** user code calls `WiFi::connect("SSID", "password")`
- **THEN** Alloy SHALL automatically:
  - Initialize NVS flash
  - Create default event loop
  - Initialize TCP/IP stack (netif)
  - Initialize WiFi driver
  - Configure station mode
  - Connect to network
- **AND** SHALL return when connected or timeout

#### Scenario: WiFi AP mode initialization
- **WHEN** user code calls `WiFi::startAP("SSID", "password")`
- **THEN** Alloy SHALL automatically configure access point
- **AND** SHALL start DHCP server
- **AND** SHALL return when AP is active

#### Scenario: WiFi connection callbacks
- **WHEN** WiFi connection state changes
- **THEN** Alloy SHALL invoke user-registered callbacks
- **AND** SHALL provide event data (IP address, disconnect reason, etc.)

### Requirement: Component Optimization
The build system SHALL only link ESP-IDF components that are actually used.

#### Scenario: Minimal binary size for basic examples
- **WHEN** building blink example without WiFi/BT
- **THEN** ESP-IDF components SHALL be minimal (esp_system, driver only)
- **AND** WiFi and Bluetooth components SHALL NOT be linked
- **AND** binary size SHALL be comparable to bare-metal build

#### Scenario: Incremental component addition
- **WHEN** developer adds `#include "wifi/station.hpp"`
- **AND** rebuilds project
- **THEN** build system SHALL detect new dependency
- **AND** SHALL link WiFi-related components
- **AND** SHALL increase binary size only by necessary components

### Requirement: Comprehensive Examples
The system SHALL provide comprehensive examples demonstrating ESP-IDF integration.

#### Scenario: WiFi Station example
- **WHEN** building `examples/esp32_wifi_station`
- **THEN** SHALL demonstrate:
  - WiFi connection
  - HTTP GET request
  - Event handling
  - Error recovery

#### Scenario: BLE Peripheral example
- **WHEN** building `examples/esp32_ble_peripheral`
- **THEN** SHALL demonstrate:
  - BLE advertising
  - GATT services
  - Characteristic read/write
  - Notifications

#### Scenario: HTTP Server example
- **WHEN** building `examples/esp32_http_server`
- **THEN** SHALL demonstrate:
  - WiFi AP + Station
  - HTTP REST API
  - JSON request/response
  - WebSocket support

#### Scenario: MQTT IoT example
- **WHEN** building `examples/esp32_mqtt_iot`
- **THEN** SHALL demonstrate:
  - WiFi connection
  - MQTT broker connection
  - Publish/Subscribe
  - SSL/TLS connection
