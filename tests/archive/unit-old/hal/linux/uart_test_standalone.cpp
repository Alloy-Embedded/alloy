/**
 * @file uart_test_standalone.cpp
 * @brief Standalone unit tests for Linux UART (no Catch2 required)
 *
 * Simple test runner that doesn't require external dependencies.
 * For full test suite, use CMake with Catch2.
 */

#include <cassert>
#include <iostream>
#include <string>

#include "hal/platform/linux/uart.hpp"

using namespace alloy::hal::linux;
using namespace alloy::core;

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Simple assertion macro
#define TEST_ASSERT(condition, message)                                            \
    do {                                                                           \
        tests_run++;                                                               \
        if (!(condition)) {                                                        \
            std::cerr << "❌ FAIL: " << message << " (line " << __LINE__ << ")\n"; \
            tests_failed++;                                                        \
        } else {                                                                   \
            std::cout << "✓ PASS: " << message << "\n";                            \
            tests_passed++;                                                        \
        }                                                                          \
    } while (0)

// Test device path
inline constexpr const char nonexistent_device[] = "/dev/tty_does_not_exist_test";
using TestUart = Uart<nonexistent_device>;

void test_open_nonexistent_device() {
    std::cout << "\n=== Test: open() non-existent device ===\n";

    TestUart uart;
    auto result = uart.open();

    TEST_ASSERT(result.is_error(), "open() should fail for non-existent device");
    TEST_ASSERT(result.error() == ErrorCode::HardwareError, "Error code should be HardwareError");
    TEST_ASSERT(!uart.isOpen(), "UART should not be open after failed open()");
}

void test_close_without_open() {
    std::cout << "\n=== Test: close() without open() ===\n";

    TestUart uart;
    auto result = uart.close();

    TEST_ASSERT(result.is_error(), "close() should fail without open()");
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized, "Error code should be NotInitialized");
}

void test_write_without_open() {
    std::cout << "\n=== Test: write() without open() ===\n";

    TestUart uart;
    uint8_t data[] = {0x01, 0x02, 0x03};
    auto result = uart.write(data, sizeof(data));

    TEST_ASSERT(result.is_error(), "write() should fail without open()");
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized, "Error code should be NotInitialized");
}

void test_write_with_nullptr() {
    std::cout << "\n=== Test: write() with nullptr ===\n";

    TestUart uart;
    auto result = uart.write(nullptr, 10);

    TEST_ASSERT(result.is_error(), "write() should fail with nullptr");
    // Note: Returns NotInitialized because UART is not open (checked first)
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized,
                "Error code should be NotInitialized (not open)");
}

void test_read_without_open() {
    std::cout << "\n=== Test: read() without open() ===\n";

    TestUart uart;
    uint8_t buffer[16];
    auto result = uart.read(buffer, sizeof(buffer));

    TEST_ASSERT(result.is_error(), "read() should fail without open()");
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized, "Error code should be NotInitialized");
}

void test_read_with_nullptr() {
    std::cout << "\n=== Test: read() with nullptr ===\n";

    TestUart uart;
    auto result = uart.read(nullptr, 10);

    TEST_ASSERT(result.is_error(), "read() should fail with nullptr");
    // Note: Returns NotInitialized because UART is not open (checked first)
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized,
                "Error code should be NotInitialized (not open)");
}

void test_baudrate_without_open() {
    std::cout << "\n=== Test: setBaudrate() without open() ===\n";

    TestUart uart;
    auto result = uart.setBaudrate(Baudrate::e115200);

    TEST_ASSERT(result.is_error(), "setBaudrate() should fail without open()");
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized, "Error code should be NotInitialized");
}

void test_available_without_open() {
    std::cout << "\n=== Test: available() without open() ===\n";

    TestUart uart;
    auto result = uart.available();

    TEST_ASSERT(result.is_error(), "available() should fail without open()");
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized, "Error code should be NotInitialized");
}

void test_flush_without_open() {
    std::cout << "\n=== Test: flush() without open() ===\n";

    TestUart uart;
    auto result = uart.flush();

    TEST_ASSERT(result.is_error(), "flush() should fail without open()");
    TEST_ASSERT(result.error() == ErrorCode::NotInitialized, "Error code should be NotInitialized");
}

void test_type_aliases() {
    std::cout << "\n=== Test: Type aliases ===\n";

    TEST_ASSERT(std::string(UartUsb0::device_path) == "/dev/ttyUSB0", "UartUsb0 path correct");
    TEST_ASSERT(std::string(UartUsb1::device_path) == "/dev/ttyUSB1", "UartUsb1 path correct");
    TEST_ASSERT(std::string(UartAcm0::device_path) == "/dev/ttyACM0", "UartAcm0 path correct");
    TEST_ASSERT(std::string(UartS0::device_path) == "/dev/ttyS0", "UartS0 path correct");
}

void test_compile_time_properties() {
    std::cout << "\n=== Test: Compile-time properties ===\n";

    TEST_ASSERT(!std::is_copy_constructible_v<UartUsb0>, "UART not copyable");
    TEST_ASSERT(!std::is_move_constructible_v<UartUsb0>, "UART not movable");
    TEST_ASSERT(std::is_default_constructible_v<UartUsb0>, "UART default constructible");
}

int main() {
    std::cout << "======================================\n";
    std::cout << "Linux UART Standalone Tests\n";
    std::cout << "======================================\n";

    // Run all tests
    test_open_nonexistent_device();
    test_close_without_open();
    test_write_without_open();
    test_write_with_nullptr();
    test_read_without_open();
    test_read_with_nullptr();
    test_baudrate_without_open();
    test_available_without_open();
    test_flush_without_open();
    test_type_aliases();
    test_compile_time_properties();

    // Print summary
    std::cout << "\n======================================\n";
    std::cout << "Test Summary\n";
    std::cout << "======================================\n";
    std::cout << "Total:  " << tests_run << "\n";
    std::cout << "Passed: " << tests_passed << " ✓\n";
    std::cout << "Failed: " << tests_failed << " ❌\n";
    std::cout << "======================================\n";

    if (tests_failed == 0) {
        std::cout << "\n🎉 All tests passed!\n\n";
        return 0;
    } else {
        std::cout << "\n⚠️  Some tests failed!\n\n";
        return 1;
    }
}
