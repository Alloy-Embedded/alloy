# Universal Logger System Design

## Context

Alloy needs a professional logging system that works across all supported platforms (bare-metal ARM, RTOS, ESP32 with ESP-IDF, host development) while maintaining the framework's core principles of zero-cost abstractions and minimal resource usage.

### Stakeholders
- **Embedded Developers**: Need efficient debugging without performance impact
- **IoT Developers**: Need runtime log control and remote logging capabilities
- **Framework Maintainers**: Need consistent logging across the entire codebase
- **Production Users**: Need reliable error reporting without debug overhead

### Constraints
- Must work on resource-constrained microcontrollers (32KB+ flash, 8KB+ RAM)
- Zero overhead for disabled log levels (compile-time elimination)
- No dynamic memory allocation in critical paths
- Thread-safe on RTOS platforms
- Platform-agnostic API (works everywhere)
- Integrate with existing SysTick for timestamps

## Goals / Non-Goals

### Goals
1. **Zero-Cost Abstraction**: Disabled logs completely removed at compile-time
2. **Universal API**: Single interface across all platforms
3. **Rich Context**: Timestamps, log levels, file/line info
4. **Flexible Output**: Multiple sinks (UART, file, network, custom)
5. **Easy to Use**: Simple macros for common cases
6. **Production Ready**: Thread-safe, efficient, battle-tested patterns

### Non-Goals
1. **Not a logging aggregation system**: Just local logging (for now)
2. **Not replacing ESP-IDF logging completely**: Can coexist and integrate
3. **Not supporting all format specifiers**: Focus on common use cases
4. **Not providing log rotation**: Sink responsibility (can be added later)

## Decisions

### Decision 1: Macro-Based API with Compile-Time Filtering

**Choice**: Use macros for logging with compile-time level filtering

**Rationale**:
- Macros capture `__FILE__`, `__LINE__`, `__FUNCTION__` automatically
- Compile-time filtering removes disabled logs (zero cost)
- Similar to Rust's `log!`, `info!`, `debug!` macros
- Familiar pattern (like `printf`, `ESP_LOGI`)

**Implementation**:
```cpp
// Compile-time minimum level (default INFO)
#ifndef LOG_MIN_LEVEL
#define LOG_MIN_LEVEL LOG_LEVEL_INFO
#endif

// Macros with compile-time filtering
#if LOG_MIN_LEVEL <= LOG_LEVEL_TRACE
    #define LOG_TRACE(fmt, ...) \
        ::logger::log(logger::Level::Trace, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_TRACE(fmt, ...) ((void)0)
#endif

// Usage
LOG_TRACE("Entering function");  // Removed if LOG_MIN_LEVEL > TRACE
LOG_INFO("Value: {}", value);    // Always present if LOG_MIN_LEVEL <= INFO
```

**Alternatives Considered**:
- **Function-based API**: Rejected - cannot capture file/line easily, no compile-time filtering
- **Stream-based API**: Rejected - too heavy for embedded (like `std::cout << "msg"`)
- **Format-only at runtime**: Rejected - misses zero-cost compile-time filtering

**Trade-offs**:
- **Pro**: Zero overhead for disabled logs
- **Pro**: Automatic context capture
- **Pro**: Familiar to embedded developers
- **Con**: Macros can be harder to debug
- **Con**: Need multiple macro definitions (but hidden in header)

### Decision 2: Sink-Based Output Architecture

**Choice**: Separate logging logic from output through "sinks"

**Rationale**:
- Decouples logging from output mechanism
- Allows multiple outputs simultaneously
- Easy to add custom sinks
- Inspired by spdlog's architecture

**Architecture**:
```
Application Code
      ↓
   LOG_INFO()
      ↓
Logger Core (formatting, filtering)
      ↓
   Sink Interface
      ↓
  ┌───┴────┬─────────┬──────────┐
  ↓        ↓         ↓          ↓
UART    File      ESP-LOG   Custom
Sink    Sink       Sink      Sink
```

**Pattern**:
```cpp
// Sink interface
class Sink {
public:
    virtual void write(const char* message, size_t length) = 0;
    virtual void flush() {}
};

// UART sink implementation
class UartSink : public Sink {
    void write(const char* message, size_t length) override {
        uart.write(message, length);
    }
};

// Usage
auto uart_sink = UartSink(uart0);
auto file_sink = FileSink("/logs/app.log");

Logger::add_sink(&uart_sink);
Logger::add_sink(&file_sink);  // Log to both
```

**Alternatives Considered**:
- **Direct output in logger**: Rejected - inflexible, hard to test
- **Callback-based**: Rejected - harder to manage multiple outputs
- **Stream-based**: Rejected - too heavy for embedded

### Decision 3: Static Allocation with Fixed-Size Buffers

**Choice**: Use static storage and fixed-size buffers, no dynamic allocation

**Rationale**:
- Predictable memory usage (critical for embedded)
- No heap fragmentation
- Deterministic behavior
- Safe for interrupt contexts (with proper design)

