# Logger Platform Adapters Specification

Platform-specific logging sinks and adapters for Alloy logger.

## ADDED Requirements

### Requirement: LOGPLAT-001 - Sink Interface

The system SHALL define a clean sink interface for custom output implementations.

#### Scenario: Implementing a custom sink
```cpp
class MySink : public logger::Sink {
public:
    void write(const char* data, size_t length) override {
        // Write log data to custom output
        my_output_device.send(data, length);
    }

    void flush() override {
        // Optional: flush buffered data
        my_output_device.flush();
    }
};

// Usage
MySink my_sink;
Logger::add_sink(&my_sink);
```

---

### Requirement: LOGPLAT-002 - UART Sink

The system SHALL provide a UART sink for bare-metal and RTOS platforms.

**Platforms**: All Alloy platforms with UART support

#### Scenario: UART logging on bare-metal
```cpp
#include "logger/platform/uart_sink.hpp"
#include "hal/stm32/uart.hpp"

using Uart = alloy::hal::stm32::Uart<USART1>;
Uart debug_uart;

void setup() {
    debug_uart.init(/* pins */);
    debug_uart.configure(UartConfig{115200});

    // Create UART sink
    logger::UartSink<Uart> uart_sink(debug_uart);
    Logger::add_sink(&uart_sink);

    LOG_INFO("UART logging initialized");
}
```

#### Scenario: UART logging with ESP-IDF
```cpp
#include "logger/platform/uart_sink.hpp"
#include "hal/esp32/uart.hpp"

using namespace alloy::hal::esp32;

Uart0 console;

void app_main() {
    console.init(GPIO_NUM_1, GPIO_NUM_3);
    console.configure(UartConfig{115200});

    logger::UartSink uart_sink(console);
    Logger::add_sink(&uart_sink);

    LOG_INFO("ESP32 UART logging ready");
}
```

---

### Requirement: LOGPLAT-003 - ESP-IDF Integration Sink

The system SHALL provide a sink that integrates with ESP-IDF's `esp_log` system.

**Purpose**: Allow Alloy logger to coexist with ESP-IDF logging infrastructure

#### Scenario: Bridge to ESP_LOG
```cpp
#include "logger/platform/esp_log_sink.hpp"

logger::EspLogSink esp_sink;
Logger::add_sink(&esp_sink);

// Alloy logs appear in ESP-IDF log system
LOG_INFO("From Alloy logger");

// Visible via:
// - idf.py monitor
// - ESP-IDF log viewer
// - Remote logging (if configured)
```

#### Scenario: Level mapping
```cpp
// Alloy levels map to ESP-IDF levels:
LOG_TRACE() -> ESP_LOGV()  // Verbose
LOG_DEBUG() -> ESP_LOGD()  // Debug
LOG_INFO()  -> ESP_LOGI()  // Info
LOG_WARN()  -> ESP_LOGW()  // Warning
LOG_ERROR() -> ESP_LOGE()  // Error
```

---

### Requirement: LOGPLAT-004 - File Sink

The system SHALL provide a file sink for platforms with file system support.

**Platforms**: Host, ESP32 (with SPIFFS/FAT), embedded with SD card

#### Scenario: File logging on host
```cpp
#include "logger/platform/file_sink.hpp"

logger::FileSink file_sink("/tmp/app.log");
Logger::add_sink(&file_sink);

LOG_INFO("Logs written to file");
LOG_ERROR("Errors also logged");

// File contents:
// [0.001234] INFO  [main.cpp:10] Logs written to file
// [0.002345] ERROR [main.cpp:11] Errors also logged
```

#### Scenario: File logging on ESP32 with SPIFFS
```cpp
#include "logger/platform/file_sink.hpp"
#include "esp_spiffs.h"

void app_main() {
    // Mount SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
    };
    esp_vfs_spiffs_register(&conf);

    // Create file sink
    logger::FileSink file_sink("/spiffs/logs/app.log");
    Logger::add_sink(&file_sink);

    LOG_INFO("File logging on ESP32");
}
```

---

### Requirement: LOGPLAT-005 - Buffer Sink

The system SHALL provide a memory buffer sink for testing and deferred output.

**Use cases**: Unit testing, log capture, batch transmission

#### Scenario: Testing with buffer sink
```cpp
#include "logger/platform/buffer_sink.hpp"

// Create 4KB buffer
logger::BufferSink buffer_sink(4096);
Logger::add_sink(&buffer_sink);

LOG_INFO("Test message 1");
LOG_ERROR("Test message 2");

// Read buffer contents
const char* contents = buffer_sink.data();
size_t length = buffer_sink.size();

// Verify logs in tests
assert(strstr(contents, "Test message 1") != nullptr);
assert(strstr(contents, "Test message 2") != nullptr);

// Clear buffer
buffer_sink.clear();
```

#### Scenario: Deferred network transmission
```cpp
logger::BufferSink buffer_sink(8192);
Logger::add_sink(&buffer_sink);

// Log events
LOG_INFO("Event 1");
LOG_INFO("Event 2");
LOG_WARN("Event 3");

// Periodically send to server
void send_logs_to_server() {
    if (buffer_sink.size() > 0) {
        http_post("/api/logs", buffer_sink.data(), buffer_sink.size());
        buffer_sink.clear();
    }
}
```

---

### Requirement: LOGPLAT-006 - Console Sink (Host Only)

The system SHALL provide a console sink with color support for host development.

**Platform**: Host only (uses stdout/stderr)

