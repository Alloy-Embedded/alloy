# Driver Abstraction Layer Specification

## ADDED Requirements

### Requirement: C++ Wrapper Layer for ESP-IDF Drivers
The system SHALL provide modern C++ wrappers around ESP-IDF C drivers.

#### Scenario: WiFi Station C++ wrapper
- **WHEN** application includes `<corezero/wifi/station.hpp>`
- **THEN** SHALL provide `WiFi::Station` class
- **AND** SHALL wrap `esp_wifi.h` APIs in RAII pattern
- **AND** SHALL use `std::string` for SSID/password instead of char arrays
- **AND** SHALL use exceptions or Result<T, Error> for error handling
- **AND** SHALL provide async operations via callbacks or coroutines

#### Scenario: GPIO C++ wrapper with ESP-IDF backend
- **WHEN** application includes `<corezero/hal/gpio.hpp>`
- **AND** `USE_ESP_IDF_DRIVERS=ON` is configured
- **THEN** GPIO operations SHALL delegate to `driver/gpio.h`
- **AND** SHALL maintain CoreZero's GPIO interface
- **AND** SHALL support interrupts via ESP-IDF's GPIO ISR service

#### Scenario: UART C++ wrapper with ESP-IDF backend
- **WHEN** application includes `<corezero/hal/uart.hpp>`
- **AND** `USE_ESP_IDF_DRIVERS=ON` is configured
- **THEN** UART operations SHALL delegate to `driver/uart.h`
- **AND** SHALL support DMA and buffered I/O
- **AND** SHALL provide async read/write operations

### Requirement: WiFi Driver Abstraction
The system SHALL provide comprehensive WiFi abstraction layer.

#### Scenario: WiFi initialization
- **WHEN** `WiFi::init()` is called
- **THEN** SHALL initialize NVS flash (if not initialized)
- **AND** SHALL create default event loop
- **AND** SHALL initialize TCP/IP adapter (netif)
- **AND** SHALL initialize WiFi driver with default config
- **AND** SHALL register event handlers

#### Scenario: WiFi Station connect
- **WHEN** `WiFi::Station::connect(ssid, password)` is called
- **THEN** SHALL configure station mode
- **AND** SHALL set SSID and password
- **AND** SHALL start WiFi driver
- **AND** SHALL wait for connection or timeout
- **AND** SHALL return IP address on success

#### Scenario: WiFi AP mode
- **WHEN** `WiFi::AP::start(ssid, password, channel)` is called
- **THEN** SHALL configure access point mode
- **AND** SHALL set SSID, password, and channel
- **AND** SHALL start DHCP server
- **AND** SHALL return when AP is ready

#### Scenario: WiFi event callbacks
- **WHEN** WiFi events occur (connected, disconnected, got IP, etc.)
- **THEN** SHALL invoke user-registered callback with event data
- **AND** SHALL provide type-safe event data structures
- **AND** SHALL support lambda captures and std::function

#### Scenario: WiFi scan
- **WHEN** `WiFi::scan()` is called
- **THEN** SHALL perform active scan
- **AND** SHALL return vector of `AccessPoint` structures
- **AND** each SHALL contain SSID, BSSID, RSSI, channel, auth mode

### Requirement: Bluetooth Low Energy Abstraction
The system SHALL provide BLE abstraction layer.

#### Scenario: BLE initialization
- **WHEN** `BLE::init()` is called
- **THEN** SHALL initialize Bluetooth controller
- **AND** SHALL initialize Bluedroid host stack
- **AND** SHALL register GATT/GAP callbacks

#### Scenario: BLE advertising
- **WHEN** `BLE::Peripheral::startAdvertising(advData)` is called
- **THEN** SHALL configure advertising parameters
- **AND** SHALL set advertising data
- **AND** SHALL start advertising
- **AND** SHALL be discoverable by central devices

#### Scenario: GATT service registration
- **WHEN** `BLE::Peripheral::addService(service)` is called
- **THEN** SHALL register service with characteristics
- **AND** SHALL support read/write/notify properties
- **AND** SHALL invoke callbacks on characteristic access

#### Scenario: BLE central scanning
- **WHEN** `BLE::Central::scan(duration)` is called
- **THEN** SHALL perform BLE scan for specified duration
- **AND** SHALL return vector of discovered peripherals
- **AND** each SHALL contain name, address, RSSI, service UUIDs

### Requirement: HTTP Server Abstraction
The system SHALL provide HTTP server abstraction layer.

#### Scenario: HTTP server initialization
- **WHEN** `HTTP::Server server(port)` is created
- **THEN** SHALL initialize ESP-IDF HTTP server
- **AND** SHALL bind to specified port
- **AND** SHALL be ready to register handlers

#### Scenario: Route registration
- **WHEN** `server.get("/api/data", handler)` is called
- **THEN** SHALL register GET handler for `/api/data`
- **AND** handler SHALL receive Request and Response objects
- **AND** SHALL support path parameters and query strings

#### Scenario: JSON request handling
- **WHEN** HTTP request with JSON body is received
- **THEN** Request object SHALL provide `json()` method
- **AND** SHALL parse JSON automatically
- **AND** SHALL return parsed object or error

#### Scenario: JSON response generation
- **WHEN** handler calls `response.json(object)`
- **THEN** SHALL serialize object to JSON
- **AND** SHALL set Content-Type: application/json
- **AND** SHALL send response with appropriate status code

