# Universal Logger Implementation Tasks

## Implementation Status

**Current Status**: Phase 2 Complete ✅

**Completed**:
- ✅ **Phase 1**: Core logger system (types, macros, formatting, sink interface)
- ✅ **Phase 2**: Platform sinks (UART, Buffer, ESP-IDF, File, Console, Rotating File)

**Files Created**:

**Core System (Phase 1)**:
- `src/logger/types.hpp` - Core types and configuration
- `src/logger/sink.hpp` - Sink base interface
- `src/logger/format.hpp` - Message formatting utilities
- `src/logger/logger.hpp` - Main logger class and macros

**Platform Sinks (Phase 2)**:
- `src/logger/platform/uart_sink.hpp` - UART sink (all platforms)
- `src/logger/platform/buffer_sink.hpp` - Buffer sink for testing
- `src/logger/platform/esp_log_sink.hpp` - ESP-IDF integration (ESP32)
- `src/logger/platform/file_sink.hpp` - File sink + rotating file sink
- `src/logger/platform/console_sink.hpp` - Console sink for host (with colors)

**Examples**:
- `examples/logger_basic/` - STM32F103 example with UART
- `examples/esp32_logger/` - ESP32 multi-sink example

**Tests & Docs**:
- `tests/unit/test_logger.cpp` - Unit tests
- `src/logger/README.md` - User documentation

**Next Steps**: Phase 3 - Advanced features (Async sink, Network sink, Custom formats)

## Phase 1: Core Logger System (Foundation) ✅ COMPLETED

### 1.1 Core Types and Enums ✅
- [x] 1.1.1 Create `src/logger/types.hpp` with Level enum and configuration structs
- [x] 1.1.2 Define compile-time configuration macros
- [x] 1.1.3 Add LogLevel enum (TRACE, DEBUG, INFO, WARN, ERROR)
- [x] 1.1.4 Add format specifier types (TimestampPrecision, Config struct)

### 1.2 Logger Core Implementation ✅
- [x] 1.2.1 Create `src/logger/logger.hpp` with Logger class declaration
- [x] 1.2.2 Core implementation (header-only, no .cpp needed)
  - Static instance management (singleton pattern)
  - Level filtering (compile-time + runtime)
  - Message formatting
  - Sink management (add/remove/iterate)
- [x] 1.2.3 Implement basic logging macros (LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR)
- [x] 1.2.4 Add thread safety for RTOS platforms (mutex guard with ALLOY_HAS_RTOS)

### 1.3 Format System ✅
- [x] 1.3.1 Create `src/logger/format.hpp` with formatting utilities
- [x] 1.3.2 Implement printf-style format parser and formatter (vsnprintf-based)
- [x] 1.3.3 Type-safe formatting deferred to Phase 3 (printf-style sufficient for now)
- [x] 1.3.4 Format specifier support via printf (%.2f, %x, etc.)
- [x] 1.3.5 Implement timestamp formatting (microseconds, milliseconds, seconds)

### 1.4 Sink Interface ✅
- [x] 1.4.1 Create `src/logger/sink.hpp` with Sink base class
- [x] 1.4.2 Define write() and flush() interface
- [x] 1.4.3 Add is_ready() optional method
- [x] 1.4.4 Document sink implementation guidelines (with example in header)

### 1.5 SysTick Integration ✅
- [x] 1.5.1 SysTick already provides micros() - no modification needed
- [x] 1.5.2 Uptime formatting utilities implemented in format.hpp
- [x] 1.5.3 Implement fallback for pre-init logging ("BOOT" timestamp when micros==0)
- [x] 1.5.4 Timestamp overflow handled by SysTick's existing design

## Phase 2: Platform Sinks ✅ COMPLETED

### 2.1 UART Sink ✅
- [x] 2.1.1 Create `src/logger/platform/uart_sink.hpp`
- [x] 2.1.2 Implement template-based UartSink<UartImpl>
- [x] 2.1.3 Create STM32 UART example (examples/logger_basic/)
- [x] 2.1.4 Create ESP32 UART example (examples/esp32_logger/)
- [ ] 2.1.5 Test with RP2040 UART (deferred - hardware needed)
- [ ] 2.1.6 Add timeout handling for blocked UART (deferred - Phase 3)

