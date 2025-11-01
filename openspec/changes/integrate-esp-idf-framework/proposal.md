# Integrate ESP-IDF Framework

## Why

Currently, CoreZero has basic ESP32 HAL support but doesn't fully leverage ESP-IDF's rich ecosystem of drivers, components, and services. Users must manually configure ESP-IDF components and cannot easily access WiFi, Bluetooth, advanced peripherals, or ESP-IDF's optimized drivers. This creates friction when developers want to use ESP32's full capabilities.

By integrating ESP-IDF as a first-class framework option, developers can seamlessly use all ESP-IDF components (WiFi, BLE, HTTP server, MQTT, etc.) with simple `#include` statements, while CoreZero's build system automatically handles component registration, linking, and configuration.

## What Changes

- **Build System Enhancement**: Extend CMake integration to automatically register CoreZero code as ESP-IDF components
- **Driver Abstraction Layer**: Create seamless mapping between CoreZero HAL and ESP-IDF drivers
- **Automatic Component Detection**: Build system detects which ESP-IDF components are needed based on includes
- **Header Forwarding**: Provide CoreZero-style headers that forward to ESP-IDF components (`#include "wifi/station.hpp"` → esp_wifi)
- **Configuration Integration**: Merge sdkconfig with CoreZero build configuration
- **Zero-Configuration UX**: Developers just `#include` what they need; build system handles the rest

### Breaking Changes
- None. ESP-IDF integration is opt-in via board selection. Existing bare-metal mode remains unchanged.

## Impact

### Affected Specs
- **NEW**: `esp-idf-integration` - ESP-IDF framework integration specification
- **NEW**: `build-system` - Enhanced CMake ESP-IDF component system
- **NEW**: `driver-abstraction` - CoreZero HAL to ESP-IDF driver mapping
- **MODIFIED**: `hal-esp32` - Extended to support optional ESP-IDF drivers

### Affected Code
- `CMakeLists.txt` (root) - Enhanced ESP-IDF detection and component registration
- `cmake/platform/esp32_integration.cmake` - Automatic component management
- `examples/*/CMakeLists.txt` - Component registration for ESP32 examples
- `src/drivers/esp32/` - NEW: ESP-IDF driver wrappers (WiFi, BLE, etc.)
- `src/hal/esp32/` - Enhanced with ESP-IDF driver backend option
- `boards/esp32_devkit/` - Integrated sdkconfig and board configuration

### User Experience Impact
**Before** (Manual ESP-IDF setup):
```cpp
// User must manually configure ESP-IDF components
// in CMakeLists.txt and sdkconfig
extern "C" {
    #include "esp_wifi.h"
    #include "esp_event.h"
}
// Then initialize everything manually...
```

**After** (Automatic integration):
```cpp
#include "wifi/station.hpp"  // CoreZero detects esp_wifi component

void setup() {
    WiFi::connect("SSID", "password");  // Just works!
}
```

## Success Criteria

1. Developer includes ESP-IDF component header → Build system automatically links component
2. No manual component registration in CMakeLists.txt
3. CoreZero HAL can optionally use ESP-IDF drivers as backend
4. Full access to ESP-IDF ecosystem (WiFi, BLE, HTTP, MQTT, etc.)
5. Zero-configuration for common use cases
6. Comprehensive examples demonstrating WiFi, BLE, and IoT capabilities

## References

- ESP-IDF Component System: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html
- ESP-IDF Build System: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#component-requirements
- Existing implementation: `cmake/platform/esp32_integration.cmake`, `docs/ESP32_IDF_INTEGRATION.md`
