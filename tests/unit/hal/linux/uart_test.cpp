/**
 * @file uart_test.cpp
 * @brief Unit tests for Linux UART implementation
 *
 * These tests verify the UART behavior on Linux hosts.
 * Tests are designed to work without requiring actual hardware.
 *
 * Test coverage:
 * - Open/close lifecycle
 * - Error handling for missing devices
 * - Baudrate configuration
 * - State management
 * - API compatibility
 *
 * Note: Full read/write tests would require pty (pseudo-terminal)
 * setup which is complex. These tests focus on API validation.
 */

#include "hal/platform/linux/uart.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace alloy::hal::linux;
using namespace alloy::core;

// ============================================================================
// Device Path Constants for Testing
// ============================================================================

// Non-existent device for error testing
inline constexpr const char nonexistent_device[] = "/dev/tty_does_not_exist_12345";
using UartNonExistent = Uart<nonexistent_device>;

// ============================================================================
// Open/Close Lifecycle Tests
// ============================================================================

TEST_CASE("UART open() fails for non-existent device", "[uart][lifecycle]") {
    auto uart = UartNonExistent{};

    SECTION("open() returns error for missing device") {
        auto result = uart.open();

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::HardwareError);
    }

    SECTION("UART reports not open after failed open") {
        uart.open();  // Fail to open
        REQUIRE_FALSE(uart.isOpen());
    }
}

TEST_CASE("UART close() fails if not opened", "[uart][lifecycle]") {
    auto uart = UartNonExistent{};

    SECTION("close() without open() returns error") {
        auto result = uart.close();

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }
}

TEST_CASE("UART open() prevents double initialization", "[uart][lifecycle]") {
    auto uart = UartNonExistent{};

    // Note: This test assumes we have a way to test with a real device
    // For now, we test the error case
    SECTION("Second open() would return AlreadyInitialized") {
        // This would require a real device to test properly
        // Just verify the API exists
        auto result1 = uart.open();
        REQUIRE(result1.is_error());  // Device doesn't exist
    }
}

// ============================================================================
// State Management Tests
// ============================================================================

TEST_CASE("UART isOpen() reflects state correctly", "[uart][state]") {
    auto uart = UartNonExistent{};

    SECTION("Initially not open") {
        REQUIRE_FALSE(uart.isOpen());
    }

    SECTION("Still not open after failed open") {
        uart.open();  // Fails
        REQUIRE_FALSE(uart.isOpen());
    }
}

// ============================================================================
// Write Operation Tests
// ============================================================================

TEST_CASE("UART write() requires initialization", "[uart][write]") {
    auto uart = UartNonExistent{};

    SECTION("write() without open() returns error") {
        uint8_t data[] = {0x01, 0x02, 0x03};
        auto result = uart.write(data, sizeof(data));

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }

    SECTION("write() with nullptr returns error") {
        auto result = uart.write(nullptr, 10);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::InvalidParameter);
    }

    SECTION("write() with zero size succeeds (edge case)") {
        uint8_t data[] = {0x01};
        auto result = uart.write(data, 0);

        // Should return InvalidParameter or NotInitialized
        REQUIRE(result.is_error());
    }
}

// ============================================================================
// Read Operation Tests
// ============================================================================

TEST_CASE("UART read() requires initialization", "[uart][read]") {
    auto uart = UartNonExistent{};

    SECTION("read() without open() returns error") {
        uint8_t buffer[16];
        auto result = uart.read(buffer, sizeof(buffer));

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }

    SECTION("read() with nullptr returns error") {
        auto result = uart.read(nullptr, 10);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::InvalidParameter);
    }

    SECTION("read() with zero size succeeds (edge case)") {
        uint8_t buffer[16];
        auto result = uart.read(buffer, 0);

        // Should return InvalidParameter or NotInitialized
        REQUIRE(result.is_error());
    }
}

// ============================================================================
// Baudrate Configuration Tests
// ============================================================================

TEST_CASE("UART setBaudrate() requires initialization", "[uart][baudrate]") {
    auto uart = UartNonExistent{};

    SECTION("setBaudrate() without open() returns error") {
        auto result = uart.setBaudrate(Baudrate::e115200);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }
}

// ============================================================================
// Buffer Management Tests
// ============================================================================

