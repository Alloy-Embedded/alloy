#pragma once

#include "format.hpp"
#include "sink.hpp"
#include "types.hpp"

#ifdef ALLOY_HAS_RTOS
    #include "rtos/mutex.hpp"
#endif

#include <cstdarg>
#include <cstring>

namespace alloy::logger {

template <size_t MaxSinks = LOG_MAX_SINKS, size_t BufferSize = 128u + LOG_MAX_MESSAGE_SIZE>
class BasicLogger {
   public:
    static_assert(MaxSinks > 0u, "BasicLogger requires at least one sink slot");
    static_assert(BufferSize > 1u, "BasicLogger buffer must reserve room for a terminator");

    BasicLogger() : BasicLogger(Config{}) {}

    explicit BasicLogger(Config config)
        : config_(config),
          runtime_level_(config_.default_level) {
        buffer_[0] = '\0';
    }

    auto configure(const Config& config) -> void {
        config_ = config;
        runtime_level_ = config_.default_level;
    }

    [[nodiscard]] auto config() const -> const Config& { return config_; }

    auto set_level(Level level) -> void { runtime_level_ = level; }

    [[nodiscard]] auto level() const -> Level { return runtime_level_; }

    auto enable_colors(bool enable) -> void { config_.enable_colors = enable; }

    auto set_timestamp_provider(TimestampProvider provider) -> void {
        config_.timestamp_provider = provider;
    }

    auto set_line_ending(LineEnding line_ending) -> void { config_.line_ending = line_ending; }

    [[nodiscard]] auto sink_count() const -> size_t { return sink_count_; }

    auto add_sink(SinkRef sink) -> bool {
        if (!sink.valid()) {
            return false;
        }
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(mutex_);
#endif
        for (size_t index = 0; index < sink_count_; ++index) {
            if (sinks_[index].identity() == sink.identity()) {
                sinks_[index] = sink;
                return true;
            }
        }
        if (sink_count_ >= MaxSinks) {
            return false;
        }
        sinks_[sink_count_++] = sink;
        return true;
    }

    template <typename SinkType>
        requires(!std::same_as<std::remove_cvref_t<SinkType>, SinkRef>)
    auto add_sink(SinkType& sink) -> bool {
        return add_sink(make_sink_ref(sink));
    }

    auto remove_sink(const void* identity) -> void {
        if (identity == nullptr) {
            return;
        }
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(mutex_);
#endif
        for (size_t index = 0; index < sink_count_; ++index) {
            if (sinks_[index].identity() == identity) {
                for (size_t shift = index; shift + 1u < sink_count_; ++shift) {
                    sinks_[shift] = sinks_[shift + 1u];
                }
                sinks_[--sink_count_] = {};
                return;
            }
        }
    }

    template <typename SinkType>
        requires(!std::same_as<std::remove_cvref_t<SinkType>, SinkRef>)
    auto remove_sink(SinkType& sink) -> void {
        remove_sink(static_cast<const void*>(&sink));
    }

    auto remove_all_sinks() -> void {
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(mutex_);
#endif
        for (size_t index = 0; index < sink_count_; ++index) {
            sinks_[index] = {};
        }
        sink_count_ = 0u;
    }

    auto flush() -> void {
#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(mutex_);
#endif
        for (size_t index = 0; index < sink_count_; ++index) {
            sinks_[index].flush();
        }
    }

    auto log(Level level, SourceLocation source_location, const char* fmt, ...) -> void {
        va_list args;
        va_start(args, fmt);
        vlog(level, source_location, fmt, args);
        va_end(args);
    }

    auto vlog(Level level, SourceLocation source_location, const char* fmt, va_list args) -> void {
        if (level < runtime_level_) {
            return;
        }

#ifdef ALLOY_HAS_RTOS
        rtos::MutexGuard lock(mutex_);
#endif
        if (sink_count_ == 0u || fmt == nullptr) {
            return;
        }

        Config active_config = config_;
        active_config.enable_timestamps =
            config_.enable_timestamps && (config_.timestamp_provider != nullptr);

        const std::uint64_t timestamp_us =
            active_config.enable_timestamps ? config_.timestamp_provider() : 0u;

        const auto prefix_len =
            Formatter::format_prefix(buffer_, BufferSize, timestamp_us, level, source_location,
                                     active_config);

        size_t total_len = prefix_len;
        size_t payload_len = 0u;
        if (prefix_len < BufferSize) {
            payload_len = Formatter::format_message(buffer_ + prefix_len, BufferSize - prefix_len, fmt, args);
            total_len += payload_len;
        }
        if (total_len < BufferSize) {
            total_len +=
                Formatter::append_line_ending(buffer_ + total_len, BufferSize - total_len, config_.line_ending);
        }
        if (total_len < BufferSize) {
            buffer_[total_len] = '\0';
        } else {
            buffer_[BufferSize - 1u] = '\0';
            total_len = BufferSize - 1u;
        }

        const RecordView record{
            .level = level,
            .timestamp_us = timestamp_us,
            .source_location = source_location,
            .payload = std::string_view{buffer_ + prefix_len, payload_len},
            .rendered = std::string_view{buffer_, total_len},
        };

        for (size_t index = 0; index < sink_count_; ++index) {
            if (sinks_[index].is_ready()) {
                sinks_[index].write_record(record);
            }
        }
    }

