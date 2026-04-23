/**
 * @file test_logger.cpp
 * @brief Unit tests for the modernized logger core.
 */

#include <catch2/catch_test_macros.hpp>

#include <span>
#include <string>
#include <string_view>

#include "core/error.hpp"
#include "core/result.hpp"
#include "logger/logger.hpp"
#include "logger/uart_logger.hpp"
#include "logger/sinks/async_uart_sink.hpp"
#include "logger/sinks/ring_buffer_sink.hpp"
#include "logger/sinks/uart_sink.hpp"

namespace {

struct StringSink {
    std::string data;
    bool ready = true;
    bool flushed = false;

    void write(std::string_view text) { data.append(text); }
    void flush() { flushed = true; }
    [[nodiscard]] auto is_ready() const -> bool { return ready; }
};

struct LegacyStringSink : alloy::logger::Sink {
    std::string data;

    void write(const char* text, size_t length) override { data.append(text, length); }
};

struct FakeBulkUart {
    std::string bytes;
    int bulk_calls = 0;
    int flush_calls = 0;
    bool fail_next = false;

    auto write(std::span<const std::byte> data)
        -> alloy::core::Result<std::size_t, alloy::core::ErrorCode> {
        if (fail_next) {
            fail_next = false;
            return alloy::core::Err(alloy::core::ErrorCode::Busy);
        }
        ++bulk_calls;
        bytes.append(reinterpret_cast<const char*>(data.data()), data.size());
        return alloy::core::Ok(data.size());
    }

    auto flush() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        ++flush_calls;
        return alloy::core::Ok();
    }
};

struct LegacyLoggerGuard {
    ~LegacyLoggerGuard() {
        alloy::logger::Logger::remove_all_sinks();
        alloy::logger::Logger::configure({});
    }
};

}  // namespace

TEST_CASE("BasicLogger emits to multiple sinks through lightweight sink refs",
          "[logger][unit][multi-sink]") {
    alloy::logger::BasicLogger<2, 128> logger;
    logger.configure({
        .default_level = alloy::logger::Level::Trace,
        .enable_timestamps = false,
        .enable_colors = false,
        .enable_source_location = false,
    });

    StringSink first_sink;
    StringSink second_sink;

    REQUIRE(logger.add_sink(first_sink));
    REQUIRE(logger.add_sink(second_sink));

    logger.log(alloy::logger::Level::Info, {}, "value=%d", 7);

    REQUIRE(first_sink.data == "INFO  value=7\n");
    REQUIRE(second_sink.data == first_sink.data);
}

TEST_CASE("BasicLogger respects runtime filtering and instance-local formatting",
          "[logger][unit][filtering]") {
    alloy::logger::BasicLogger<1, 128> logger;
    logger.configure({
        .default_level = alloy::logger::Level::Warn,
        .enable_timestamps = true,
        .enable_colors = false,
        .enable_source_location = true,
        .timestamp_precision = alloy::logger::TimestampPrecision::Milliseconds,
        .line_ending = alloy::logger::LineEnding::CRLF,
        .timestamp_provider = []() -> std::uint64_t { return 1'234'567ull; },
    });

    StringSink sink;
    REQUIRE(logger.add_sink(sink));

    logger.log(alloy::logger::Level::Info, {"ignored.cpp", 5}, "suppressed");
    REQUIRE(sink.data.empty());

    logger.log(alloy::logger::Level::Error, {"driver.cpp", 17}, "boom");

    REQUIRE(sink.data == "[1.234] ERROR [driver.cpp:17] boom\r\n");
}

TEST_CASE("Legacy Logger wrapper stays compatible with old global flow",
          "[logger][unit][legacy]") {
    LegacyLoggerGuard guard;
    LegacyStringSink sink;

    alloy::logger::Logger::remove_all_sinks();
    alloy::logger::Logger::configure({
        .default_level = alloy::logger::Level::Info,
        .enable_timestamps = false,
        .enable_colors = false,
        .enable_source_location = false,
    });

    REQUIRE(alloy::logger::Logger::add_sink(&sink));

    LOG_INFO("legacy %d", 1);

    REQUIRE(sink.data == "INFO  legacy 1\n");
}

TEST_CASE("UART sink prefers bulk writes when the HAL handle supports them",
          "[logger][unit][uart]") {
    FakeBulkUart uart;
    auto sink = alloy::logger::make_uart_sink(uart);

    sink.write("ABC", 3);
    sink.flush();

    REQUIRE(uart.bulk_calls == 1);
    REQUIRE(uart.flush_calls == 1);
    REQUIRE(uart.bytes == "ABC");
}

TEST_CASE("AsyncUartSink buffers records until drained explicitly",
          "[logger][unit][async-uart]") {
    FakeBulkUart uart;
    auto sink = alloy::logger::make_async_uart_sink<FakeBulkUart, 128, 8>(uart);

    sink.write_record({
        .level = alloy::logger::Level::Info,
        .rendered = "first\n",
    });
    sink.write_record({
        .level = alloy::logger::Level::Warn,
        .rendered = "second\n",
    });

    REQUIRE(sink.pending_records() == 2);
    const auto stats = sink.pump_all();
    REQUIRE(stats.records_drained == 2);
    REQUIRE(stats.bytes_written == 13);
    REQUIRE_FALSE(stats.transport_error);
    REQUIRE(uart.bytes == "first\nsecond\n");
    REQUIRE(sink.pending_records() == 0);
}

TEST_CASE("UartLogger helper wires the owned UART sink automatically",
          "[logger][unit][uart-logger-helper]") {
    FakeBulkUart uart;
    auto logger = alloy::logger::make_uart_logger(uart, {
        .default_level = alloy::logger::Level::Info,
        .enable_timestamps = false,
        .enable_source_location = false,
    });

    LOG_INFO_TO(logger, "hello");
    logger.flush();

    REQUIRE(uart.bytes == "INFO  hello\n");
}

TEST_CASE("RingBufferSink keeps recent rendered logs together with record metadata",
          "[logger][unit][ring-buffer]") {
    alloy::logger::BasicLogger<2, 128> logger;
    logger.configure({
        .default_level = alloy::logger::Level::Trace,
        .enable_timestamps = true,
        .enable_colors = false,
        .enable_source_location = true,
        .timestamp_precision = alloy::logger::TimestampPrecision::Microseconds,
        .timestamp_provider = []() -> std::uint64_t { return 42u; },
    });

    alloy::logger::RingBufferSink<96, 4> ring_sink;
    REQUIRE(logger.add_sink(ring_sink));

    logger.log(alloy::logger::Level::Warn, {"ring.cpp", 33}, "ready=%d", 1);

    REQUIRE(ring_sink.record_count() == 1);
    const auto latest = ring_sink.latest();
    REQUIRE(latest.level == alloy::logger::Level::Warn);
    REQUIRE(latest.timestamp_us == 42u);
    REQUIRE(std::string_view{latest.source_location.file} == "ring.cpp");
    REQUIRE(latest.payload == "ready=1");
    REQUIRE(latest.rendered == "[0.000042] WARN  [ring.cpp:33] ready=1\n");
}