**Implementation**:
```cpp
class Logger {
private:
    static constexpr size_t MAX_MESSAGE_SIZE = 256;
    static constexpr size_t MAX_SINKS = 4;

    char message_buffer_[MAX_MESSAGE_SIZE];
    Sink* sinks_[MAX_SINKS];
    size_t sink_count_ = 0;
};
```

**Buffer sizes**:
- **Message buffer**: 256 bytes (adjustable via `LOG_MAX_MESSAGE_SIZE`)
- **Sink array**: 4 sinks (adjustable via `LOG_MAX_SINKS`)
- **Format buffer**: 128 bytes for timestamp/prefix

**Alternatives Considered**:
- **Dynamic allocation**: Rejected - not safe for embedded
- **User-provided buffers**: Rejected - too complex for simple use
- **Unbounded buffers**: Rejected - risk of overflow

**Trade-offs**:
- **Pro**: Predictable, safe, fast
- **Pro**: No malloc/free overhead
- **Pro**: Works in interrupt context (with care)
- **Con**: Limited message size (but 256 bytes is generous)
- **Con**: Limited number of sinks (but 4 is plenty)

### Decision 4: Two-Phase Formatting (Timestamp + Message)

**Choice**: Format timestamp prefix separately from user message

**Rationale**:
- Timestamp generation is platform-specific
- Message formatting is platform-agnostic
- Allows custom timestamp formats
- Enables colored output on capable terminals

**Format structure**:
```
[timestamp] LEVEL [source] message
```

**Example outputs**:
```
[0.123456] INFO  [main.cpp:42] Application started
[0.234567] WARN  [sensor.cpp:15] Temperature high: 85°C
[1.345678] ERROR [network.cpp:23] Connection failed: timeout
```

**Implementation**:
```cpp
// Phase 1: Format timestamp prefix
void format_prefix(char* buffer, size_t size,
                   uint32_t timestamp_us, Level level,
                   const char* file, int line) {
    snprintf(buffer, size,
             "[%lu.%06lu] %-5s [%s:%d] ",
             timestamp_us / 1000000,
             timestamp_us % 1000000,
             level_string(level),
             basename(file),
             line);
}

// Phase 2: Append user message
snprintf(buffer + prefix_len, size - prefix_len, fmt, args...);
```

**Alternatives Considered**:
- **Single-phase formatting**: Rejected - harder to customize
- **No prefix**: Rejected - loses valuable context
- **Separate timestamp line**: Rejected - wastes space

### Decision 5: Runtime Level Control with Static Default

**Choice**: Compile-time minimum level + runtime maximum level

**Rationale**:
- Compile-time removes disabled logs completely
- Runtime allows temporary debug without reflashing
- Best of both worlds

**Implementation**:
```cpp
class Logger {
public:
    // Set runtime level (can only increase filtering, not decrease)
    static void set_level(Level level) {
        if (level >= compile_time_min_level()) {
            runtime_level_ = level;
        }
    }

private:
    static constexpr Level compile_time_min_level() {
#if LOG_MIN_LEVEL == LOG_LEVEL_TRACE
        return Level::Trace;
#elif LOG_MIN_LEVEL == LOG_LEVEL_DEBUG
        return Level::Debug;
// ...
#endif
    }

    static Level runtime_level_;
};

// Usage
Logger::set_level(Level::Error);  // Only ERROR logs shown
// TRACE, DEBUG, INFO, WARN logs still removed at compile-time
```

**Alternatives Considered**:
- **Only compile-time**: Rejected - requires recompile to change levels
- **Only runtime**: Rejected - always pays cost even for disabled logs
- **Configuration file**: Rejected - too complex for embedded

### Decision 6: Thread-Safety via Lightweight Mutex

**Choice**: Use platform-specific lightweight synchronization

**Rationale**:
- RTOS platforms need thread safety
- Bare-metal can skip mutex (compile-time)
- Critical section approach for minimal overhead

**Implementation**:
```cpp
class Logger {
private:
#ifdef ALLOY_HAS_RTOS
    static alloy::rtos::Mutex mutex_;
#endif

public:
    static void log(Level level, const char* file, int line,
                   const char* fmt, ...) {
#ifdef ALLOY_HAS_RTOS
        alloy::rtos::MutexLock lock(mutex_);
#endif
        // Format and write...
    }
};
```

**Alternatives Considered**:
- **Lock-free queue**: Rejected - too complex for first version
- **Always use mutex**: Rejected - overhead on bare-metal
- **User-managed locking**: Rejected - error-prone

## Architecture Overview

### Component Diagram

```
┌─────────────────────────────────────────────────┐
│                Application Code                  │
└─────────────────┬───────────────────────────────┘
                  │ LOG_INFO(), LOG_ERROR(), etc.
                  ↓
┌─────────────────────────────────────────────────┐
│              Logger Core                         │
│  - Level filtering (compile + runtime)          │
│  - Message formatting                            │
│  - Timestamp integration                         │
│  - Thread safety (RTOS)                          │
└─────────────────┬───────────────────────────────┘
                  │ formatted message
                  ↓
┌─────────────────────────────────────────────────┐
│              Sink Interface                      │
│  - write(data, len)                             │
│  - flush()                                       │
└──────┬──────────┬──────────┬────────────────────┘
       │          │          │
       ↓          ↓          ↓
┌──────────┐ ┌─────────┐ ┌──────────┐
│ UartSink │ │FileSink │ │CustomSink│
└──────────┘ └─────────┘ └──────────┘
       │          │          │
       ↓          ↓          ↓
    UART       File I/O   User Code
```