   private:
    Config config_{};
    Level runtime_level_ = Level::Info;
    SinkRef sinks_[MaxSinks]{};
    size_t sink_count_ = 0u;
    char buffer_[BufferSize]{};

#ifdef ALLOY_HAS_RTOS
    rtos::Mutex mutex_;
#endif
};

/**
 * Legacy compatibility wrapper.
 *
 * Existing examples and call sites can keep using the global `Logger` and
 * `LOG_*` macros, while new code can instantiate `BasicLogger<>` directly.
 */
class Logger {
   public:
    using DefaultLogger = BasicLogger<>;

    static auto instance() -> DefaultLogger& {
        static DefaultLogger logger;
        return logger;
    }

    static auto configure(const Config& config) -> void { instance().configure(config); }

    [[nodiscard]] static auto config() -> const Config& { return instance().config(); }

    static auto set_level(Level level) -> void { instance().set_level(level); }

    [[nodiscard]] static auto get_level() -> Level { return instance().level(); }

    static auto add_sink(SinkRef sink) -> bool { return instance().add_sink(sink); }

    static auto add_sink(Sink* sink) -> bool {
        return sink != nullptr ? instance().add_sink(make_sink_ref(*sink)) : false;
    }

    template <typename SinkType>
        requires(!std::same_as<std::remove_cvref_t<SinkType>, SinkRef> &&
                 !std::is_pointer_v<std::remove_reference_t<SinkType>>)
    static auto add_sink(SinkType& sink) -> bool {
        return instance().add_sink(sink);
    }

    static auto remove_sink(SinkRef sink) -> void { instance().remove_sink(sink.identity()); }

    static auto remove_sink(Sink* sink) -> void { instance().remove_sink(sink); }

    template <typename SinkType>
        requires(!std::same_as<std::remove_cvref_t<SinkType>, SinkRef> &&
                 !std::is_pointer_v<std::remove_reference_t<SinkType>>)
    static auto remove_sink(SinkType& sink) -> void {
        instance().remove_sink(sink);
    }

    static auto remove_all_sinks() -> void { instance().remove_all_sinks(); }

    static auto flush() -> void { instance().flush(); }

    static auto enable_colors(bool enable) -> void { instance().enable_colors(enable); }

    static auto set_line_ending(LineEnding line_ending) -> void {
        instance().set_line_ending(line_ending);
    }

    static auto set_timestamp_provider(TimestampProvider provider) -> void {
        instance().set_timestamp_provider(provider);
    }

    static auto log(Level level, const char* file, int line, const char* fmt, ...) -> void {
        va_list args;
        va_start(args, fmt);
        instance().vlog(level, SourceLocation{file, line}, fmt, args);
        va_end(args);
    }
};

}  // namespace alloy::logger

// ============================================================================
// Logging Macros
// ============================================================================

/**
 * Helper macro to strip file path down to just filename
 */
#define LOG_FILENAME(file)                       \
    (strrchr(file, '/') ? strrchr(file, '/') + 1 \
                        : (strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file))

#if LOG_ENABLE_SOURCE_LOCATION
    #define ALLOY_LOG_SOURCE_LOCATION \
        ::alloy::logger::SourceLocation{LOG_FILENAME(__FILE__), __LINE__}
#else
    #define ALLOY_LOG_SOURCE_LOCATION ::alloy::logger::SourceLocation{}
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_TRACE
    #define LOG_TRACE_TO(logger_instance, fmt, ...) \
        (logger_instance).log(::alloy::logger::Level::Trace, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
    #define LOG_TRACE(fmt, ...) \
        ::alloy::logger::Logger::instance().log(::alloy::logger::Level::Trace, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
#else
    #define LOG_TRACE_TO(logger_instance, fmt, ...) ((void)0)
    #define LOG_TRACE(fmt, ...)            ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_DEBUG
    #define LOG_DEBUG_TO(logger_instance, fmt, ...) \
        (logger_instance).log(::alloy::logger::Level::Debug, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) \
        ::alloy::logger::Logger::instance().log(::alloy::logger::Level::Debug, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
#else
    #define LOG_DEBUG_TO(logger_instance, fmt, ...) ((void)0)
    #define LOG_DEBUG(fmt, ...)            ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_INFO
    #define LOG_INFO_TO(logger_instance, fmt, ...) \
        (logger_instance).log(::alloy::logger::Level::Info, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...) \
        ::alloy::logger::Logger::instance().log(::alloy::logger::Level::Info, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
#else
    #define LOG_INFO_TO(logger_instance, fmt, ...) ((void)0)
    #define LOG_INFO(fmt, ...)            ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_WARN
    #define LOG_WARN_TO(logger_instance, fmt, ...) \
        (logger_instance).log(::alloy::logger::Level::Warn, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...) \
        ::alloy::logger::Logger::instance().log(::alloy::logger::Level::Warn, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
#else
    #define LOG_WARN_TO(logger_instance, fmt, ...) ((void)0)
    #define LOG_WARN(fmt, ...)            ((void)0)
#endif

#if LOG_MIN_LEVEL <= LOG_LEVEL_ERROR
    #define LOG_ERROR_TO(logger_instance, fmt, ...) \
        (logger_instance).log(::alloy::logger::Level::Error, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) \
        ::alloy::logger::Logger::instance().log(::alloy::logger::Level::Error, ALLOY_LOG_SOURCE_LOCATION, fmt, ##__VA_ARGS__)
#else
    #define LOG_ERROR_TO(logger_instance, fmt, ...) ((void)0)
    #define LOG_ERROR(fmt, ...)            ((void)0)
#endif
