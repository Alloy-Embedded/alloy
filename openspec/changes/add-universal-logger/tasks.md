# Universal Logger Implementation Tasks

## Phase 1: Core Logger System (Foundation)

### 1.1 Core Types and Enums
- [ ] 1.1.1 Create `src/logger/types.hpp` with Level enum and configuration structs
- [ ] 1.1.2 Define compile-time configuration macros
- [ ] 1.1.3 Add LogLevel enum (TRACE, DEBUG, INFO, WARN, ERROR)
- [ ] 1.1.4 Add format specifier types

### 1.2 Logger Core Implementation
- [ ] 1.2.1 Create `src/logger/logger.hpp` with Logger class declaration
- [ ] 1.2.2 Create `src/logger/logger.cpp` with core implementation
  - Static instance management
  - Level filtering (compile-time + runtime)
  - Message formatting
  - Sink management (add/remove/iterate)
- [ ] 1.2.3 Implement basic logging macros (LOG_INFO, LOG_ERROR, etc.)
- [ ] 1.2.4 Add thread safety for RTOS platforms (mutex guard)

### 1.3 Format System
- [ ] 1.3.1 Create `src/logger/format.hpp` with formatting utilities
- [ ] 1.3.2 Implement printf-style format parser and formatter
- [ ] 1.3.3 Implement type-safe `{}` formatter (basic version)
- [ ] 1.3.4 Add format specifier support (`:x`, `:.2f`, etc.)
- [ ] 1.3.5 Implement timestamp formatting (microseconds, milliseconds, seconds)

### 1.4 Sink Interface
- [ ] 1.4.1 Create `src/logger/sink.hpp` with Sink base class
- [ ] 1.4.2 Define write() and flush() interface
- [ ] 1.4.3 Add is_ready() optional method
- [ ] 1.4.4 Document sink implementation guidelines

### 1.5 SysTick Integration
- [ ] 1.5.1 Modify `src/hal/interface/systick.hpp` to add millis() helper
- [ ] 1.5.2 Add uptime formatting utilities
- [ ] 1.5.3 Implement fallback for pre-init logging ("BOOT" timestamp)
- [ ] 1.5.4 Test timestamp overflow handling (32-bit microseconds)

## Phase 2: Platform Sinks

### 2.1 UART Sink
- [ ] 2.1.1 Create `src/logger/platform/uart_sink.hpp`
- [ ] 2.1.2 Implement template-based UartSink<UartImpl>
- [ ] 2.1.3 Test with STM32 UART
- [ ] 2.1.4 Test with ESP32 UART
- [ ] 2.1.5 Test with RP2040 UART
- [ ] 2.1.6 Add timeout handling for blocked UART

### 2.2 Buffer Sink (Testing)
- [ ] 2.2.1 Create `src/logger/platform/buffer_sink.hpp`
- [ ] 2.2.2 Implement fixed-size circular buffer
- [ ] 2.2.3 Add clear() and data() accessors
- [ ] 2.2.4 Add unit tests

### 2.3 ESP-IDF Integration Sink
- [ ] 2.3.1 Create `src/logger/platform/esp_log_sink.hpp`
- [ ] 2.3.2 Implement esp_log bridge
- [ ] 2.3.3 Map CoreZero levels to ESP-IDF levels
- [ ] 2.3.4 Test integration with ESP-IDF logging infrastructure

### 2.4 File Sink (Host + ESP32)
- [ ] 2.4.1 Create `src/logger/platform/file_sink.hpp`
- [ ] 2.4.2 Implement POSIX file operations (for host)
- [ ] 2.4.3 Implement ESP-IDF VFS operations (for ESP32)
- [ ] 2.4.4 Add write buffering
- [ ] 2.4.5 Test with SPIFFS and FAT file systems

### 2.5 Console Sink (Host Only)
- [ ] 2.5.1 Create `src/logger/platform/console_sink.hpp`
- [ ] 2.5.2 Implement stdout/stderr output
- [ ] 2.5.3 Add ANSI color support with auto-detection
- [ ] 2.5.4 Test on Linux, macOS, Windows

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
- [ ] 4.3.2 Show CoreZero logger + ESP_LOG coexistence
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
