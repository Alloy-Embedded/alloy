# Logger Core Specification

Platform-agnostic logging system with zero-cost compile-time filtering and runtime level control.

## ADDED Requirements

### Requirement: LOG-001 - Universal Logging API

The system SHALL provide a universal logging API that works consistently across all platforms (bare-metal, RTOS, ESP-IDF, host).

#### Scenario: Basic logging across platforms
```cpp
// Same code works on all platforms
#include "logger/logger.hpp"

void application() {
    LOG_INFO("Application starting");
    LOG_WARN("Low memory: {} KB free", get_free_memory());
    LOG_ERROR("Failed to connect: {}", error_message);
}
```

#### Scenario: Logging with multiple arguments
```cpp
LOG_INFO("Sensor reading: temp={}°C, humidity={}%", temp, humidity);
LOG_DEBUG("Buffer: addr={:x}, size={}", ptr, size);
```

---

### Requirement: LOG-002 - Log Levels

The system SHALL support five log levels with clear semantics.

**Levels** (in order of severity):
1. **TRACE**: Very detailed diagnostic information
2. **DEBUG**: Debug information useful during development
3. **INFO**: Informational messages about normal operation
4. **WARN**: Warning messages about potential issues
5. **ERROR**: Error messages about failures

#### Scenario: Using different log levels
```cpp
LOG_TRACE("Entering parse_packet()");
LOG_DEBUG("Parsed header: type={}, len={}", type, len);
LOG_INFO("Connection established to {}", server_addr);
LOG_WARN("Retry count high: {}/3", retry_count);
LOG_ERROR("Packet validation failed: checksum mismatch");
```

---

### Requirement: LOG-003 - Compile-Time Filtering

The system SHALL eliminate disabled log levels at compile-time with zero runtime overhead.

**Configuration**: `LOG_MIN_LEVEL` defines minimum compile-time level.

#### Scenario: Compile-time optimization
```cpp
// Configuration in build system
#define LOG_MIN_LEVEL LOG_LEVEL_INFO

// In application code
LOG_TRACE("This is removed at compile-time");  // Not in binary
LOG_DEBUG("This is removed at compile-time");  // Not in binary
LOG_INFO("This is included");                   // In binary
LOG_ERROR("This is included");                  // In binary
```

**Verification**:
- Binary size with `LOG_MIN_LEVEL=INFO` < binary size with `LOG_MIN_LEVEL=TRACE`
- Disassembly shows no trace of disabled log calls

---

### Requirement: LOG-004 - Runtime Level Control

The system SHALL allow runtime adjustment of log level without recompilation.

**Constraint**: Runtime level can only increase filtering (cannot enable compile-time disabled logs).

#### Scenario: Dynamic level adjustment
```cpp
// At startup (default: INFO)
Logger::set_level(Level::Debug);  // Show DEBUG and above

// During operation
Logger::set_level(Level::Error);  // Only show ERROR logs

// Cannot show logs below compile-time minimum
// If LOG_MIN_LEVEL=INFO, cannot enable TRACE/DEBUG at runtime
```

---

### Requirement: LOG-005 - Timestamp Integration

The system SHALL automatically include microsecond-precision timestamps from SysTick.

**Format**: `[seconds.microseconds]`

#### Scenario: Automatic timestamps
```cpp
LOG_INFO("First message");
delay_ms(100);
LOG_INFO("Second message");

// Output:
// [0.000123] INFO  [main.cpp:10] First message
// [0.100456] INFO  [main.cpp:12] Second message
```

#### Scenario: Timestamps before SysTick initialization
```cpp
// Before systick::init()
LOG_INFO("Bootloader starting");

// Output uses fallback:
// [BOOT] INFO  [boot.cpp:5] Bootloader starting
```

---

### Requirement: LOG-006 - Source Location Context

The system SHALL optionally include file, line, and function information.

**Configuration**: `LOG_ENABLE_SOURCE_LOCATION` (default: enabled)

#### Scenario: With source location
```cpp
#define LOG_ENABLE_SOURCE_LOCATION 1

void connect_to_server() {
    LOG_INFO("Connecting...");
}

// Output:
// [1.234567] INFO  [network.cpp:42] Connecting...
```

