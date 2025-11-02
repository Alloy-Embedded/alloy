# CoreZero Universal Logger

Professional, zero-cost, platform-agnostic logging system for embedded systems.

## Quick Start

```cpp
#include "logger/logger.hpp"
#include "logger/platform/uart_sink.hpp"

// Create and register a sink
logger::UartSink<Uart1> uart_sink;
logger::Logger::add_sink(&uart_sink);

// Log messages
LOG_INFO("Application started");
LOG_WARN("Temperature: %.1f°C", temp);
LOG_ERROR("Connection failed");

// Output:
// [0.123456] INFO  [main.cpp:42] Application started
// [0.234567] WARN  [main.cpp:43] Temperature: 23.5°C
// [0.345678] ERROR [main.cpp:44] Connection failed
```

## Features

- ✅ **Zero-Cost**: Disabled logs completely removed at compile-time
- ✅ **Universal**: Single API across bare-metal, RTOS, ESP-IDF, host
- ✅ **Rich Context**: Microsecond timestamps, file/line, log levels
- ✅ **Flexible Output**: Multiple sinks (UART, file, network, custom)
- ✅ **Minimal**: < 2KB flash, < 1KB RAM
- ✅ **Thread-Safe**: Automatic synchronization on RTOS

## Log Levels

```cpp
LOG_TRACE("Very detailed diagnostic");  // Development only
LOG_DEBUG("Debug information");         // Development
LOG_INFO("Normal operation");           // Production
LOG_WARN("Potential issue");            // Production
LOG_ERROR("Something failed");          // Production
```

## Configuration

### Compile-Time (CMakeLists.txt)

```cmake
# Set minimum log level (removes lower levels from binary)
target_compile_definitions(my_target PRIVATE
    LOG_MIN_LEVEL=LOG_LEVEL_INFO  # Only INFO and above in binary
)

# Configure features
target_compile_definitions(my_target PRIVATE
    LOG_MAX_MESSAGE_SIZE=256      # Message buffer size
    LOG_ENABLE_COLORS=0           # Disable ANSI colors
    LOG_ENABLE_SOURCE_LOCATION=1  # Include file:line
    LOG_ENABLE_TIMESTAMPS=1       # Include timestamps
)
```

### Runtime

```cpp
// Change log level dynamically
Logger::set_level(Level::Error);  // Only show ERROR logs

// Enable colors (if compiled with LOG_ENABLE_COLORS=1)
Logger::enable_colors(true);

// Flush all sinks
Logger::flush();
```

## Available Sinks

### UART Sink (Bare-metal, RTOS, ESP32)

```cpp
#include "logger/platform/uart_sink.hpp"

logger::UartSink<Uart1> uart_sink;
Logger::add_sink(&uart_sink);
```

### Buffer Sink (Testing, Deferred Output)

```cpp
#include "logger/platform/buffer_sink.hpp"

char buffer[1024];
logger::BufferSink buffer_sink(buffer, sizeof(buffer));
Logger::add_sink(&buffer_sink);

// Later: read buffer_sink.data()
```

### Coming Soon

- **ESP-IDF Sink**: Bridge to `ESP_LOG` system
- **File Sink**: Log to SD card or filesystem
- **Console Sink**: Color output for host development
- **Network Sink**: UDP/syslog remote logging

## Creating Custom Sinks

```cpp
#include "logger/sink.hpp"

class MySink : public logger::Sink {
public:
    void write(const char* data, size_t length) override {
        // Write to your output device
        my_device.send(data, length);
    }

    void flush() override {
        // Optional: flush buffered data
        my_device.flush();
    }

    bool is_ready() const override {
        // Optional: check if sink is ready
        return my_device.is_initialized();
    }
};

// Usage
MySink my_sink;
Logger::add_sink(&my_sink);
```

## Examples

### Basic Example (STM32F103)

Complete example: [`examples/logger_basic/`](../../examples/logger_basic/)

### Multiple Sinks

```cpp
logger::UartSink<Uart1> uart_sink;
char buffer[1024];
logger::BufferSink buffer_sink(buffer, sizeof(buffer));

Logger::add_sink(&uart_sink);   // Log to UART
Logger::add_sink(&buffer_sink);  // Also save to buffer

LOG_INFO("Goes to both outputs");
```

### Conditional Logging

