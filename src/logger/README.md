# Alloy Universal Logger

Header-only logging core for embedded systems with:

- explicit logger instances for board or subsystem ownership
- lightweight non-owning sink registration via `SinkRef`
- legacy `Logger`/`LOG_*` compatibility for older code paths
- typed UART sinks that work directly with Alloy HAL UART handles

## Quick Start

```cpp
#include "logger/uart_logger.hpp"

auto uart = board::make_debug_uart();
static_cast<void>(uart.configure());

auto app_logger = alloy::logger::make_uart_logger(uart, {
    .default_level = alloy::logger::Level::Info,
    .enable_timestamps = false,
    .enable_source_location = true,
    .line_ending = alloy::logger::LineEnding::CRLF,
});

LOG_INFO_TO(app_logger, "Application started");
LOG_WARN_TO(app_logger, "Temperature: %.1f C", temp);
LOG_ERROR_TO(app_logger, "Connection failed");

// Output:
// [0.123456] INFO  [main.cpp:42] Application started
// [0.234567] WARN  [main.cpp:43] Temperature: 23.5°C
// [0.345678] ERROR [main.cpp:44] Connection failed
```

## Features

- ✅ **Zero-Cost**: Disabled logs completely removed at compile-time
- ✅ **Instance-first**: Explicit ownership instead of hidden global state
- ✅ **Universal**: Same sink and formatting path across bare-metal, RTOS, and host
- ✅ **Rich Context**: Microsecond timestamps, file/line, log levels
- ✅ **Flexible Output**: Multiple sinks via `SinkRef`, with or without inheritance
- ✅ **Record-Aware**: Sinks can consume `RecordView` instead of reparsing text
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
// Instance-local runtime policy
app_logger.set_level(Level::Error);  // Only show ERROR logs

// Change line endings per sink environment
app_logger.set_line_ending(LineEnding::CRLF);

// Provide timestamps explicitly when you want them
app_logger.set_timestamp_provider([]() -> std::uint64_t {
    return alloy::systick::micros();
});

// The legacy global wrapper still exists
Logger::set_level(Level::Warn);
Logger::flush();
```

## Available Sinks

### UART Sink (Bare-metal, RTOS, ESP32)

```cpp
#include "logger/uart_logger.hpp"

auto uart = board::make_debug_uart();
auto app_logger = logger::make_uart_logger(uart);
```

### Buffer Sink (Testing, Deferred Output)

```cpp
#include "logger/platform/buffer_sink.hpp"

char buffer[1024];
logger::BufferSink buffer_sink(buffer, sizeof(buffer));
Logger::add_sink(&buffer_sink);

// Later: read buffer_sink.data()
```

### Ring Buffer Sink (Fast Deferred Inspection)

```cpp
#include "logger/sinks/ring_buffer_sink.hpp"

logger::RingBufferSink<512, 16> ring_sink;
app_logger.add_sink(ring_sink);

LOG_WARN_TO(app_logger, "brownout margin low");

const auto latest = ring_sink.latest();
// latest.payload  -> "brownout margin low"
// latest.rendered -> full formatted line
```

### Async UART Logger (Deferred Drain)

```cpp
#include "logger/uart_logger.hpp"

auto uart = board::make_debug_uart();
auto async_logger =
    logger::make_async_uart_logger<decltype(uart), 1024, 32>(uart, {
        .default_level = logger::Level::Debug,
        .enable_timestamps = false,
    });

LOG_INFO_TO(async_logger, "boot");

// Drain later from the main loop or a low-priority task
const auto stats = async_logger.pump_all();
```

### Coming Soon

- richer record-oriented formatting policies
- asynchronous or deferred sinks for DMA/ring-buffer backends
- board-level helpers that wire debug UART logging in one call

## Creating Custom Sinks

```cpp
#include "logger/sink.hpp"

class MySink {
public:
    void write_record(const logger::RecordView& record) {
        trace_backend.push(record.level, record.timestamp_us, record.payload);
    }

    void write(std::string_view text) {
        // Write to your output device
        my_device.send(text.data(), text.size());
    }

    void flush() {
        // Optional: flush buffered data
        my_device.flush();
    }

    bool is_ready() const {
        // Optional: check if sink is ready
        return my_device.is_initialized();
    }
};

// Usage
MySink my_sink;
app_logger.add_sink(my_sink);
```

## Examples

### Basic Example (STM32F103)

Complete example: [`examples/logger_basic/`](../../examples/logger_basic/)

### Multiple Sinks

```cpp
logger::UartSink<decltype(uart)> uart_sink(uart);
char buffer[1024];
logger::BufferSink buffer_sink(buffer, sizeof(buffer));

app_logger.add_sink(uart_sink);
app_logger.add_sink(buffer_sink);

LOG_INFO_TO(app_logger, "Goes to both outputs");
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

1. Check sink is registered: `app_logger.add_sink(my_sink)`
2. Check sink is ready: `my_sink.is_ready()`
3. Check log level: `Logger::set_level(Level::Trace)`
4. Check compile-time level: `LOG_MIN_LEVEL`

### Timestamps Show "[BOOT]"

SysTick not initialized yet. Ensure `Board::initialize()` or `systick::init()` called first.

### Build Errors with Types

Include proper headers:
```cpp
#include "logger/logger.hpp"  // Core logger
#include "logger/sinks/uart_sink.hpp"  // If using UART sink
```

## Documentation

- **OpenSpec**: [`openspec/changes/add-universal-logger/`](../../openspec/changes/add-universal-logger/)
- **Design Document**: [`design.md`](../../openspec/changes/add-universal-logger/design.md)
- **Requirements**: [`specs/logger-core/spec.md`](../../openspec/changes/add-universal-logger/specs/logger-core/spec.md)
- **Implementation Summary**: [`IMPLEMENTATION_SUMMARY.md`](../../openspec/changes/add-universal-logger/IMPLEMENTATION_SUMMARY.md)

## License

Part of Alloy Framework - See LICENSE file