#### Scenario: Host development with colors
```cpp
#include "logger/platform/console_sink.hpp"

logger::ConsoleSink console_sink(true);  // Enable colors
Logger::add_sink(&console_sink);

LOG_INFO("Info in green");
LOG_WARN("Warning in yellow");
LOG_ERROR("Error in red");

// Colors automatically disabled if:
// - Not a TTY (piped to file)
// - Windows console without ANSI support
// - Explicitly disabled
```

---

### Requirement: LOGPLAT-007 - Network Sink (UDP)

The system SHALL provide a UDP network sink for remote logging.

**Use case**: IoT devices sending logs to central server

#### Scenario: UDP logging to syslog server
```cpp
#include "logger/platform/udp_sink.hpp"

// Create UDP sink pointing to syslog server
logger::UdpSink udp_sink("192.168.1.100", 514);  // Standard syslog port
Logger::add_sink(&udp_sink);

LOG_INFO("Log sent over network");

// UDP packet sent to server:
// <134>1 0.123456 esp32 app - INFO [main.cpp:10] Log sent over network
```

---

### Requirement: LOGPLAT-008 - Rotating File Sink

The system SHOULD provide a rotating file sink (Phase 2).

**Features**: Maximum file size, backup rotation, old file deletion

#### Scenario: Rotating logs
```cpp
#include "logger/platform/rotating_file_sink.hpp"

// 5 files, 1MB each = 5MB total
logger::RotatingFileSink file_sink("/logs/app.log", 1024 * 1024, 5);
Logger::add_sink(&file_sink);

LOG_INFO("Logs automatically rotated");

// Creates:
// /logs/app.log      (current)
// /logs/app.log.1    (previous)
// /logs/app.log.2    (older)
// ...
// /logs/app.log.4    (oldest)
```

---

### Requirement: LOGPLAT-009 - Async Sink Wrapper

The system SHOULD provide async wrapper for high-throughput logging (Phase 2).

**Use case**: Logging from time-critical tasks without blocking

#### Scenario: Async logging
```cpp
#include "logger/platform/async_sink.hpp"

logger::UartSink uart_sink(uart0);

// Wrap in async sink with 1KB ring buffer
logger::AsyncSink async_uart(uart_sink, 1024);
Logger::add_sink(&async_uart);

// Logging returns immediately, output happens in background
LOG_INFO("Non-blocking log");  // Returns in microseconds
```

---

## Platform Support Matrix

| Sink | Bare-Metal | RTOS | ESP-IDF | Host |
|------|-----------|------|---------|------|
| UartSink | ✅ | ✅ | ✅ | ❌ |
| EspLogSink | ❌ | ❌ | ✅ | ❌ |
| FileSink | ⚠️ (SD) | ⚠️ (SD) | ✅ | ✅ |
| BufferSink | ✅ | ✅ | ✅ | ✅ |
| ConsoleSink | ❌ | ❌ | ❌ | ✅ |
| UdpSink | ❌ | ⚠️ (TCP/IP) | ✅ | ✅ |
| RotatingSink | ❌ | ⚠️ (SD) | ✅ | ✅ |
| AsyncSink | ❌ | ✅ | ✅ | ✅ |

Legend:
- ✅ Fully supported
- ⚠️ Supported with additional hardware/software
- ❌ Not applicable

## Sink API

### Base Sink Interface

```cpp
namespace logger {

class Sink {
public:
    virtual ~Sink() = default;

    // Write formatted log message
    virtual void write(const char* data, size_t length) = 0;

    // Flush buffered data (optional)
    virtual void flush() {}

    // Check if sink is ready (optional)
    virtual bool is_ready() const { return true; }
};

} // namespace logger
```

### UART Sink

```cpp
template<typename UartImpl>
class UartSink : public Sink {
public:
    explicit UartSink(UartImpl& uart);

    void write(const char* data, size_t length) override;
    void flush() override;

private:
    UartImpl& uart_;
};
```

### File Sink

```cpp
class FileSink : public Sink {
public:
    explicit FileSink(const char* filename);
    ~FileSink();

    void write(const char* data, size_t length) override;
    void flush() override;
    bool is_ready() const override;

    // File-specific operations
    void close();
    size_t size() const;

private:
    int fd_;
    const char* filename_;
};
```

### Buffer Sink

```cpp
class BufferSink : public Sink {
public:
    explicit BufferSink(size_t capacity);
    ~BufferSink();

    void write(const char* data, size_t length) override;

    // Buffer access
    const char* data() const;
    size_t size() const;
    void clear();

private:
    char* buffer_;
    size_t capacity_;
    size_t size_;
};
```

## Performance Characteristics

| Sink | Write Latency | Throughput | Buffer Size |
|------|--------------|------------|-------------|
| UartSink | ~10 µs + UART | ~115 KB/s @ 115200 | Hardware FIFO |
| FileSink (host) | ~100 µs | ~1 MB/s | OS buffered |
| FileSink (ESP32) | ~500 µs | ~100 KB/s | VFS buffered |
| BufferSink | ~5 µs | Memory speed | User defined |
| EspLogSink | ~20 µs | Depends on ESP_LOG | ESP-IDF buffer |
| UdpSink | ~200 µs + network | ~1 MB/s | Network MTU |

## Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `UART_SINK_TIMEOUT_MS` | uint32 | 1000 | UART write timeout |
| `FILE_SINK_BUFFER_SIZE` | size | 512 | File write buffer |
| `BUFFER_SINK_MAX_SIZE` | size | 8192 | Maximum buffer size |
| `UDP_SINK_MTU` | size | 1400 | Maximum UDP packet size |

## Related Specifications

- `logger-core` - Core logging system
- `uart` - UART HAL interface (for UartSink)
- `filesystem` - File system support (for FileSink)