#### Scenario: WebSocket support
- **WHEN** `server.websocket("/ws", handler)` is registered
- **AND** WebSocket upgrade request is received
- **THEN** SHALL upgrade connection to WebSocket
- **AND** SHALL invoke handler with WebSocket object
- **AND** SHALL support send/receive text and binary frames

### Requirement: MQTT Client Abstraction
The system SHALL provide MQTT client abstraction layer.

#### Scenario: MQTT client initialization
- **WHEN** `MQTT::Client client(broker_url)` is created
- **THEN** SHALL initialize ESP-IDF MQTT client
- **AND** SHALL configure broker URL
- **AND** SHALL support mqtt://, mqtts://, ws://, wss:// schemes

#### Scenario: MQTT connect
- **WHEN** `client.connect()` is called
- **THEN** SHALL establish connection to broker
- **AND** SHALL return when connected or timeout
- **AND** SHALL support username/password authentication
- **AND** SHALL support SSL/TLS certificates

#### Scenario: MQTT publish
- **WHEN** `client.publish(topic, payload, qos)` is called
- **THEN** SHALL publish message to specified topic
- **AND** SHALL support QoS 0, 1, 2
- **AND** SHALL return when message is sent (QoS 0) or acknowledged (QoS 1/2)

#### Scenario: MQTT subscribe
- **WHEN** `client.subscribe(topic, callback)` is called
- **THEN** SHALL send SUBSCRIBE packet to broker
- **AND** SHALL invoke callback when messages arrive on topic
- **AND** SHALL support topic wildcards (`+`, `#`)

#### Scenario: MQTT event callbacks
- **WHEN** MQTT events occur (connected, disconnected, error, etc.)
- **THEN** SHALL invoke user-registered event callback
- **AND** SHALL provide event type and data

### Requirement: Driver Backend Selection
The system SHALL allow selection between CoreZero bare-metal drivers and ESP-IDF drivers.

#### Scenario: ESP-IDF driver backend enabled
- **WHEN** `USE_ESP_IDF_DRIVERS=ON` is set in CMake
- **THEN** CoreZero HAL implementations SHALL use ESP-IDF drivers
- **AND** SHALL provide ESP-IDF features (DMA, interrupts, power management)

#### Scenario: Bare-metal driver backend
- **WHEN** `USE_ESP_IDF_DRIVERS=OFF` or not set
- **THEN** CoreZero HAL implementations SHALL use direct register access
- **AND** SHALL be lighter weight
- **AND** SHALL not depend on ESP-IDF

#### Scenario: Runtime driver detection
- **WHEN** application queries driver backend
- **THEN** SHALL return whether ESP-IDF or bare-metal backend is active
- **AND** SHALL be available via compile-time constant `USE_ESP_IDF`

### Requirement: Header Forwarding Pattern
The system SHALL provide CoreZero-style headers that forward to ESP-IDF components.

#### Scenario: WiFi header forwarding
- **WHEN** application includes `<corezero/wifi/station.hpp>`
- **THEN** header SHALL internally include ESP-IDF headers (`esp_wifi.h`, `esp_netif.h`)
- **AND** SHALL wrap ESP-IDF types in CoreZero namespace
- **AND** SHALL provide modern C++ interface

#### Scenario: Namespace isolation
- **WHEN** using CoreZero WiFi classes
- **THEN** ALL types SHALL be in `corezero::wifi` namespace
- **AND** ESP-IDF types SHALL be wrapped or hidden
- **AND** SHALL not pollute global namespace with ESP-IDF C types

### Requirement: Error Handling Abstraction
The system SHALL provide consistent error handling across drivers.

#### Scenario: ESP-IDF error code translation
- **WHEN** ESP-IDF function returns `esp_err_t`
- **THEN** CoreZero wrapper SHALL translate to C++ exception or Result<T, Error>
- **AND** SHALL preserve error code and message
- **AND** SHALL provide stack trace information in debug builds

#### Scenario: Result type for fallible operations
- **WHEN** operation can fail (e.g., WiFi connect)
- **THEN** SHALL return `Result<T, Error>` instead of throwing
- **AND** caller SHALL explicitly handle success/error cases
- **AND** SHALL support monadic operations (map, and_then, or_else)

#### Scenario: Exception safety
- **WHEN** ESP-IDF operation fails
- **AND** exceptions are enabled
- **THEN** CoreZero wrapper SHALL throw typed exception
- **AND** SHALL properly clean up ESP-IDF resources
- **AND** SHALL maintain RAII guarantees

### Requirement: Async Operations Support
The system SHALL support asynchronous operations for I/O and network drivers.

#### Scenario: Async WiFi operations
- **WHEN** calling `WiFi::Station::connectAsync(ssid, password, callback)`
- **THEN** SHALL return immediately
- **AND** SHALL invoke callback when connected or failed
- **AND** callback SHALL run on event loop thread

#### Scenario: Async UART read
- **WHEN** calling `UART::readAsync(buffer, size, callback)`
- **THEN** SHALL return immediately
- **AND** SHALL invoke callback when data is available
- **AND** SHALL support cancellation

#### Scenario: Coroutine support (C++20)
- **WHEN** using C++20 coroutines
- **THEN** async operations SHALL be awaitable
- **AND** `co_await WiFi::Station::connectAsync(...)` SHALL suspend until complete
- **AND** SHALL integrate with ESP-IDF event loop
