# ESP-IDF Integration Design

## Context

Alloy Framework aims to provide modern C++20 development for embedded systems with portability across multiple architectures. ESP32 is a key platform that offers rich connectivity (WiFi, Bluetooth) and extensive ecosystem via ESP-IDF.

Currently, Alloy has basic ESP32 HAL support but developers cannot easily access ESP-IDF's advanced features. This integration will make ESP-IDF a first-class citizen while maintaining Alloy's philosophy of simplicity and modern C++.

### Stakeholders
- **Embedded Developers**: Need easy access to WiFi, BLE, and ESP-IDF components
- **Framework Maintainers**: Need clean abstraction that doesn't leak ESP-IDF details
- **IoT Application Developers**: Need production-ready networking and connectivity

### Constraints
- Must maintain bare-metal mode for minimal use cases
- Must not force ESP-IDF dependency on non-ESP32 platforms
- Must preserve Alloy's modern C++ interface
- Must support both FreeRTOS (ESP-IDF) and Alloy RTOS

## Goals / Non-Goals

### Goals
1. **Automatic Component Detection**: Developer includes header → component is linked
2. **Zero-Configuration UX**: No manual CMakeLists.txt editing for common cases
3. **Modern C++ API**: Wrap ESP-IDF C APIs in RAII, exceptions/Result, async
4. **Comprehensive Examples**: WiFi, BLE, HTTP, MQTT examples that "just work"
5. **Incremental Adoption**: Use as much or as little of ESP-IDF as needed
6. **Build Mode Transparency**: Same code works in ESP-IDF and bare-metal modes

### Non-Goals
1. **Not replacing ESP-IDF**: Alloy is a wrapper, not a reimplementation
2. **Not supporting all ESP-IDF components**: Focus on most common (WiFi, BLE, HTTP, MQTT)
3. **Not breaking existing bare-metal code**: Opt-in integration only
4. **Not abstracting ESP32 specifics**: Some features are ESP32-specific (OK to expose)

## Decisions

### Decision 1: Dual Build Mode Architecture

**Choice**: Support both ESP-IDF mode and bare-metal mode via CMake configuration

**Rationale**:
- Some users want minimal bare-metal (learning, size constraints)
- Some users need ESP-IDF features (WiFi, production apps)
- Dual mode provides flexibility without forcing dependencies

**Implementation**:
```cmake
# In examples/*/CMakeLists.txt
if(DEFINED ENV{IDF_PATH} AND EXISTS "$ENV{IDF_PATH}/tools/cmake/idf.cmake")
    set(USE_ESP_IDF TRUE)
    # ESP-IDF mode: use idf_component_register()
    idf_component_register(SRCS ... REQUIRES ...)
else()
    set(USE_ESP_IDF FALSE)
    # Bare-metal mode: use add_executable()
    add_executable(...)
endif()
```

**Alternatives Considered**:
- **ESP-IDF only**: Rejected - forces dependency on all users
- **Separate build system**: Rejected - too complex, duplicates code
- **Runtime mode switching**: Rejected - adds overhead, complexity

### Decision 2: Automatic Component Dependency Detection

**Choice**: Scan source files for `#include` statements to detect required ESP-IDF components

**Rationale**:
- Reduces boilerplate in CMakeLists.txt
- Developers don't need to know ESP-IDF component names
- Build system handles transitive dependencies automatically

**Implementation**:
```cmake
# Scan source files for includes
file(GLOB_RECURSE SOURCES *.cpp *.hpp)
foreach(SOURCE ${SOURCES})
    file(READ ${SOURCE} CONTENT)
    if(CONTENT MATCHES "#include [\"<]esp_wifi.h[\">]")
        list(APPEND AUTO_COMPONENTS esp_wifi esp_netif nvs_flash)
    endif()
    # ... similar for other components
endforeach()

# Register with auto-detected components
idf_component_register(
    SRCS ${SOURCES}
    REQUIRES esp_system driver ${AUTO_COMPONENTS}
)
```

**Alternatives Considered**:
- **Manual REQUIRES list**: Rejected - too much boilerplate
- **Link-time detection**: Rejected - too late, causes link errors
- **Clang AST parsing**: Rejected - too complex for build system

**Trade-offs**:
- **Pro**: Zero-configuration for users
- **Pro**: Always up-to-date with code changes
- **Con**: Slightly slower configure time (file scanning)
- **Con**: Cannot detect conditional includes in `#if` blocks

### Decision 3: C++ Wrapper Layer Architecture

**Choice**: Provide thin C++ wrappers that delegate to ESP-IDF, not reimplementation

**Rationale**:
- ESP-IDF drivers are mature and optimized
- Reimplementing would duplicate effort and introduce bugs
- Wrappers provide modern C++ interface without changing behavior