```cpp
#ifdef DEBUG_MODE
    LOG_DEBUG("Debug build info: %s", build_info);
#endif

if (temperature > THRESHOLD) {
    LOG_WARN("Temperature high: %.1f°C", temperature);
}
```

## Resource Usage

| Component | Flash | RAM | Notes |
|-----------|-------|-----|-------|
| Core Logger | ~1.5 KB | 512 bytes | Singleton + static data |
| Message Buffer | - | 256 bytes | Configurable |
| UART Sink | ~200 bytes | - | Template instantiation |
| Buffer Sink | ~300 bytes | Variable | + buffer storage |
| **Total** | **~2 KB** | **~784 bytes** | + sink buffers |

## Performance

| Operation | Time | Notes |
|-----------|------|-------|
| Disabled log (compile-time) | 0 cycles | Removed from binary |
| Disabled log (runtime) | ~10 cycles | Level check only |
| Enabled log (format only) | ~50 µs | @ 72 MHz Cortex-M3 |
| UART output @ 115200 | +~0.9 ms | Per byte transmission |

## Thread Safety

### RTOS Platforms

Automatically thread-safe when `ALLOY_HAS_RTOS` is defined:

```cpp
// Task 1
LOG_INFO("From task 1");

// Task 2
LOG_INFO("From task 2");

// Outputs are properly synchronized
```

### Bare-Metal

Zero synchronization overhead - no mutex code included.

## Integration

### With SysTick

Logger automatically uses `alloy::systick::micros()` for timestamps.

No configuration needed - works out of the box!

### With RTOS

Logger detects RTOS via `ALLOY_HAS_RTOS` and adds mutex protection automatically.

### With ESP-IDF

Coming soon: ESP-IDF sink will bridge to `ESP_LOG` system.

## Best Practices

### 1. Set Appropriate Compile-Time Level

```cmake
# Development build
target_compile_definitions(dev_target PRIVATE LOG_MIN_LEVEL=LOG_LEVEL_TRACE)

# Production build
target_compile_definitions(prod_target PRIVATE LOG_MIN_LEVEL=LOG_LEVEL_INFO)
```

### 2. Use Levels Appropriately

- **TRACE**: Very detailed, temporary debugging (remove after fixing bug)
- **DEBUG**: Development information (keep for troubleshooting)
- **INFO**: Important events (keep minimal in production)
- **WARN**: Potential issues (always investigate)
- **ERROR**: Failures (always fix)

### 3. Avoid Logging in ISRs

Logging can be slow (especially with UART). Prefer to set flags in ISRs and log in main task.

```cpp
// ❌ Bad
void timer_isr() {
    LOG_INFO("Timer tick");  // Too slow!
}

// ✅ Good
volatile bool timer_flag = false;
void timer_isr() {
    timer_flag = true;  // Fast flag set
}

void main_task() {
    if (timer_flag) {
        LOG_INFO("Timer event");
        timer_flag = false;
    }
}
```

### 4. Use Flush Before Critical Operations

```cpp
LOG_ERROR("About to reset system");
Logger::flush();  // Ensure log is sent
system_reset();
```

## Troubleshooting

### Logs Not Appearing

1. Check sink is registered: `Logger::add_sink(&my_sink)`
2. Check sink is ready: `my_sink.is_ready()`
3. Check log level: `Logger::set_level(Level::Trace)`
4. Check compile-time level: `LOG_MIN_LEVEL`

### Timestamps Show "[BOOT]"

SysTick not initialized yet. Ensure `Board::initialize()` or `systick::init()` called first.

### Build Errors with Types

Include proper headers:
```cpp
#include "logger/logger.hpp"  // Core logger
#include "logger/platform/uart_sink.hpp"  // If using UART sink
```

## Documentation

- **OpenSpec**: [`openspec/changes/add-universal-logger/`](../../openspec/changes/add-universal-logger/)
- **Design Document**: [`design.md`](../../openspec/changes/add-universal-logger/design.md)
- **Requirements**: [`specs/logger-core/spec.md`](../../openspec/changes/add-universal-logger/specs/logger-core/spec.md)
- **Implementation Summary**: [`IMPLEMENTATION_SUMMARY.md`](../../openspec/changes/add-universal-logger/IMPLEMENTATION_SUMMARY.md)

## License

Part of CoreZero Framework - See LICENSE file
