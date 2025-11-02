#pragma once

#include "types.hpp"
#include "sink.hpp"
#include "format.hpp"
#include "hal/interface/systick.hpp"

#ifdef ALLOY_HAS_RTOS
#include "rtos/mutex.hpp"
#endif

#include <cstdarg>

namespace alloy {
namespace logger {

/**
 * Core logging engine
 *
 * Provides centralized logging with:
 * - Compile-time log level filtering (zero-cost for disabled logs)
 * - Runtime level control
 * - Multiple output sinks
 * - Automatic timestamps from SysTick
 * - Thread-safe operation (on RTOS platforms)
 *
 * Usage:
 *   Logger::add_sink(&uart_sink);
 *   LOG_INFO("Application started");
 *   LOG_WARN("Temperature: {}°C", temp);
 */
class Logger {
public:
    /**
     * Get singleton instance
     */
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    /**
     * Set runtime log level
     *
     * Only logs at or above this level will be output.
     * Cannot enable logs below compile-time minimum (LOG_MIN_LEVEL).
     *
     * @param level New minimum level
     */
    static void set_level(Level level) {
        instance().runtime_level_ = level;
    }

    /**
     * Get current runtime log level
     */
    static Level get_level() {
        return instance().runtime_level_;
    }

    /**
     * Add an output sink
     *
     * @param sink Pointer to sink (must remain valid)
     * @return true if added, false if max sinks reached
     */
    static bool add_sink(Sink* sink) {
        auto& inst = instance();
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(inst.mutex_);
#endif
        if (inst.sink_count_ >= LOG_MAX_SINKS) {
            return false;
        }
        inst.sinks_[inst.sink_count_++] = sink;
        return true;
    }

    /**
     * Remove a sink
     *
     * @param sink Pointer to sink to remove
     */
    static void remove_sink(Sink* sink) {
        auto& inst = instance();
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(inst.mutex_);
#endif
        for (size_t i = 0; i < inst.sink_count_; ++i) {
            if (inst.sinks_[i] == sink) {
                // Shift remaining sinks down
                for (size_t j = i; j < inst.sink_count_ - 1; ++j) {
                    inst.sinks_[j] = inst.sinks_[j + 1];
                }
                --inst.sink_count_;
                break;
            }
        }
    }

    /**
     * Remove all sinks
     */
    static void remove_all_sinks() {
        auto& inst = instance();
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(inst.mutex_);
#endif
        inst.sink_count_ = 0;
    }

    /**
     * Flush all sinks
     */
    static void flush() {
        auto& inst = instance();
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(inst.mutex_);
#endif
        for (size_t i = 0; i < inst.sink_count_; ++i) {
            inst.sinks_[i]->flush();
        }
    }

    /**
     * Configure logger
     *
     * @param config Configuration settings
     */
    static void configure(const Config& config) {
        instance().config_ = config;
        instance().runtime_level_ = config.default_level;
    }

    /**
     * Enable or disable color output
     *
     * @param enable true to enable colors
     */
    static void enable_colors(bool enable) {
        instance().config_.enable_colors = enable;
    }

    /**
     * Core logging function (usually called via macros)
     *
     * @param level Log level
     * @param file Source file name
     * @param line Source line number
     * @param fmt Printf-style format string
     * @param ... Format arguments
     */
    static void log(Level level, const char* file, int line, const char* fmt, ...) {
        auto& inst = instance();

        // Runtime level check
        if (level < inst.runtime_level_) {
            return;
        }

#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(inst.mutex_);
#endif

        // Skip if no sinks
        if (inst.sink_count_ == 0) {
            return;
        }

        // Get timestamp
        uint64_t timestamp_us = 0;
#if LOG_ENABLE_TIMESTAMPS
        timestamp_us = static_cast<uint64_t>(alloy::systick::micros());
#endif

        // Format prefix
        size_t prefix_len = Formatter::format_prefix(
            inst.buffer_,
            sizeof(inst.buffer_),
            timestamp_us,
            level,
            file,
            line,
            inst.config_
        );

        // Format message
        va_list args;
        va_start(args, fmt);
        size_t msg_len = Formatter::format_message(
            inst.buffer_ + prefix_len,
            sizeof(inst.buffer_) - prefix_len - 1,  // Leave room for newline
            fmt,
            args
        );
        va_end(args);

        // Add newline
        size_t total_len = prefix_len + msg_len;
        if (total_len < sizeof(inst.buffer_) - 1) {
            inst.buffer_[total_len++] = '\n';
            inst.buffer_[total_len] = '\0';
        }

        // Write to all sinks
        for (size_t i = 0; i < inst.sink_count_; ++i) {
            if (inst.sinks_[i]->is_ready()) {
                inst.sinks_[i]->write(inst.buffer_, total_len);
            }
        }
    }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Configuration
    Config config_;
    Level runtime_level_ = Level::Info;