**Architecture**:
```
Application Code
      ↓
Alloy C++ Wrapper (alloy::wifi::Station)
      ↓
ESP-IDF C API (esp_wifi_*)
      ↓
ESP-IDF Driver Implementation
      ↓
ESP32 Hardware
```

**Pattern**:
```cpp
// alloy/wifi/station.hpp
namespace alloy::wifi {
    class Station {
        public:
            Result<IP, Error> connect(const std::string& ssid,
                                     const std::string& password);
        private:
            // Wraps esp_wifi.h APIs
    };
}

// Implementation delegates to ESP-IDF
Result<IP, Error> Station::connect(...) {
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &config);
    if (err != ESP_OK) {
        return Err(Error::from_esp(err));
    }
    // ... more ESP-IDF calls
    return Ok(ip_address);
}
```

**Alternatives Considered**:
- **Direct ESP-IDF usage**: Rejected - not modern C++, error-prone
- **Full reimplementation**: Rejected - duplicates ESP-IDF effort
- **Header-only forwarding**: Rejected - leaks ESP-IDF types

### Decision 4: Error Handling via Result<T, Error> Type

**Choice**: Use `Result<T, Error>` pattern instead of exceptions for fallible operations

**Rationale**:
- Embedded systems often disable exceptions (code size)
- Result type makes errors explicit and forces handling
- Composable via monadic operations (map, and_then, or_else)

**Implementation**:
```cpp
// result.hpp
template<typename T, typename E>
class Result {
    public:
        static Result Ok(T value);
        static Result Err(E error);

        bool is_ok() const;
        bool is_err() const;

        T unwrap();  // Returns value or panics
        T unwrap_or(T default_value);

        template<typename F>
        auto map(F&& fn) -> Result<decltype(fn(std::declval<T>())), E>;

        template<typename F>
        auto and_then(F&& fn) -> decltype(fn(std::declval<T>()));
};

// Usage
auto result = WiFi::connect("SSID", "password");
if (result.is_ok()) {
    auto ip = result.unwrap();
    // ...
} else {
    auto error = result.unwrap_err();
    // Handle error
}

// Or monadic style
WiFi::connect("SSID", "password")
    .and_then([](IP ip) { return HTTP::get("http://example.com"); })
    .map([](Response resp) { return resp.body(); })
    .unwrap_or("default");
```

**Alternatives Considered**:
- **Exceptions**: Rejected - not embedded-friendly, increases code size
- **Error codes + output parameters**: Rejected - C-style, error-prone
- **std::optional**: Rejected - doesn't carry error information

### Decision 5: Header Organization and Namespacing

**Choice**: Alloy headers in `alloy/` directory, ESP-IDF details hidden

**Structure**:
```
src/
├── alloy/
│   ├── wifi/
│   │   ├── station.hpp       // alloy::wifi::Station
│   │   ├── ap.hpp            // alloy::wifi::AP
│   │   └── scan.hpp          // alloy::wifi::scan()
│   ├── bluetooth/
│   │   ├── ble.hpp           // alloy::bluetooth::BLE
│   │   └── gatt.hpp          // GATT server/client
│   ├── http/
│   │   ├── server.hpp        // alloy::http::Server
│   │   └── client.hpp        // alloy::http::Client
│   ├── mqtt/
│   │   └── client.hpp        // alloy::mqtt::Client
│   └── hal/
│       ├── gpio.hpp          // alloy::hal::GPIO (can use ESP-IDF backend)
│       ├── uart.hpp          // alloy::hal::UART
│       └── spi.hpp           // alloy::hal::SPI
└── drivers/
    └── esp32/                // ESP-IDF wrapper implementations
        ├── wifi_impl.cpp
        ├── ble_impl.cpp
        └── ...
```

**Rationale**:
- Clear separation between interface (headers) and implementation
- Users include Alloy headers, not ESP-IDF headers directly
- ESP-IDF types wrapped and hidden in implementation

**Alternatives Considered**:
- **Flat structure**: Rejected - unclear organization
- **ESP-IDF headers exposed**: Rejected - leaks implementation details

### Decision 6: Component Configuration via idf_component.yml

**Choice**: Use ESP-IDF's component manager for optional components

**Example** (`examples/esp32_wifi_station/idf_component.yml`):
```yaml
dependencies:
  idf:
    version: ">=5.0"
  # Can add external components from component registry
  # e.g., esp-mqtt, esp-protocols, etc.
```

**Rationale**:
- Leverages ESP-IDF ecosystem
- Easy to add external components from Espressif registry
- Version management built-in

**Alternatives Considered**:
- **Git submodules**: Rejected - harder to manage versions
- **Manual download**: Rejected - not reproducible

## Risks / Trade-offs

### Risk 1: ESP-IDF Version Compatibility
**Risk**: Different ESP-IDF versions have different APIs

**Mitigation**:
- Document minimum ESP-IDF version (v5.0+)
- Test against multiple ESP-IDF versions in CI
- Use `#if ESP_IDF_VERSION >= ...` for version-specific code
- Provide clear error messages for unsupported versions