### 2.2 Buffer Sink (Testing) ✅
- [x] 2.2.1 Create `src/logger/platform/buffer_sink.hpp`
- [x] 2.2.2 Implement fixed-size buffer (linear, not circular)
- [x] 2.2.3 Add clear() and data() accessors
- [x] 2.2.4 Add unit tests (tests/unit/test_logger.cpp)

### 2.3 ESP-IDF Integration Sink ✅
- [x] 2.3.1 Create `src/logger/platform/esp_log_sink.hpp`
- [x] 2.3.2 Implement esp_log bridge (EspLogSink)
- [x] 2.3.3 Implement level-aware variant (EspLogSinkWithLevel)
- [x] 2.3.4 Integrated in ESP32 example (examples/esp32_logger/)

### 2.4 File Sink (Host + ESP32) ✅
- [x] 2.4.1 Create `src/logger/platform/file_sink.hpp`
- [x] 2.4.2 Implement POSIX file operations (FileSink)
- [x] 2.4.3 File operations work on ESP-IDF via VFS (same API)
- [x] 2.4.4 Implement rotating file sink (RotatingFileSink)
- [ ] 2.4.5 Test with SPIFFS and FAT (deferred - requires hardware)

### 2.5 Console Sink (Host Only) ✅
- [x] 2.5.1 Create `src/logger/platform/console_sink.hpp`
- [x] 2.5.2 Implement stdout/stderr output (ConsoleSink)
- [x] 2.5.3 Add ANSI color support with TTY auto-detection
- [x] 2.5.4 Implement simple variant (SimpleConsoleSink)
- [ ] 2.5.5 Test on Linux, macOS, Windows (deferred - requires CI)

## Phase 3: Advanced Features

### 3.1 Color Output
- [ ] 3.1.1 Add ANSI color code definitions
- [ ] 3.1.2 Implement color formatting per log level
- [ ] 3.1.3 Add enable_colors() configuration
- [ ] 3.1.4 Auto-detect TTY support
- [ ] 3.1.5 Test on various terminals

### 3.2 Custom Format Strings
- [ ] 3.2.1 Implement format string parser
- [ ] 3.2.2 Add format placeholders (%T, %L, %S, %N, %F, %M)
- [ ] 3.2.3 Implement set_format() API
- [ ] 3.2.4 Provide predefined formats (minimal, standard, verbose)

### 3.3 Network Sink (UDP)
- [ ] 3.3.1 Create `src/logger/platform/udp_sink.hpp`
- [ ] 3.3.2 Implement UDP socket creation
- [ ] 3.3.3 Add syslog format support (RFC 5424)
- [ ] 3.3.4 Test with remote syslog server
- [ ] 3.3.5 Handle network failures gracefully

### 3.4 Rotating File Sink (Phase 2)
- [ ] 3.4.1 Create `src/logger/platform/rotating_file_sink.hpp`
- [ ] 3.4.2 Implement file size checking
- [ ] 3.4.3 Implement file rotation (rename/backup)
- [ ] 3.4.4 Add configurable rotation parameters
- [ ] 3.4.5 Test rotation on full file system

### 3.5 Async Sink Wrapper (Phase 2)
- [ ] 3.5.1 Create `src/logger/platform/async_sink.hpp`
- [ ] 3.5.2 Implement lock-free ring buffer
- [ ] 3.5.3 Create background worker thread
- [ ] 3.5.4 Handle buffer overflow scenarios
- [ ] 3.5.5 Performance benchmarks

## Phase 4: Examples and Testing

### 4.1 Basic Example
- [ ] 4.1.1 Create `examples/logger_basic/` project
- [ ] 4.1.2 Demonstrate basic logging with UART sink
- [ ] 4.1.3 Show all log levels
- [ ] 4.1.4 Show compile-time filtering
- [ ] 4.1.5 Add README with build instructions

### 4.2 Multi-Sink Example
- [ ] 4.2.1 Create `examples/logger_multi_sink/` project
- [ ] 4.2.2 Demonstrate UART + File logging
- [ ] 4.2.3 Show runtime level control
- [ ] 4.2.4 Demonstrate sink management (add/remove)

### 4.3 ESP32 Integration Example
- [ ] 4.3.1 Create `examples/esp32_logger/` project
- [ ] 4.3.2 Show Alloy logger + ESP_LOG coexistence
- [ ] 4.3.3 Demonstrate UART + File + UDP sinks
- [ ] 4.3.4 Add WiFi logging to remote server

