#pragma once

#include <cstdarg>
#include <utility>

#include "logger/logger.hpp"
#include "logger/sinks/async_uart_sink.hpp"
#include "logger/sinks/uart_sink.hpp"

namespace alloy::logger {

template <typename UartImpl, size_t MaxSinks = LOG_MAX_SINKS,
          size_t BufferSize = 128u + LOG_MAX_MESSAGE_SIZE>
class UartLogger {
   public:
    explicit UartLogger(UartImpl& uart, Config config = {})
        : logger_(config),
          uart_sink_(uart) {
        static_cast<void>(logger_.add_sink(uart_sink_));
    }

    [[nodiscard]] auto logger() -> BasicLogger<MaxSinks, BufferSize>& { return logger_; }
    [[nodiscard]] auto logger() const -> const BasicLogger<MaxSinks, BufferSize>& { return logger_; }

    [[nodiscard]] auto sink() -> UartSink<UartImpl>& { return uart_sink_; }
    [[nodiscard]] auto sink() const -> const UartSink<UartImpl>& { return uart_sink_; }

    auto configure(const Config& config) -> void { logger_.configure(config); }
    auto set_level(Level level) -> void { logger_.set_level(level); }
    [[nodiscard]] auto level() const -> Level { return logger_.level(); }
    auto enable_colors(bool enable) -> void { logger_.enable_colors(enable); }
    auto set_timestamp_provider(TimestampProvider provider) -> void {
        logger_.set_timestamp_provider(provider);
    }
    auto set_line_ending(LineEnding line_ending) -> void { logger_.set_line_ending(line_ending); }

    template <typename SinkType>
    auto add_sink(SinkType& sink) -> bool {
        return logger_.add_sink(sink);
    }

    template <typename SinkType>
    auto remove_sink(SinkType& sink) -> void {
        logger_.remove_sink(sink);
    }

    auto flush() -> void { logger_.flush(); }

    auto log(Level level, SourceLocation source_location, const char* fmt, ...) -> void {
        va_list args;
        va_start(args, fmt);
        logger_.vlog(level, source_location, fmt, args);
        va_end(args);
    }

   private:
    BasicLogger<MaxSinks, BufferSize> logger_;
    UartSink<UartImpl> uart_sink_;
};

template <typename UartImpl, size_t MaxSinks = LOG_MAX_SINKS,
          size_t BufferSize = 128u + LOG_MAX_MESSAGE_SIZE>
auto make_uart_logger(UartImpl& uart, Config config = {}) -> UartLogger<UartImpl, MaxSinks, BufferSize> {
    return UartLogger<UartImpl, MaxSinks, BufferSize>(uart, config);
}

template <typename UartImpl, size_t Capacity = 512, size_t MaxRecords = 16,
          size_t MaxSinks = LOG_MAX_SINKS, size_t BufferSize = 128u + LOG_MAX_MESSAGE_SIZE>
class AsyncUartLogger {
   public:
    explicit AsyncUartLogger(UartImpl& uart, Config config = {})
        : logger_(config),
          uart_sink_(uart) {
        static_cast<void>(logger_.add_sink(uart_sink_));
    }

    [[nodiscard]] auto logger() -> BasicLogger<MaxSinks, BufferSize>& { return logger_; }
    [[nodiscard]] auto logger() const -> const BasicLogger<MaxSinks, BufferSize>& { return logger_; }

    [[nodiscard]] auto sink() -> AsyncUartSink<UartImpl, Capacity, MaxRecords>& { return uart_sink_; }
    [[nodiscard]] auto sink() const -> const AsyncUartSink<UartImpl, Capacity, MaxRecords>& {
        return uart_sink_;
    }

    auto configure(const Config& config) -> void { logger_.configure(config); }
    auto set_level(Level level) -> void { logger_.set_level(level); }
    [[nodiscard]] auto level() const -> Level { return logger_.level(); }
    auto enable_colors(bool enable) -> void { logger_.enable_colors(enable); }
    auto set_timestamp_provider(TimestampProvider provider) -> void {
        logger_.set_timestamp_provider(provider);
    }
    auto set_line_ending(LineEnding line_ending) -> void { logger_.set_line_ending(line_ending); }

    template <typename SinkType>
    auto add_sink(SinkType& sink) -> bool {
        return logger_.add_sink(sink);
    }

    template <typename SinkType>
    auto remove_sink(SinkType& sink) -> void {
        logger_.remove_sink(sink);
    }

    auto log(Level level, SourceLocation source_location, const char* fmt, ...) -> void {
        va_list args;
        va_start(args, fmt);
        logger_.vlog(level, source_location, fmt, args);
        va_end(args);
    }

    [[nodiscard]] auto pump_one() -> typename AsyncUartSink<UartImpl, Capacity, MaxRecords>::PumpStats {
        return uart_sink_.pump_one();
    }

    [[nodiscard]] auto pump_all() -> typename AsyncUartSink<UartImpl, Capacity, MaxRecords>::PumpStats {
        return uart_sink_.pump_all();
    }

    [[nodiscard]] auto pending_records() const -> size_t { return uart_sink_.pending_records(); }

    auto flush() -> void { logger_.flush(); }

   private:
    BasicLogger<MaxSinks, BufferSize> logger_;
    AsyncUartSink<UartImpl, Capacity, MaxRecords> uart_sink_;
};

template <typename UartImpl, size_t Capacity = 512, size_t MaxRecords = 16,
          size_t MaxSinks = LOG_MAX_SINKS, size_t BufferSize = 128u + LOG_MAX_MESSAGE_SIZE>
auto make_async_uart_logger(UartImpl& uart, Config config = {})
    -> AsyncUartLogger<UartImpl, Capacity, MaxRecords, MaxSinks, BufferSize> {
    return AsyncUartLogger<UartImpl, Capacity, MaxRecords, MaxSinks, BufferSize>(uart, config);
}

}  // namespace alloy::logger
