# ESP32 ESP-IDF Integration Guide

Complete guide for using Alloy Framework with ESP-IDF on ESP32 platforms.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Quick Start](#quick-start)
- [WiFi](#wifi)
- [Bluetooth Low Energy](#bluetooth-low-energy)
- [HTTP Server](#http-server)
- [MQTT Client](#mqtt-client)
- [HAL Drivers](#hal-drivers)
- [Build System](#build-system)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)

## Overview

Alloy provides modern C++ abstractions over ESP-IDF, offering:

- **Type Safety**: C++20 concepts and templates
- **Error Handling**: `Result<T, ErrorCode>` monadic operations
- **RAII**: Automatic resource management
- **Zero-Cost**: Same performance as hand-written ESP-IDF code
- **Easy to Use**: Intuitive APIs with comprehensive examples

## Features

### ✅ Connectivity
- **WiFi**: Station, Access Point, Scanner
- **BLE**: Central/Scanner (Phase 1), Peripheral (Phase 2 planned)
- **HTTP**: Server with REST API support
- **MQTT**: Client with pub/sub, QoS 0/1/2, TLS

### ✅ Hardware Abstraction Layer (HAL)
- **GPIO**: Digital I/O with interrupts
- **UART**: Serial communication with DMA
- **SPI**: Master mode with DMA
- **I2C**: Master mode with timeout handling

### ✅ Core Infrastructure
- **Result<T>**: Rust-inspired error handling
- **Error Types**: ESP-IDF error code mapping
- **Build System**: Automatic component detection
- **Examples**: Complete working examples for all features

## Requirements

- **ESP-IDF**: v5.0 or later
- **CMake**: 3.16 or later
- **Compiler**: C++20 capable (GCC 10+, Clang 12+)
- **Hardware**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6

### Installation

```bash
# Install ESP-IDF v5.0+
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# Activate ESP-IDF environment
. $HOME/esp/esp-idf/export.sh
```

## Quick Start

### 1. Project Structure

Alloy ESP-IDF projects use the standard ESP-IDF structure:

```
my_project/
├── CMakeLists.txt          # Project root
├── main/
│   ├── CMakeLists.txt      # Main component
│   └── main.cpp            # Application code
└── sdkconfig.defaults      # Default configuration
```

### 2. Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)

# Set Alloy root
get_filename_component(ALLOY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Add Alloy components
set(EXTRA_COMPONENT_DIRS
    "${ALLOY_ROOT}/src/wifi"
    "${ALLOY_ROOT}/src/mqtt"
    "${ALLOY_ROOT}/src/http"
    "${ALLOY_ROOT}/src/ble"
    "${ALLOY_ROOT}/src/core"
    "${ALLOY_ROOT}/src/hal/esp32"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_project)
```

### 3. Main Component CMakeLists.txt

```cmake
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES wifi mqtt http ble core nvs_flash esp_event
)
```

### 4. Minimal Application

```cpp
#include "wifi/station.hpp"
#include "esp_log.h"
#include "nvs_flash.h"

static const char* TAG = "APP";

extern "C" void app_main() {
    // Initialize NVS
    nvs_flash_init();

    ESP_LOGI(TAG, "Alloy ESP32 Application");

    // WiFi connection
    WiFi::Station wifi;
    auto result = wifi.connect("MY_SSID", "MY_PASSWORD");

    if (result.is_ok()) {
        ESP_LOGI(TAG, "WiFi Connected!");
    }
}
```

### 5. Build and Flash

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## WiFi

### Station Mode (Client)

```cpp
#include "wifi/station.hpp"

WiFi::Station wifi;

// Set connection callback
wifi.set_connection_callback([](bool connected, const WiFi::ConnectionInfo& info) {
    if (connected) {
        printf("Connected! IP: " IPSTR "\n", IP2STR(&info.ip));
    }
});

// Connect
auto result = wifi.connect("SSID", "PASSWORD");
if (result.is_ok()) {
    auto info = result.value();
    printf("RSSI: %d dBm\n", wifi.rssi().value());
}
```

### Access Point Mode (Server)

```cpp
#include "wifi/access_point.hpp"

WiFi::AccessPoint ap;

// Configure AP
WiFi::APConfig config{
    .ssid = "ESP32-AP",
    .password = "password123",
    .channel = 1,
    .max_connections = 4
};

// Start AP
auto result = ap.start(config);
if (result.is_ok()) {
    printf("AP started on IP: " IPSTR "\n", IP2STR(&result.value().ip));
}

// Monitor stations
ap.set_station_callback([](bool connected, const WiFi::MacAddress& mac) {
    printf("Station %s: %02x:%02x:%02x:%02x:%02x:%02x\n",
           connected ? "connected" : "disconnected",
           mac.addr[0], mac.addr[1], mac.addr[2],
           mac.addr[3], mac.addr[4], mac.addr[5]);
});
```

### WiFi Scanning

```cpp
#include "wifi/scanner.hpp"

WiFi::Scanner scanner;

// Blocking scan
auto result = scanner.scan();
if (result.is_ok()) {
    uint8_t count = result.value();
    WiFi::AccessPointInfo aps[20];
    scanner.get_results(aps, count);

    for (int i = 0; i < count; i++) {
        printf("%s (RSSI: %d, Ch: %d)\n",
               aps[i].ssid, aps[i].rssi, aps[i].channel);
    }
}
```

## Bluetooth Low Energy

### BLE Scanner (Central)

```cpp
#include "ble/central.hpp"

BLE::Central central;

// Scan for devices
BLE::ScanConfig scan_config{
    .duration = 5000,  // 5 seconds
    .scan_type = BLE::ScanType::Active
};

auto result = central.scan(scan_config);
if (result.is_ok()) {
    uint8_t count = result.value();
    BLE::DeviceInfo devices[50];
    central.get_scan_results(devices, count);

    for (int i = 0; i < count; i++) {
        printf("%s (%02x:%02x:...) RSSI: %d\n",
               devices[i].name, devices[i].address[0],
               devices[i].address[1], devices[i].rssi);
    }
}
```

### Async Scanning

```cpp
// Set scan callback
central.set_scan_callback([](const BLE::DeviceInfo& device) {
    printf("Found: %s (RSSI: %d)\n", device.name, device.rssi);
});

// Start async scan
central.scan_async(scan_config);
```

## HTTP Server

```cpp
#include "http/server.hpp"

HTTP::Server server(80);

// Define routes
server.get("/", [](HTTP::Request& req, HTTP::Response& res) -> HTTP::Status {
    return res.html("<h1>Hello from ESP32!</h1>").send();
});

server.get("/api/status", [](HTTP::Request& req, HTTP::Response& res) -> HTTP::Status {
    return res.json("{\"status\":\"ok\",\"uptime\":12345}").send();
});

server.post("/api/data", [](HTTP::Request& req, HTTP::Response& res) -> HTTP::Status {
    char body[256];
    auto result = req.body(body, sizeof(body));

    if (result.is_ok()) {
        printf("Received: %s\n", body);
        return res.text("Data received").send();
    }
    return HTTP::Status::BadRequest;
});

// Start server
auto result = server.start();
if (result.is_ok()) {
    printf("Server started on port 80\n");
}
```

## MQTT Client

```cpp
#include "mqtt/client.hpp"

// Configure client
MQTT::Config config{
    .broker_uri = "mqtt://broker.hivemq.com",
    .client_id = "esp32_client",
    .keepalive = 120,
    .lwt_topic = "device/status",
    .lwt_message = "offline",
    .lwt_qos = MQTT::QoS::AtLeastOnce,
    .lwt_retain = true
};

MQTT::Client mqtt(config);

// Connection callback
mqtt.set_connection_callback([](bool connected, MQTT::ErrorReason reason) {
    if (connected) {
        printf("MQTT Connected!\n");
    } else {
        printf("MQTT Disconnected: %d\n", static_cast<int>(reason));
    }
});

// Connect
auto result = mqtt.connect();
if (result.is_ok()) {
    // Subscribe to topic
    mqtt.subscribe("sensors/+/temperature", MQTT::QoS::AtLeastOnce,
        [](const MQTT::Message& msg) {
            printf("Topic: %.*s, Data: %.*s\n",
                   msg.topic.length(), msg.topic.data(),
                   msg.length, msg.data);
        });

    // Publish message
    mqtt.publish("sensors/esp32/temperature", "25.5",
                MQTT::QoS::AtLeastOnce);
}
```

### Secure MQTT (TLS)

```cpp
extern const uint8_t ca_cert_pem_start[] asm("_binary_ca_cert_pem_start");

MQTT::Config config{
    .broker_uri = "mqtts://secure-broker.com",
    .port = 8883,
    .ca_cert = (const char*)ca_cert_pem_start,
    .username = "user",
    .password = "pass"
};
```

## HAL Drivers

### GPIO

```cpp
#include "hal/esp32/gpio.hpp"

using namespace alloy::hal::esp32;

// LED on GPIO 2
GpioOutput<2> led;
led.set_high();
led.toggle();

// Button with interrupt
GpioInputPullUp<0> button;
button.attach_interrupt(GPIO_INTR_NEGEDGE, []() {
    printf("Button pressed!\n");
});
```

### UART

```cpp
#include "hal/esp32/uart.hpp"

using namespace alloy::hal::esp32;

Uart0 serial;
serial.init(GPIO_NUM_1, GPIO_NUM_3);  // TX, RX
serial.configure(UartConfig{115200_baud});

serial.write_string("Hello, UART!\n");

auto result = serial.read_byte(1000);  // 1 second timeout
if (result.is_ok()) {
    uint8_t byte = result.value();
}
```

### SPI

```cpp
#include "hal/esp32/spi.hpp"

using namespace alloy::hal::esp32;

Spi2 spi;
spi.init(GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_13);  // CLK, MISO, MOSI

auto device_result = spi.add_device(GPIO_NUM_15,  // CS
    SpiConfig{SpiMode::Mode0, 1000000});  // 1 MHz

if (device_result.is_ok()) {
    auto device = device_result.value();
    uint8_t tx[] = {0x01, 0x02};
    uint8_t rx[2];
    device.transfer(tx, rx);
}
```

### I2C

```cpp
#include "hal/esp32/i2c.hpp"

using namespace alloy::hal::esp32;

I2c0 i2c;
i2c.init(GPIO_NUM_21, GPIO_NUM_22);  // SDA, SCL
i2c.configure(I2cConfig{I2cSpeed::Fast});  // 400 kHz

// Read register
uint8_t reg_addr = 0x00;
uint8_t data[2];
i2c.write_read(0x50, {&reg_addr, 1}, data);

// Scan bus
auto scan_result = i2c.scan();
if (scan_result.is_ok()) {
    for (uint8_t addr : scan_result.value()) {
        printf("Device at 0x%02X\n", addr);
    }
}
```

## Build System

### Automatic Component Detection

Alloy automatically detects required ESP-IDF components based on `#include` statements:

- `#include "esp_wifi.h"` → links `esp_wifi`, `esp_netif`, `nvs_flash`
- `#include "esp_bt.h"` → links `bt`, `nvs_flash`
- `#include "mqtt_client.h"` → links `mqtt`

This is handled by `cmake/platform/esp32_integration.cmake`.

### Manual Component Requirements

```cmake
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES wifi mqtt http  # Manual dependencies
)
```

## Examples

Alloy includes comprehensive examples:

| Example | Description | Key Features |
|---------|-------------|--------------|
| `esp32_wifi_station` | WiFi client | Station mode, HTTP GET |
| `esp32_wifi_ap` | WiFi access point | AP mode, station monitoring |
| `esp32_wifi_scanner` | WiFi scanner | Network scanning, RSSI |
| `esp32_ble_scanner` | BLE scanner | Device discovery, async scan |
| `esp32_http_server` | HTTP server | REST API, JSON responses |
| `esp32_mqtt_iot` | MQTT client | Pub/sub, QoS, LWT, TLS |

### Running Examples

```bash
cd examples/esp32_wifi_station
./build.sh
idf.py -p /dev/ttyUSB0 flash monitor
```

## Troubleshooting

### Build Errors

**Problem**: `ESP-IDF not found`
```
Solution: Source ESP-IDF environment
. $HOME/esp/esp-idf/export.sh
```

**Problem**: `Component not found`
```cmake
# Ensure EXTRA_COMPONENT_DIRS includes Alloy components
set(EXTRA_COMPONENT_DIRS "${ALLOY_ROOT}/src/wifi" ...)
```

### WiFi Issues

**Problem**: Connection fails
```
- Verify SSID and password
- Check WiFi signal strength
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check firewall/router settings
```

### MQTT Issues

**Problem**: Connection timeout
```
- Verify broker URL and port
- Check network connectivity
- Test with public broker: mqtt://test.mosquitto.org
- Enable MQTT logs: CONFIG_LOG_DEFAULT_LEVEL_DEBUG
```

### Memory Issues

**Problem**: Heap allocation failed
```
- Reduce buffer sizes
- Disable unused features in sdkconfig
- Enable PSRAM if available
- Monitor heap with esp_get_free_heap_size()
```

## Performance Tips

1. **Enable Compiler Optimizations**
   ```cmake
   CONFIG_COMPILER_OPTIMIZATION_SIZE=y  # or _PERF
   ```

2. **Use DMA for Large Transfers**
   ```cpp
   spi.init(..., 0, true);  // Enable DMA
   ```

3. **Configure Task Priorities**
   ```cpp
   CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096
   ```

4. **Optimize WiFi Settings**
   ```cmake
   CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10
   CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32
   ```

## Additional Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [Alloy Examples](../examples/)
- [ESP32 HAL Documentation](../src/hal/esp32/README.md)
- [OpenSpec Design Documents](../openspec/changes/integrate-esp-idf-framework/)

## Support

For issues and questions:
- Check existing examples in `examples/`
- Review documentation in `docs/`
- Search ESP-IDF documentation
- Open GitHub issue with detailed description

## License

Alloy Framework is open source. See LICENSE file for details.