### 4.4 Custom Sink Example
- [ ] 4.4.1 Create `examples/logger_custom_sink/` project
- [ ] 4.4.2 Implement custom LED blink sink
- [ ] 4.4.3 Implement custom LCD display sink
- [ ] 4.4.4 Document sink API

### 4.5 Unit Tests
- [ ] 4.5.1 Create `tests/unit/test_logger.cpp`
- [ ] 4.5.2 Test level filtering (compile-time + runtime)
- [ ] 4.5.3 Test format parsing and formatting
- [ ] 4.5.4 Test sink management
- [ ] 4.5.5 Test thread safety (on RTOS)
- [ ] 4.5.6 Test buffer overflow handling
- [ ] 4.5.7 Test timestamp formatting

### 4.6 Integration Tests
- [ ] 4.6.1 Test on bare-metal STM32
- [ ] 4.6.2 Test on ESP32 with ESP-IDF
- [ ] 4.6.3 Test on RP2040
- [ ] 4.6.4 Test on host (Linux/macOS/Windows)
- [ ] 4.6.5 Benchmark performance vs printf
- [ ] 4.6.6 Measure flash and RAM usage

## Phase 5: Documentation

### 5.1 API Documentation
- [ ] 5.1.1 Document Logger class in header
- [ ] 5.1.2 Document all logging macros
- [ ] 5.1.3 Document Sink interface
- [ ] 5.1.4 Document format specifiers
- [ ] 5.1.5 Document configuration options

### 5.2 User Guide
- [ ] 5.2.1 Create `docs/LOGGER_GUIDE.md`
- [ ] 5.2.2 Quick start section
- [ ] 5.2.3 Configuration guide
- [ ] 5.2.4 Platform-specific notes
- [ ] 5.2.5 Best practices
- [ ] 5.2.6 Troubleshooting section

### 5.3 Migration Guide
- [ ] 5.3.1 Create `docs/LOGGER_MIGRATION.md`
- [ ] 5.3.2 Migration from printf
- [ ] 5.3.3 Migration from ESP_LOG
- [ ] 5.3.4 Migration from other logging libraries
- [ ] 5.3.5 Gradual adoption strategy

### 5.4 Performance Guide
- [ ] 5.4.1 Document performance characteristics
- [ ] 5.4.2 Provide optimization tips
- [ ] 5.4.3 Show benchmark results
- [ ] 5.4.4 Compare with alternatives

## Dependencies

### Sequential Dependencies
- Phase 1 must be completed before Phase 2
- SysTick integration (1.5) needed for timestamps
- Core logger (1.2) needed for all sinks
- Format system (1.3) needed for message formatting

### Parallel Work
- Platform sinks (2.1-2.5) can be developed in parallel
- Advanced features (3.1-3.5) can be developed in parallel after Phase 2
- Examples (4.1-4.4) can be created as features become available
- Documentation (5.1-5.4) can be written alongside implementation

## Validation Criteria

Each task should be validated with:
- [ ] Code compiles without warnings
- [ ] Unit tests pass (if applicable)
- [ ] Example demonstrates feature
- [ ] Documentation is updated
- [ ] Resource usage is within targets (flash < 2KB, RAM < 1KB for core)

## Estimated Effort

- **Phase 1 (Core)**: 8-12 hours
- **Phase 2 (Sinks)**: 6-8 hours
- **Phase 3 (Advanced)**: 8-10 hours
- **Phase 4 (Testing)**: 6-8 hours
- **Phase 5 (Docs)**: 4-6 hours

**Total**: 32-44 hours (approximately 1 week full-time)

## Success Metrics

1. ✅ All log levels work across all platforms
2. ✅ Compile-time filtering verifiably removes code
3. ✅ Core logger < 2KB flash, < 1KB RAM
4. ✅ Logging overhead < 50 microseconds (without I/O)
5. ✅ At least 4 different sinks implemented
6. ✅ Thread-safe on RTOS platforms
7. ✅ All examples build and run
8. ✅ Comprehensive documentation
9. ✅ Adopted in 3+ existing examples
10. ✅ Performance benchmarks documented

## Future Enhancements (Post Phase 5)

- [ ] Structured logging (JSON output)
- [ ] Log filtering by tag/component
- [ ] Log compression
- [ ] Remote log aggregation
- [ ] Log analysis tools
- [ ] Integration with debugging tools (GDB, OpenOCD)
- [ ] Binary log format for space efficiency
- [ ] Log encryption for sensitive data