#### Scenario: Without source location (minimal mode)
```cpp
#define LOG_ENABLE_SOURCE_LOCATION 0

LOG_INFO("Connecting...");

// Output:
// [1.234567] INFO  Connecting...
```

---

### Requirement: LOG-007 - Printf-Style Formatting

The system SHALL support printf-style format specifiers for compatibility.

**Supported specifiers**: `%d`, `%u`, `%x`, `%s`, `%f`, `%p`, `%%`

#### Scenario: Printf-style formatting
```cpp
int value = 42;
float temperature = 23.5f;
const char* name = "sensor1";

LOG_INFO("Value: %d, Temp: %.1f°C, Name: %s", value, temperature, name);

// Output:
// [0.123456] INFO  [main.cpp:10] Value: 42, Temp: 23.5°C, Name: sensor1
```

---

### Requirement: LOG-008 - Type-Safe Formatting

The system SHALL support type-safe `{}` placeholders (fmt-style).

**Format**: `{}` for automatic type deduction, `{:format}` for custom format.

#### Scenario: Type-safe formatting
```cpp
int value = 42;
float temp = 23.5f;
std::string name = "sensor1";

LOG_INFO("Value: {}, Temp: {:.1f}°C, Name: {}", value, temp, name);

// Output:
// [0.123456] INFO  [main.cpp:10] Value: 42, Temp: 23.5°C, Name: sensor1
```

#### Scenario: Hexadecimal formatting
```cpp
uint32_t addr = 0xDEADBEEF;
LOG_DEBUG("Address: 0x{:08x}", addr);

// Output:
// [0.123456] DEBUG [main.cpp:15] Address: 0xdeadbeef
```

---

### Requirement: LOG-009 - Sink Management

The system SHALL support multiple output sinks simultaneously.

**Maximum sinks**: Configurable via `LOG_MAX_SINKS` (default: 4)

#### Scenario: Multiple output destinations
```cpp
// Setup sinks
UartSink uart_sink(uart0);
FileSink file_sink("/logs/app.log");

Logger::add_sink(&uart_sink);
Logger::add_sink(&file_sink);

// Log goes to both UART and file
LOG_INFO("Message to both outputs");

// Remove sink
Logger::remove_sink(&file_sink);
```

---

### Requirement: LOG-010 - Thread Safety (RTOS)

The system SHALL provide thread-safe logging on RTOS platforms.

**Implementation**: Lightweight mutex protecting logger state

#### Scenario: Concurrent logging from multiple tasks
```cpp
// Task 1
void sensor_task(void*) {
    while (true) {
        LOG_INFO("Temperature: {}°C", read_temp());
        delay(1000);
    }
}

// Task 2
void network_task(void*) {
    while (true) {
        LOG_INFO("Packets: {}", packet_count);
        delay(500);
    }
}

// Logs are properly interleaved, not corrupted
```

---

### Requirement: LOG-011 - Bare-Metal Safety

The system SHALL work safely on bare-metal (no RTOS) with zero synchronization overhead.

**Implementation**: Mutex code is compile-time removed on bare-metal

#### Scenario: Bare-metal logging
```cpp
// On bare-metal platform (no ALLOY_HAS_RTOS)
int main() {
    LOG_INFO("Bare-metal app starting");  // No mutex overhead
    while (1) {
        LOG_DEBUG("Loop iteration");
    }
}
```

---

### Requirement: LOG-012 - Minimal Resource Usage

The system SHALL have minimal memory footprint suitable for resource-constrained MCUs.

**Targets**:
- Core logger: < 2KB flash
- Message buffer: 256 bytes RAM (configurable)
- Per-sink overhead: < 500 bytes flash

#### Scenario: Configuration for small MCUs
```cpp
// Minimal configuration for 32KB flash MCUs
#define LOG_MIN_LEVEL LOG_LEVEL_INFO
#define LOG_MAX_MESSAGE_SIZE 128
#define LOG_MAX_SINKS 2
#define LOG_ENABLE_COLORS 0
#define LOG_ENABLE_SOURCE_LOCATION 0

// Estimated usage: ~1KB flash, 128 bytes RAM
```

---

### Requirement: LOG-013 - Color Output Support

The system SHALL optionally support ANSI color codes for terminal output.

