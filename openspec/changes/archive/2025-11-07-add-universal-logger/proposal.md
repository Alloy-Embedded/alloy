# Add Universal Logger System

## Why

Currently, CoreZero lacks a unified logging system. Developers must use platform-specific logging (ESP_LOG on ESP32, printf on bare-metal, Serial on Arduino) which creates several problems:

- **Inconsistent APIs**: Different logging methods across platforms break portability
- **No Runtime Control**: Cannot enable/disable logs without recompiling
- **Poor Debug Experience**: Missing timestamps, log levels, and file/line information
- **Performance Impact**: Always-on logging wastes CPU cycles and flash space
- **Production Issues**: Cannot differentiate between debug logs and critical errors

A universal logger inspired by Rust's `log`, `spdlog`, and `log4cxx` will provide:

- **Zero-cost abstraction**: Compile-time filtering removes disabled logs
- **Platform agnostic**: Works on bare-metal, RTOS, ESP-IDF, and host
- **Timestamps**: Microsecond precision using existing SysTick
- **Log levels**: TRACE, DEBUG, INFO, WARN, ERROR, with runtime filtering
- **Minimal overhead**: <2KB flash for core functionality
- **Rich context**: Optional file, line, function, and timestamp metadata
- **Flexible output**: Support for UART, USB, files, network, and custom sinks

## What Changes

- **Core Logger API**: Modern C++20 logging macros and classes
- **Platform Adapters**: Output handlers for all supported platforms
- **SysTick Integration**: Automatic timestamp from existing HAL
- **Compile-time Filtering**: Zero overhead for disabled log levels
- **Runtime Configuration**: Dynamic log level control
- **Format System**: printf-style and type-safe formatting
- **Multiple Sinks**: Console, file, network, and custom output targets

### Breaking Changes
- None. This is a new capability that existing code can adopt incrementally.

## Impact

### Affected Specs
- **NEW**: `logger-core` - Core logging system specification
- **NEW**: `logger-platform` - Platform-specific adapters
- **MODIFIED**: `systick` - Enhanced to provide uptime for logging

### Affected Code
- `src/logger/` - NEW: Core logging implementation
  - `logger.hpp` - Main logging interface
  - `logger.cpp` - Core implementation
  - `sink.hpp` - Output sink abstraction
  - `format.hpp` - Format utilities
- `src/logger/platform/` - NEW: Platform adapters
  - `uart_sink.hpp` - UART output
  - `esp_log_sink.hpp` - ESP-IDF integration
  - `file_sink.hpp` - File output (host/ESP32)
- `src/hal/interface/systick.hpp` - MODIFIED: Add milliseconds helper
- `examples/logger_demo/` - NEW: Comprehensive logging examples

### User Experience Impact

**Before** (Inconsistent logging):
```cpp
// ESP32
ESP_LOGI(TAG, "Value: %d", value);

// Bare-metal
printf("Value: %d\n", value);

// Different levels, no timestamps, hard to filter
```

**After** (Universal logger):
```cpp
#include "logger/logger.hpp"

// Consistent API everywhere
LOG_INFO("Value: {}", value);
LOG_WARN("Threshold exceeded: {} > {}", value, threshold);
LOG_ERROR("Failed: {}", error.message());

// With automatic timestamps and context:
// [0.123456] INFO  [main.cpp:42] Value: 42
// [0.234567] WARN  [main.cpp:43] Threshold exceeded: 100 > 50
// [0.345678] ERROR [main.cpp:44] Failed: Connection timeout
```

## Success Criteria

1. ✅ Single API works across all platforms (bare-metal, RTOS, ESP-IDF, host)
2. ✅ Compile-time filtering removes disabled logs (zero overhead)
3. ✅ Runtime log level control without recompiling
4. ✅ Microsecond precision timestamps from SysTick
5. ✅ Optional file/line/function information
6. ✅ Core implementation < 2KB flash
7. ✅ Support printf-style and type-safe formatting
8. ✅ Multiple output sinks (UART, file, network)
9. ✅ Thread-safe on RTOS platforms
10. ✅ Examples for all common use cases

## References

**Inspiration from best logging libraries:**
- [Rust log crate](https://docs.rs/log/) - Simple, zero-cost abstraction
- [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging
- [log4cxx](https://logging.apache.org/log4cxx/) - Flexible configuration
- [ESP-IDF logging](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html) - Platform-specific approach

**CoreZero foundations:**
- `src/hal/interface/systick.hpp` - Timing source for timestamps
- `src/core/result.hpp` - Error handling pattern
- `src/core/types.hpp` - Common types
