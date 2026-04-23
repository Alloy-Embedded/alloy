#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

// ============================================================================
// Compile-Time Configuration Macros
// ============================================================================

/**
 * Minimum compile-time log level
 * Logs below this level are completely removed from the binary
 *
 * Define before including logger.hpp to customize:
 * #define LOG_MIN_LEVEL LOG_LEVEL_INFO
 */
#ifndef LOG_MIN_LEVEL
    #define LOG_MIN_LEVEL LOG_LEVEL_INFO
#endif

/**
 * Log level constants for compile-time filtering
 */
#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_OFF   5

/**
 * Maximum message size (bytes)
 * Total buffer = prefix (128) + message (LOG_MAX_MESSAGE_SIZE)
 */
#ifndef LOG_MAX_MESSAGE_SIZE
    #define LOG_MAX_MESSAGE_SIZE 256
#endif

/**
 * Maximum number of sinks that can be registered
 */
#ifndef LOG_MAX_SINKS
    #define LOG_MAX_SINKS 4
#endif

/**
 * Enable/disable color output
 * Colors use ANSI escape codes
 */
#ifndef LOG_ENABLE_COLORS
    #define LOG_ENABLE_COLORS 0
#endif

/**
 * Enable/disable source location (file:line)
 * Disabling saves flash space
 */
#ifndef LOG_ENABLE_SOURCE_LOCATION
    #define LOG_ENABLE_SOURCE_LOCATION 1
#endif

/**
 * Enable/disable timestamps
 * Requires SysTick to be initialized
 */
#ifndef LOG_ENABLE_TIMESTAMPS
    #define LOG_ENABLE_TIMESTAMPS 1
#endif

/**
 * Timestamp precision
 * 0 = seconds, 1 = milliseconds, 2 = microseconds
 */
#ifndef LOG_TIMESTAMP_PRECISION
    #define LOG_TIMESTAMP_PRECISION 2
#endif

namespace alloy::logger {

/**
 * Log severity levels (in order of increasing severity)
 */
enum class Level : std::uint8_t {
    Trace = 0,  // Very detailed diagnostic information
    Debug = 1,  // Debug information useful during development
    Info = 2,   // Informational messages about normal operation
    Warn = 3,   // Warning messages about potential issues
    Error = 4,  // Error messages about failures
};

/**
 * Convert log level to string representation
 */
inline auto level_to_string(Level level) -> const char* {
    switch (level) {
        case Level::Trace:
            return "TRACE";
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

/**
 * Convert log level to padded string representation used by the text formatter.
 */
inline auto level_to_short_string(Level level) -> const char* {
    switch (level) {
        case Level::Trace:
            return "TRACE";
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO ";
        case Level::Warn:
            return "WARN ";
        case Level::Error:
            return "ERROR";
        default:
            return "?????";
    }
}

/**
 * Timestamp precision configuration.
 */
enum class TimestampPrecision : std::uint8_t {
    Seconds,
    Milliseconds,
    Microseconds,
};

/**
 * Line ending policy for text sinks.
 */
enum class LineEnding : std::uint8_t {
    None,
    LF,
    CRLF,
};

/**
 * Lightweight source-location payload used by instance-based loggers.
 */
struct SourceLocation {
    const char* file = nullptr;
    int line = 0;
};

/**
 * Immutable view over a single log emission.
 *
 * `payload` is the message body only. `rendered` is the fully formatted text
 * that was emitted by the logger, including prefix and line ending.
 */
struct RecordView {
    Level level = Level::Info;
    std::uint64_t timestamp_us = 0u;
    SourceLocation source_location{};
    std::string_view payload{};
    std::string_view rendered{};
};

using TimestampProvider = std::uint64_t (*)();

/**
 * Logger configuration structure.
 *
 * The logger stays text-first for now, but configuration is instance-local so
 * each board or subsystem can tune timestamps, colors, and line endings
 * independently.
 */
struct Config {
    Level default_level = Level::Info;
    bool enable_timestamps = (LOG_ENABLE_TIMESTAMPS != 0);
    bool enable_colors = (LOG_ENABLE_COLORS != 0);
    bool enable_source_location = (LOG_ENABLE_SOURCE_LOCATION != 0);
    TimestampPrecision timestamp_precision =
        (LOG_TIMESTAMP_PRECISION == 0)
            ? TimestampPrecision::Seconds
            : ((LOG_TIMESTAMP_PRECISION == 1) ? TimestampPrecision::Milliseconds
                                              : TimestampPrecision::Microseconds);
    LineEnding line_ending = LineEnding::LF;
    TimestampProvider timestamp_provider = nullptr;
};

}  // namespace alloy::logger
