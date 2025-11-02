# Alloy Core Types

Core types and utilities for the Alloy framework.

## Overview

- `types.hpp` - Basic type aliases (u8, u16, u32, i8, i16, i32, etc.)
- `error.hpp` - Result<T> and ErrorCode for error handling without exceptions
- `esp_error.hpp` - ESP-IDF integration for Result<T>
- `result.hpp` - Generic Result<T, E> template (for non-ESP platforms)
- `concepts.hpp` - C++20 concepts
- `units.hpp` - Type-safe units (voltage, current, frequency, etc.)

## Error Handling

Alloy uses a Rust-inspired `Result<T>` type for error handling without exceptions.

### Basic Usage

```cpp
#include "core/error.hpp"

using alloy::core::Result;
using alloy::core::ErrorCode;

// Function that can fail
Result<u32> read_sensor() {
    if (!sensor_ready()) {
        return Result<u32>::error(ErrorCode::Timeout);
    }

    u32 value = sensor_read_value();
    return Result<u32>::ok(value);
}

// Check result
auto result = read_sensor();
if (result.is_ok()) {
    u32 value = result.value();
    // Use value...
} else {
    ErrorCode err = result.error();
    // Handle error...
}
```

### ESP-IDF Integration

When building for ESP32 with ESP-IDF, use `esp_error.hpp` for seamless integration with Alloy:

```cpp
#include "core/error.hpp"
#include "core/esp_error.hpp"
#include <esp_wifi.h>

using alloy::core::Result;

// Convert ESP-IDF errors automatically
Result<bool> init_wifi() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        return esp_result_error<bool>(err);
    }

    return Result<bool>::ok(true);
}

// Or use ESP_TRY macro for cleaner code
Result<void> start_wifi() {
    // ESP_TRY returns early if error occurs
    ESP_TRY(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_TRY(esp_wifi_start());

    return Result<void>::ok();
}
```

### ESP_TRY Macro

The `ESP_TRY` macro simplifies ESP-IDF error handling:

```cpp
Result<IPAddress> connect_wifi(const char* ssid, const char* password) {
    // Initialize WiFi
    ESP_TRY(esp_wifi_init(&cfg));

    // Configure
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_TRY(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_TRY(esp_wifi_start());
    ESP_TRY(esp_wifi_connect());

    // If we get here, all operations succeeded
    return Result<IPAddress>::ok(get_current_ip());
}
```

### Error Conversion

ESP-IDF errors are automatically mapped to Alloy ErrorCode:

| ESP-IDF Error | Alloy ErrorCode |
|---------------|-------------------|
| ESP_OK | ErrorCode::Ok |
| ESP_ERR_INVALID_ARG | ErrorCode::InvalidParameter |
| ESP_ERR_TIMEOUT | ErrorCode::Timeout |
| ESP_ERR_NOT_SUPPORTED | ErrorCode::NotSupported |
| ESP_ERR_NO_MEM | ErrorCode::HardwareError |
| ESP_FAIL | ErrorCode::HardwareError |
| Others | ErrorCode::Unknown |

### Helper Functions

```cpp
// Check ESP error and convert to Result<void>
Result<void> init_nvs() {
    return esp_check(nvs_flash_init());
}

// Get error name string
esp_err_t err = esp_wifi_connect();
if (err != ESP_OK) {
    const char* name = esp_error_name(err);  // "ESP_ERR_WIFI_CONN"
    ESP_LOGE("APP", "WiFi failed: %s", name);
}

// Log error with tag
esp_log_error("WIFI", err, "Failed to connect");
```

## Examples

See `examples/esp32_wifi_station/` for a complete example using Result<T> with ESP-IDF.

## Type-Safe Units

```cpp
#include "core/units.hpp"

using namespace alloy::core::units;

// Type-safe voltage
Voltage v = 3.3_V;
Millivolts mv = v;  // Automatic conversion

// Type-safe frequency
Frequency f = 80_MHz;
auto period = f.period();  // Get period in nanoseconds
```

## Concepts

```cpp
#include "core/concepts.hpp"

template<Integral T>
T add(T a, T b) {
    return a + b;
}

// Only works with integral types
auto result = add(5, 10);       // OK
// auto result = add(5.0, 10.0); // Compile error
```