    // Sinks
    Sink* sinks_[LOG_MAX_SINKS] = {nullptr};
    size_t sink_count_ = 0;

    // Message buffer
    char buffer_[128 + LOG_MAX_MESSAGE_SIZE];  // Prefix + message

#ifdef ALLOY_HAS_RTOS
    rtos::Mutex mutex_;
#endif
};

} // namespace logger
} // namespace alloy

// ============================================================================
// Logging Macros
// ============================================================================

/**
 * Helper macro to strip file path down to just filename
 */
#define LOG_FILENAME(file) (strrchr(file, '/') ? strrchr(file, '/') + 1 : \
                           (strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file))

/**
 * TRACE level logging - very detailed diagnostic information
 * Removed at compile-time if LOG_MIN_LEVEL > LOG_LEVEL_TRACE
 */
#if LOG_MIN_LEVEL <= LOG_LEVEL_TRACE
    #if LOG_ENABLE_SOURCE_LOCATION
        #define LOG_TRACE(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Trace, \
                                        LOG_FILENAME(__FILE__), __LINE__, \
                                        fmt, ##__VA_ARGS__)
    #else
        #define LOG_TRACE(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Trace, \
                                        nullptr, 0, fmt, ##__VA_ARGS__)
    #endif
#else
    #define LOG_TRACE(fmt, ...) ((void)0)
#endif

/**
 * DEBUG level logging - debug information useful during development
 * Removed at compile-time if LOG_MIN_LEVEL > LOG_LEVEL_DEBUG
 */
#if LOG_MIN_LEVEL <= LOG_LEVEL_DEBUG
    #if LOG_ENABLE_SOURCE_LOCATION
        #define LOG_DEBUG(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Debug, \
                                        LOG_FILENAME(__FILE__), __LINE__, \
                                        fmt, ##__VA_ARGS__)
    #else
        #define LOG_DEBUG(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Debug, \
                                        nullptr, 0, fmt, ##__VA_ARGS__)
    #endif
#else
    #define LOG_DEBUG(fmt, ...) ((void)0)
#endif

/**
 * INFO level logging - informational messages about normal operation
 * Removed at compile-time if LOG_MIN_LEVEL > LOG_LEVEL_INFO
 */
#if LOG_MIN_LEVEL <= LOG_LEVEL_INFO
    #if LOG_ENABLE_SOURCE_LOCATION
        #define LOG_INFO(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Info, \
                                        LOG_FILENAME(__FILE__), __LINE__, \
                                        fmt, ##__VA_ARGS__)
    #else
        #define LOG_INFO(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Info, \
                                        nullptr, 0, fmt, ##__VA_ARGS__)
    #endif
#else
    #define LOG_INFO(fmt, ...) ((void)0)
#endif

/**
 * WARN level logging - warning messages about potential issues
 * Removed at compile-time if LOG_MIN_LEVEL > LOG_LEVEL_WARN
 */
#if LOG_MIN_LEVEL <= LOG_LEVEL_WARN
    #if LOG_ENABLE_SOURCE_LOCATION
        #define LOG_WARN(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Warn, \
                                        LOG_FILENAME(__FILE__), __LINE__, \
                                        fmt, ##__VA_ARGS__)
    #else
        #define LOG_WARN(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Warn, \
                                        nullptr, 0, fmt, ##__VA_ARGS__)
    #endif
#else
    #define LOG_WARN(fmt, ...) ((void)0)
#endif

/**
 * ERROR level logging - error messages about failures
 * Removed at compile-time if LOG_MIN_LEVEL > LOG_LEVEL_ERROR
 */
#if LOG_MIN_LEVEL <= LOG_LEVEL_ERROR
    #if LOG_ENABLE_SOURCE_LOCATION
        #define LOG_ERROR(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Error, \
                                        LOG_FILENAME(__FILE__), __LINE__, \
                                        fmt, ##__VA_ARGS__)
    #else
        #define LOG_ERROR(fmt, ...) \
            ::alloy::logger::Logger::log(::alloy::logger::Level::Error, \
                                        nullptr, 0, fmt, ##__VA_ARGS__)
    #endif
#else
    #define LOG_ERROR(fmt, ...) ((void)0)
#endif
