#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "types.hpp"

namespace alloy {
namespace logger {

/**
 * Format utilities for logger messages
 */
class Formatter {
public:
    /**
     * Format timestamp into buffer
     *
     * @param buffer Output buffer
     * @param size Buffer size
     * @param timestamp_us Timestamp in microseconds
     * @param precision Timestamp precision
     * @return Number of characters written
     */
    static size_t format_timestamp(char* buffer, size_t size,
                                   uint64_t timestamp_us,
                                   TimestampPrecision precision) {
        if (timestamp_us == 0) {
            // Before SysTick initialization
            return snprintf(buffer, size, "[BOOT] ");
        }

        switch (precision) {
            case TimestampPrecision::Seconds: {
                uint32_t seconds = static_cast<uint32_t>(timestamp_us / 1000000);
                return snprintf(buffer, size, "[%lu] ", seconds);
            }

            case TimestampPrecision::Milliseconds: {
                uint32_t seconds = static_cast<uint32_t>(timestamp_us / 1000000);
                uint32_t millis = static_cast<uint32_t>((timestamp_us % 1000000) / 1000);
                return snprintf(buffer, size, "[%lu.%03lu] ", seconds, millis);
            }

            case TimestampPrecision::Microseconds: {
                uint32_t seconds = static_cast<uint32_t>(timestamp_us / 1000000);
                uint32_t micros = static_cast<uint32_t>(timestamp_us % 1000000);
                return snprintf(buffer, size, "[%lu.%06lu] ", seconds, micros);
            }

            default:
                return snprintf(buffer, size, "[?] ");
        }
    }

    /**
     * Format log level into buffer
     *
     * @param buffer Output buffer
     * @param size Buffer size
     * @param level Log level
     * @param use_colors Whether to use ANSI color codes
     * @return Number of characters written
     */
    static size_t format_level(char* buffer, size_t size, Level level, bool use_colors) {
        if (use_colors) {
            const char* color = get_level_color(level);
            const char* reset = "\033[0m";
            return snprintf(buffer, size, "%s%-5s%s ", color, level_to_string(level), reset);
        } else {
            return snprintf(buffer, size, "%-5s ", level_to_string(level));
        }
    }

    /**
     * Format source location into buffer
     *
     * @param buffer Output buffer
     * @param size Buffer size
     * @param file Source file path
     * @param line Line number
     * @return Number of characters written
     */
    static size_t format_source_location(char* buffer, size_t size,
                                         const char* file, int line) {
        // Extract just filename from path
        const char* filename = get_filename(file);
        return snprintf(buffer, size, "[%s:%d] ", filename, line);
    }

    /**
     * Format complete log message prefix
     *
     * @param buffer Output buffer
     * @param size Buffer size
     * @param timestamp_us Timestamp in microseconds
     * @param level Log level
     * @param file Source file (can be nullptr)
     * @param line Line number
     * @param config Logger configuration
     * @return Number of characters written
     */
    static size_t format_prefix(char* buffer, size_t size,
                                uint64_t timestamp_us, Level level,
                                const char* file, int line,
                                const Config& config) {
        size_t pos = 0;

        // Timestamp
        if (config.enable_timestamps && pos < size) {
            pos += format_timestamp(buffer + pos, size - pos,
                                   timestamp_us, config.timestamp_precision);
        }

        // Log level
        if (pos < size) {
            pos += format_level(buffer + pos, size - pos, level, config.enable_colors);
        }

        // Source location
        if (config.enable_source_location && file != nullptr && pos < size) {
            pos += format_source_location(buffer + pos, size - pos, file, line);
        }

        return pos;
    }

    /**
     * Format message with printf-style arguments
     *
     * @param buffer Output buffer
     * @param size Buffer size
     * @param fmt Format string
     * @param args Variable arguments
     * @return Number of characters written
     */
    static size_t format_message(char* buffer, size_t size,
                                 const char* fmt, va_list args) {
        return vsnprintf(buffer, size, fmt, args);
    }

private:
    /**
     * Get ANSI color code for log level
     */
    static const char* get_level_color(Level level) {
        switch (level) {
            case Level::Trace: return "\033[90m";  // Gray
            case Level::Debug: return "\033[36m";  // Cyan
            case Level::Info:  return "\033[32m";  // Green
            case Level::Warn:  return "\033[33m";  // Yellow
            case Level::Error: return "\033[31m";  // Red
            default:           return "\033[0m";   // Reset
        }
    }

    /**
     * Extract filename from full path
     */
    static const char* get_filename(const char* path) {
        if (path == nullptr) return "";

        const char* filename = path;
        for (const char* p = path; *p != '\0'; ++p) {
            if (*p == '/' || *p == '\\') {
                filename = p + 1;
            }
        }
        return filename;
    }
};

} // namespace logger
} // namespace alloy