**Configuration**: `LOG_ENABLE_COLORS` (default: disabled)

#### Scenario: Color-coded log levels
```cpp
#define LOG_ENABLE_COLORS 1

LOG_TRACE("Trace message");  // Gray
LOG_DEBUG("Debug message");  // Cyan
LOG_INFO("Info message");    // Green
LOG_WARN("Warning message"); // Yellow
LOG_ERROR("Error message");  // Red

// UART output includes ANSI escape codes for color
// File output strips colors automatically
```

---

### Requirement: LOG-014 - Custom Format Strings

The system SHALL allow customization of log message format.

**Default format**: `[timestamp] LEVEL [file:line] message`

#### Scenario: Custom format configuration
```cpp
// Minimal format
Logger::set_format("[%T] %L: %M");

LOG_INFO("Test");
// Output: [0.123456] INFO: Test

// Detailed format
Logger::set_format("[%T] %L [%S:%L %F] %M");

LOG_INFO("Test");
// Output: [0.123456] INFO [main.cpp:10 main()] Test
```

**Format placeholders**:
- `%T` - Timestamp
- `%L` - Level
- `%S` - Source file
- `%N` - Line number
- `%F` - Function name
- `%M` - Message

---

### Requirement: LOG-015 - Flush Support

The system SHALL provide explicit flush control for buffered sinks.

#### Scenario: Ensuring logs are written
```cpp
LOG_INFO("Critical operation starting");
Logger::flush();  // Ensure log is written before operation

perform_critical_operation();

LOG_INFO("Critical operation complete");
Logger::flush();
```

---

### Requirement: LOG-016 - Tag/Component Filtering (Future)

The system SHOULD support filtering by tag/component (deferred to Phase 2).

#### Scenario: Component-specific logging
```cpp
LOG_INFO_TAG("network", "Connected to {}", addr);
LOG_WARN_TAG("sensor", "Calibration needed");

// Runtime filtering
Logger::set_level_for_tag("network", Level::Debug);
Logger::set_level_for_tag("sensor", Level::Error);
```

---

## Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `LOG_MIN_LEVEL` | enum | `LOG_LEVEL_INFO` | Compile-time minimum level |
| `LOG_MAX_MESSAGE_SIZE` | size | 256 | Maximum message length (bytes) |
| `LOG_MAX_SINKS` | size | 4 | Maximum number of sinks |
| `LOG_ENABLE_TIMESTAMPS` | bool | 1 | Include timestamps |
| `LOG_ENABLE_COLORS` | bool | 0 | ANSI color output |
| `LOG_ENABLE_SOURCE_LOCATION` | bool | 1 | Include file/line info |
| `LOG_TIMESTAMP_PRECISION` | enum | `MICROS` | Timestamp resolution |

## API Reference

### Logging Macros

```cpp
LOG_TRACE(fmt, ...)   // Trace level
LOG_DEBUG(fmt, ...)   // Debug level
LOG_INFO(fmt, ...)    // Info level
LOG_WARN(fmt, ...)    // Warning level
LOG_ERROR(fmt, ...)   // Error level
```

### Logger Class

```cpp
class Logger {
public:
    // Configuration
    static void set_level(Level level);
    static Level get_level();
    static void set_format(const char* format);

    // Sink management
    static void add_sink(Sink* sink);
    static void remove_sink(Sink* sink);
    static void remove_all_sinks();

    // Output control
    static void flush();
    static void enable_colors(bool enable);

    // Core logging (usually called via macros)
    static void log(Level level, const char* file, int line,
                   const char* function, const char* fmt, ...);
};
```

## Performance Characteristics

| Operation | Time | Notes |
|-----------|------|-------|
| Disabled log (compile-time) | 0 cycles | Completely removed |
| Disabled log (runtime) | ~10 cycles | Level check only |
| Enabled log (no sink) | ~50 µs | Format only |
| Enabled log (UART sink) | ~50 µs + I/O time | Depends on baud rate |
| Enabled log (file sink) | ~50 µs + write time | Depends on storage speed |
| `Logger::flush()` | Varies | Depends on sinks |

## Related Specifications

- `logger-platform` - Platform-specific sink implementations
- `systick` - Timestamp source (modified to add milliseconds helper)
