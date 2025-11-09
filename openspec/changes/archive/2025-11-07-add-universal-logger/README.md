# Universal Logger System - OpenSpec

## Quick Summary

A professional, zero-cost, platform-agnostic logging system for Alloy Framework.

### Key Features
- ✅ **Zero-Cost**: Disabled logs completely removed at compile-time
- ✅ **Universal**: Single API across bare-metal, RTOS, ESP-IDF, host
- ✅ **Rich Context**: Microsecond timestamps, file/line, log levels
- ✅ **Flexible Output**: Multiple sinks (UART, file, network, custom)
- ✅ **Minimal**: < 2KB flash, < 1KB RAM for core
- ✅ **Thread-Safe**: Proper synchronization on RTOS

### Example Usage

```cpp
#include "logger/logger.hpp"

// Simple logging
LOG_INFO("Application started");
LOG_WARN("Temperature high: {}°C", temp);
LOG_ERROR("Connection failed: {}", error);

// Output:
// [0.123456] INFO  [main.cpp:10] Application started
// [0.234567] WARN  [main.cpp:11] Temperature high: 85°C
// [0.345678] ERROR [main.cpp:12] Connection failed: timeout
```

### Configuration

```cpp
// Compile-time minimum level (removes disabled logs)
#define LOG_MIN_LEVEL LOG_LEVEL_INFO

// Maximum message size
#define LOG_MAX_MESSAGE_SIZE 256

// Enable/disable features
#define LOG_ENABLE_COLORS 1
#define LOG_ENABLE_SOURCE_LOCATION 1
```

## Files in this OpenSpec

- **`proposal.md`** - High-level motivation and impact analysis
- **`design.md`** - Architecture decisions and trade-offs
- **`tasks.md`** - Ordered implementation tasks
- **`specs/logger-core/spec.md`** - Core logging system requirements
- **`specs/logger-platform/spec.md`** - Platform-specific sink requirements

## Implementation Phases

| Phase | Description | Effort |
|-------|-------------|--------|
| 1 | Core Logger System | 8-12 hours |
| 2 | Platform Sinks | 6-8 hours |
| 3 | Advanced Features | 8-10 hours |
| 4 | Examples & Testing | 6-8 hours |
| 5 | Documentation | 4-6 hours |

**Total**: ~1 week full-time

## Platform Support

| Platform | UART | File | ESP-LOG | Network | Colors |
|----------|------|------|---------|---------|--------|
| Bare-metal | ✅ | ⚠️ SD | ❌ | ❌ | ❌ |
| RTOS | ✅ | ⚠️ SD | ❌ | ⚠️ | ❌ |
| ESP-IDF | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| Host | ❌ | ✅ | ❌ | ✅ | ✅ |

## Design Highlights

### Compile-Time Filtering
```cpp
#define LOG_MIN_LEVEL LOG_LEVEL_INFO

LOG_TRACE("Removed at compile-time");  // Not in binary
LOG_INFO("Included in binary");         // In binary
```

### Multiple Sinks
```cpp
UartSink uart_sink(uart0);
FileSink file_sink("/logs/app.log");

Logger::add_sink(&uart_sink);
Logger::add_sink(&file_sink);  // Logs to both
```

### Thread-Safe (RTOS)
```cpp
// Task 1
LOG_INFO("From task 1");

// Task 2
LOG_INFO("From task 2");

// Outputs are properly synchronized, not corrupted
```

## Resource Usage

**Core Logger**:
- Flash: ~1.5 KB
- RAM: ~512 bytes (+ message buffer)

**With All Features**:
- Flash: ~3 KB
- RAM: ~1 KB (+ 256 byte message buffer)

**Per Sink**:
- UART: ~200 bytes flash
- File: ~500 bytes flash
- Buffer: ~300 bytes flash + buffer size

## Related Work

Inspired by best logging libraries:
- [Rust log crate](https://docs.rs/log/) - Zero-cost abstraction model
- [spdlog](https://github.com/gabime/spdlog) - Sink architecture
- [ESP-IDF logging](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html) - Platform integration
- [log4cxx](https://logging.apache.org/log4cxx/) - Configuration flexibility

## Status

**OpenSpec Status**: ✅ READY FOR REVIEW

**Next Steps**:
1. Review and approve OpenSpec
2. Begin Phase 1 implementation (Core Logger System)
3. Test on target platforms
4. Create examples
5. Deploy to production

## Questions/Feedback

Please review:
- Architecture decisions in `design.md`
- Requirements in `specs/*/spec.md`
- Implementation plan in `tasks.md`

Contact: [OpenSpec discussion]
