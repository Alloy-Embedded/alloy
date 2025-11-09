/// Unit tests for Alloy Logger System
///
/// Tests:
/// - Log level filtering (compile-time and runtime)
/// - Buffer sink output
/// - Message formatting
/// - Sink management

#include <cassert>
#include <cstring>

#include "logger/logger.hpp"
#include "logger/platform/buffer_sink.hpp"

using namespace alloy::logger;

// Test buffer
static char test_buffer[2048];

// Helper to check if buffer contains substring
bool buffer_contains(const char* needle) {
    return strstr(test_buffer, needle) != nullptr;
}

// Test: Basic logging
void test_basic_logging() {
    // Clear buffer
    memset(test_buffer, 0, sizeof(test_buffer));

    // Create buffer sink
    BufferSink sink(test_buffer, sizeof(test_buffer));
    Logger::add_sink(&sink);

    // Log message
    LOG_INFO("Test message");

    // Verify message appears in buffer
    assert(buffer_contains("Test message"));
    assert(buffer_contains("INFO"));

    // Cleanup
    Logger::remove_all_sinks();
}

// Test: Log levels
void test_log_levels() {
    memset(test_buffer, 0, sizeof(test_buffer));
    BufferSink sink(test_buffer, sizeof(test_buffer));
    Logger::add_sink(&sink);

    // Test all levels
    LOG_TRACE("Trace message");
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warn message");
    LOG_ERROR("Error message");

    // All messages should appear
    assert(buffer_contains("Info message"));
    assert(buffer_contains("Warn message"));
    assert(buffer_contains("Error message"));

    Logger::remove_all_sinks();
}

// Test: Runtime level filtering
void test_runtime_filtering() {
    memset(test_buffer, 0, sizeof(test_buffer));
    BufferSink sink(test_buffer, sizeof(test_buffer));
    Logger::add_sink(&sink);

    // Set level to WARN - should filter out INFO and below
    Logger::set_level(Level::Warn);

    LOG_INFO("Should not appear");
    LOG_WARN("Should appear");
    LOG_ERROR("Should also appear");

    // Verify filtering
    assert(!buffer_contains("Should not appear"));
    assert(buffer_contains("Should appear"));
    assert(buffer_contains("Should also appear"));

    // Reset level
    Logger::set_level(Level::Info);
    Logger::remove_all_sinks();
}

// Test: Formatted logging
void test_formatted_logging() {
    memset(test_buffer, 0, sizeof(test_buffer));
    BufferSink sink(test_buffer, sizeof(test_buffer));
    Logger::add_sink(&sink);

    // Log with format args
    int value = 42;
    float temp = 23.5f;

    LOG_INFO("Value: %d, Temp: %.1f", value, temp);

    // Verify formatting
    assert(buffer_contains("Value: 42"));
    assert(buffer_contains("Temp: 23.5"));

    Logger::remove_all_sinks();
}

// Test: Multiple sinks
void test_multiple_sinks() {
    char buffer1[512];
    char buffer2[512];
    memset(buffer1, 0, sizeof(buffer1));
    memset(buffer2, 0, sizeof(buffer2));

    BufferSink sink1(buffer1, sizeof(buffer1));
    BufferSink sink2(buffer2, sizeof(buffer2));

    Logger::add_sink(&sink1);
    Logger::add_sink(&sink2);

    LOG_INFO("Multi-sink test");

    // Both sinks should receive the message
    assert(strstr(buffer1, "Multi-sink test") != nullptr);
    assert(strstr(buffer2, "Multi-sink test") != nullptr);

    Logger::remove_all_sinks();
}

// Test: Sink management
void test_sink_management() {
    char buffer1[512];
    memset(buffer1, 0, sizeof(buffer1));

    BufferSink sink1(buffer1, sizeof(buffer1));

    // Add sink
    assert(Logger::add_sink(&sink1) == true);

    LOG_INFO("Message 1");
    assert(strstr(buffer1, "Message 1") != nullptr);

    // Remove sink
    Logger::remove_sink(&sink1);

    // Clear buffer and log again
    memset(buffer1, 0, sizeof(buffer1));
    LOG_INFO("Message 2");

    // Message 2 should not appear (sink was removed)
    assert(strstr(buffer1, "Message 2") == nullptr);
}

// Test: Buffer sink behavior
void test_buffer_sink() {
    char small_buffer[64];
    memset(small_buffer, 0, sizeof(small_buffer));

    BufferSink sink(small_buffer, sizeof(small_buffer));

    // Fill buffer
    const char* test_data = "Test data to write";
    sink.write(test_data, strlen(test_data));

    assert(sink.size() == strlen(test_data));
    assert(strcmp(sink.data(), test_data) == 0);

    // Clear buffer
    sink.clear();
    assert(sink.size() == 0);
    assert(strlen(sink.data()) == 0);
}

// Test: Source location (if enabled)
#if LOG_ENABLE_SOURCE_LOCATION
void test_source_location() {
    memset(test_buffer, 0, sizeof(test_buffer));
    BufferSink sink(test_buffer, sizeof(test_buffer));
    Logger::add_sink(&sink);

    LOG_INFO("Location test");

    // Should contain filename and line info
    assert(buffer_contains("test_logger.cpp"));

    Logger::remove_all_sinks();
}
#endif

// Main test runner
int main() {
    // Initialize logger
    Logger::set_level(Level::Trace);  // Show all logs for testing

    // Run tests
    test_basic_logging();
    test_log_levels();
    test_runtime_filtering();
    test_formatted_logging();
    test_multiple_sinks();
    test_sink_management();
    test_buffer_sink();

#if LOG_ENABLE_SOURCE_LOCATION
    test_source_location();
#endif

    return 0;
}