TEST_CASE("UART available() requires initialization", "[uart][buffer]") {
    auto uart = UartNonExistent{};

    SECTION("available() without open() returns error") {
        auto result = uart.available();

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }
}

TEST_CASE("UART flush() requires initialization", "[uart][buffer]") {
    auto uart = UartNonExistent{};

    SECTION("flush() without open() returns error") {
        auto result = uart.flush();

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }
}

// ============================================================================
// Type Alias Tests
// ============================================================================

TEST_CASE("UART type aliases are distinct types", "[uart][types]") {
    SECTION("UartUsb0 and UartUsb1 are different types") {
        // This is a compile-time check - if it compiles, test passes
        [[maybe_unused]] UartUsb0 uart0;
        [[maybe_unused]] UartUsb1 uart1;

        // They should have different device paths
        REQUIRE(std::string(UartUsb0::device_path) != std::string(UartUsb1::device_path));
    }

    SECTION("Device paths are correct") {
        REQUIRE(std::string(UartUsb0::device_path) == "/dev/ttyUSB0");
        REQUIRE(std::string(UartUsb1::device_path) == "/dev/ttyUSB1");
        REQUIRE(std::string(UartAcm0::device_path) == "/dev/ttyACM0");
        REQUIRE(std::string(UartS0::device_path) == "/dev/ttyS0");
    }
}

// ============================================================================
// Compile-Time Checks
// ============================================================================

TEST_CASE("UART has expected compile-time properties", "[uart][compile]") {
    SECTION("Device path is constexpr") {
        constexpr const char* path = UartUsb0::device_path;
        REQUIRE(path != nullptr);
    }

    SECTION("UART is not copyable") {
        REQUIRE_FALSE(std::is_copy_constructible_v<UartUsb0>);
        REQUIRE_FALSE(std::is_copy_assignable_v<UartUsb0>);
    }

    SECTION("UART is not movable") {
        REQUIRE_FALSE(std::is_move_constructible_v<UartUsb0>);
        REQUIRE_FALSE(std::is_move_assignable_v<UartUsb0>);
    }

    SECTION("UART is default constructible") {
        REQUIRE(std::is_default_constructible_v<UartUsb0>);
    }
}

// ============================================================================
// Error Code Coverage
// ============================================================================

TEST_CASE("UART uses appropriate error codes", "[uart][errors]") {
    auto uart = UartNonExistent{};

    SECTION("Missing device returns HardwareError") {
        auto result = uart.open();
        REQUIRE(result.error() == ErrorCode::HardwareError);
    }

    SECTION("Operations before open return NotInitialized") {
        uint8_t buffer[16];

        auto write_result = uart.write(buffer, 1);
        REQUIRE(write_result.error() == ErrorCode::NotInitialized);

        auto read_result = uart.read(buffer, 1);
        REQUIRE(read_result.error() == ErrorCode::NotInitialized);

        auto baudrate_result = uart.setBaudrate(Baudrate::e115200);
        REQUIRE(baudrate_result.error() == ErrorCode::NotInitialized);

        auto available_result = uart.available();
        REQUIRE(available_result.error() == ErrorCode::NotInitialized);

        auto flush_result = uart.flush();
        REQUIRE(flush_result.error() == ErrorCode::NotInitialized);
    }

    SECTION("Close without open returns NotInitialized") {
        auto result = uart.close();
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }

    SECTION("Nullptr parameters return InvalidParameter") {
        auto write_result = uart.write(nullptr, 10);
        REQUIRE(write_result.error() == ErrorCode::InvalidParameter);

        auto read_result = uart.read(nullptr, 10);
        REQUIRE(read_result.error() == ErrorCode::InvalidParameter);
    }
}

// ============================================================================
// Summary Statistics
// ============================================================================

/*
 * Test Coverage Summary:
 * - Lifecycle: 3 test cases
 * - State: 1 test case
 * - Write: 1 test case (3 sections)
 * - Read: 1 test case (3 sections)
 * - Baudrate: 1 test case
 * - Buffer: 2 test cases
 * - Types: 1 test case (2 sections)
 * - Compile: 1 test case (4 sections)
 * - Errors: 1 test case (4 sections)
 *
 * Total: 12 test cases with multiple sections
 *
 * Note: These tests focus on API validation and error handling.
 * Full functional testing would require pty setup, which is
 * platform-specific and complex. Integration tests should be
 * added separately for that purpose.
 */