### Risk 2: Binary Size Increase
**Risk**: ESP-IDF components increase binary size significantly

**Mitigation**:
- Only link components that are actually used (automatic detection)
- Provide `sdkconfig.defaults` with size optimizations
- Document size impact of each component
- Keep bare-metal mode for size-sensitive applications

**Data**:
- Bare-metal blink: ~30 KB
- ESP-IDF minimal (esp_system, driver): ~200 KB
- ESP-IDF + WiFi: ~600 KB
- ESP-IDF + WiFi + BLE: ~1 MB

### Risk 3: Build Complexity
**Risk**: Dual build mode adds complexity to CMake

**Mitigation**:
- Comprehensive tests for both build modes
- Clear documentation with examples
- Error messages guide users to correct configuration
- Provide build scripts that handle common cases

### Risk 4: ESP-IDF Dependency Management
**Risk**: Users might have wrong ESP-IDF version or missing tools

**Mitigation**:
- `scripts/setup_esp_idf.sh` automates installation
- Docker container with pre-installed ESP-IDF
- Clear error messages with installation instructions
- Version check in CMake configuration

### Risk 5: API Instability
**Risk**: Alloy API might need to change as ESP-IDF evolves

**Mitigation**:
- Follow semantic versioning
- Deprecation warnings before breaking changes
- Wrapper layer isolates users from ESP-IDF API changes
- Comprehensive tests detect breaking changes early

## Migration Plan

### Phase 1: Foundation (Current PR)
- [x] Basic ESP-IDF detection in CMake
- [x] Dual build mode for examples
- [x] Documentation

### Phase 2: Build System Enhancement
- [ ] Automatic component detection from includes
- [ ] Improved CMakeLists.txt helpers
- [ ] Flash and monitor targets
- [ ] CI integration

### Phase 3: Driver Wrappers
- [ ] WiFi Station/AP C++ wrappers
- [ ] BLE peripheral/central wrappers
- [ ] HTTP server wrapper
- [ ] MQTT client wrapper

### Phase 4: HAL Backend Integration
- [ ] GPIO HAL with ESP-IDF backend option
- [ ] UART HAL with ESP-IDF backend option
- [ ] SPI HAL with ESP-IDF backend option
- [ ] I2C HAL with ESP-IDF backend option

### Phase 5: Examples and Documentation
- [ ] WiFi Station example (connect, HTTP GET)
- [ ] BLE Peripheral example (GATT services)
- [ ] HTTP Server example (REST API)
- [ ] MQTT IoT example (pub/sub)
- [ ] Complete API documentation

### Rollback Plan
If integration causes issues:
1. Bare-metal mode remains functional (fallback)
2. ESP-IDF integration is opt-in (safe default)
3. Specific examples can disable ESP-IDF mode
4. Docker environment pins ESP-IDF version (reproducible)

## Open Questions

### Q1: Should we support ESP-IDF's FreeRTOS or Alloy RTOS?
**Options**:
- A) Use ESP-IDF's FreeRTOS when in ESP-IDF mode
- B) Port Alloy RTOS to work with ESP-IDF
- C) Support both (configurable)

**Current thinking**: Option C - Let user choose. ESP-IDF's FreeRTOS is battle-tested and integrates well with WiFi/BLE. Alloy RTOS is simpler and educational. Provide both options.

**Configuration**:
```cmake
option(USE_ALLOY_RTOS "Use Alloy RTOS instead of FreeRTOS" OFF)
```

### Q2: How to handle sdkconfig in multi-example project?
**Options**:
- A) Single sdkconfig at project root
- B) Per-example sdkconfig
- C) Shared sdkconfig.defaults + per-example overrides

**Current thinking**: Option C - Shared defaults for common settings, examples can override specific settings.

### Q3: Should Alloy abstractions support non-ESP32 WiFi/BLE chips?
**Current answer**: Not in this phase. Focus on ESP32 first. If other platforms need WiFi/BLE (e.g., RP2040W), can extend later. Architecture allows platform-specific implementations behind common interface.

### Q4: How to handle ESP-IDF event loop vs Alloy event system?
**Options**:
- A) Use ESP-IDF event loop exclusively
- B) Bridge ESP-IDF events to Alloy event system
- C) Keep them separate

**Current thinking**: Option A for now - Use ESP-IDF event loop when in ESP-IDF mode. It's well-integrated with WiFi/BLE. Can add bridge later if needed.

## Success Metrics

1. **Ease of Use**: Developer can add WiFi to blink example in < 10 lines of code
2. **Build Time**: Incremental builds < 5 seconds for typical changes
3. **Binary Size**: WiFi example < 700 KB (including all components)
4. **Documentation**: Every component has working example
5. **Compatibility**: Works with ESP-IDF v5.0, v5.1, v5.2+
6. **CI Coverage**: All examples build and pass tests in CI