### Class Structure

```cpp
namespace logger {

// Core classes
class Logger;           // Main logging engine
class Sink;            // Output interface
class Formatter;       // Message formatting

// Sink implementations
class UartSink;        // UART output
class FileSink;        // File output (FATFS, ESP-IDF)
class EspLogSink;      // ESP-IDF esp_log integration
class BufferSink;      // Memory buffer (for testing)

// Configuration
struct Config {
    Level default_level;
    bool enable_timestamps;
    bool enable_colors;
    bool enable_file_line;
};

} // namespace logger
```

## Risks / Trade-offs

### Risk 1: Buffer Overflow on Long Messages
**Risk**: Messages > 256 bytes get truncated

**Mitigation**:
- Document maximum message size
- Use `vsnprintf` to prevent buffer overrun
- Add compile-time configurable `LOG_MAX_MESSAGE_SIZE`
- Provide helper for multi-line logs

### Risk 2: Performance Impact on High-Frequency Logging
**Risk**: Logging in tight loops can slow execution

**Mitigation**:
- Compile-time filtering removes most logs
- Use LOG_TRACE for verbose logging (easily disabled)
- Document best practices (avoid logging in ISRs)
- Provide async sink for high-throughput scenarios (future)

### Risk 3: Platform-Specific Timestamp Availability
**Risk**: Some platforms might not have SysTick initialized

**Mitigation**:
- Graceful fallback to sequential counters
- Document SysTick requirement
- Add compile-time flag `LOG_ENABLE_TIMESTAMPS`
- Provide manual timestamp injection

### Risk 4: Flash/RAM Usage
**Risk**: Logger might be too large for smallest MCUs

**Mitigation**:
- Core implementation < 2KB flash
- Make features optional (`LOG_ENABLE_COLORS`, `LOG_ENABLE_FILE_LINE`)
- Provide minimal configuration
- Document resource usage per feature

**Estimated sizes**:
- Core logger: ~1.5KB flash, 512 bytes RAM
- Each sink: ~200-500 bytes flash
- Message buffer: 256 bytes RAM (configurable)
- Timestamp formatting: ~300 bytes flash

### Risk 5: Integration with Existing Code
**Risk**: Existing code uses different logging methods

**Mitigation**:
- Provide migration guide
- Support coexistence (e.g., ESP_LOG bridge)
- No breaking changes to existing APIs
- Gradual adoption path

## Migration Plan

### Phase 1: Core Implementation
- Logger core class
- Basic macros (LOG_INFO, LOG_ERROR, etc.)
- UART sink for bare-metal
- SysTick timestamp integration
- Unit tests

### Phase 2: Platform Adapters
- ESP-IDF sink (esp_log integration)
- File sink (for host/ESP32)
- RTOS thread safety
- Color output support

### Phase 3: Advanced Features
- Multiple sink support
- Custom format strings
- Log filtering by tag/component
- Performance benchmarks

### Phase 4: Examples and Documentation
- Basic usage example
- Multi-sink example
- Custom sink example
- Migration guide from printf/ESP_LOG
- Performance guide

## Open Questions

### Q1: Should we support structured logging (JSON)?
**Options**:
- A) Plain text only
- B) Optional JSON output
- C) Pluggable formatters

**Current thinking**: Option A for Phase 1, Option C for future. JSON adds complexity and size. Can be added later as a formatter plugin.

### Q2: How to handle multi-line log messages?
**Options**:
- A) Truncate to single line
- B) Split into multiple log calls
- C) Support explicit newlines

**Current thinking**: Option C. Allow `\n` in format string, sink handles it appropriately.

### Q3: Should we buffer logs for async output?
**Options**:
- A) Synchronous only (blocking)
- B) Optional async ring buffer
- C) User-managed buffering

**Current thinking**: Option A for Phase 1, Option B for future if needed. Async adds complexity.

### Q4: How to handle logs before SysTick initialization?
**Options**:
- A) No timestamps (print "BOOT")
- B) Use fallback counter
- C) Buffer and timestamp later

**Current thinking**: Option A. Simple, predictable. Most logs happen after init anyway.

## Success Metrics

1. **Performance**: <10 microseconds per log call (formatted, no I/O)
2. **Size**: Core < 2KB flash, <1KB RAM
3. **Compatibility**: Works on all Alloy platforms
4. **Usability**: 90% of logging needs covered by simple macros
5. **Adoption**: Used in 5+ examples within first month

## References

- [spdlog architecture](https://github.com/gabime/spdlog/wiki)
- [Rust log crate](https://docs.rs/log/latest/log/)
- [ESP-IDF logging guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html)
- [Embedded Artistry: Logging](https://embeddedartistry.com/blog/2018/08/20/implementing-a-better-logging-system/)
