/// Unit tests for host UART implementation
///
/// Tests the host UART using stdout/stdin simulation.

#include <catch2/catch_test_macros.hpp>

#include "hal/host/uart.hpp"

#include "core/units.hpp"

using namespace alloy::core;
using namespace alloy::hal;

/// Test: UART requires configuration before use
TEST_CASE("UART requires configuration before use", "[uart][host]") {
    // Given: Unconfigured UART
    alloy::hal::host::Uart uart;

    // When/Then: read_byte() should fail with NotInitialized
    auto result = uart.read_byte();
    REQUIRE(result.is_error());
    REQUIRE(result.error() == ErrorCode::NotInitialized);
}

/// Test: UART configuration succeeds with valid parameters
TEST_CASE("UART configuration succeeds", "[uart][host]") {
    // Given: UART and valid config
    alloy::hal::host::Uart uart;
    UartConfig config{baud_rates::Baud115200};

    // When: Configuring UART
    auto result = uart.configure(config);

    // Then: Configuration should succeed
    REQUIRE(result.is_ok());
}

/// Test: UART rejects zero baud rate
TEST_CASE("UART rejects zero baud rate", "[uart][host]") {
    // Given: UART and invalid config (zero baud rate)
    alloy::hal::host::Uart uart;
    UartConfig config{BaudRate{0}};

    // When: Configuring with invalid baud rate
    auto result = uart.configure(config);

    // Then: Configuration should fail
    REQUIRE(result.is_error());
    REQUIRE(result.error() == ErrorCode::InvalidParameter);
}

/// Test: Write byte succeeds after configuration
TEST_CASE("Write byte succeeds after configuration", "[uart][host][write]") {
    // Given: Configured UART
    alloy::hal::host::Uart uart;
    auto config_result = uart.configure(UartConfig{baud_rates::Baud115200});
    REQUIRE(config_result.is_ok());

    // When: Writing a byte
    auto result = uart.write_byte('A');

    // Then: Write should succeed
    REQUIRE(result.is_ok());
}

/// Test: Write fails without configuration
TEST_CASE("Write fails without configuration", "[uart][host][write]") {
    // Given: Unconfigured UART
    alloy::hal::host::Uart uart;

    // When/Then: write_byte() should fail
    auto result = uart.write_byte('A');
    REQUIRE(result.is_error());
    REQUIRE(result.error() == ErrorCode::NotInitialized);
}

/// Test: Read byte returns valid Result type
TEST_CASE("Read byte returns valid Result type", "[uart][host][read]") {
    // Given: Configured UART
    alloy::hal::host::Uart uart;
    auto config_result = uart.configure(UartConfig{baud_rates::Baud115200});
    REQUIRE(config_result.is_ok());

    // When: Reading from stdin
    auto result = uart.read_byte();

    // Then: Should return either:
    // - Ok with data byte (if stdin has data)
    // - Timeout error (if no data available)
    // - HardwareError (if read fails)
    // This test validates the Result type works correctly
    if (result.is_ok()) {
        // Data was successfully read
        REQUIRE(result.value() >= 0);
        REQUIRE(result.value() <= 255);
    } else {
        // Error occurred - should be Timeout or HardwareError
        REQUIRE(
            (result.error() == ErrorCode::Timeout || result.error() == ErrorCode::HardwareError));
    }
}

/// Test: Available returns sensible value
TEST_CASE("Available returns sensible value", "[uart][host]") {
    // Given: Configured UART
    alloy::hal::host::Uart uart;
    auto config_result = uart.configure(UartConfig{baud_rates::Baud115200});
    REQUIRE(config_result.is_ok());

    // When/Then: available() should return a non-negative value
    auto count = uart.available();
    REQUIRE(count >= 0);
}

/// Test: UART satisfies UartDevice concept
TEST_CASE("Host UART satisfies UartDevice concept", "[uart][host][concept]") {
    // Compile-time check already in uart.hpp
    // This test just validates the concept compiles
    REQUIRE((UartDevice<alloy::hal::host::Uart>));
}

/// Test: BaudRate type safety
TEST_CASE("BaudRate type safety", "[uart][baudrate]") {
    // Given: BaudRate values
    BaudRate rate1 = baud_rates::Baud9600;
    BaudRate rate2 = baud_rates::Baud115200;

    // Then: Values should be correct
    REQUIRE(rate1.value() == 9600);
    REQUIRE(rate2.value() == 115200);

    // And: Comparison should work
    REQUIRE(rate1 != rate2);
    REQUIRE(rate1 == BaudRate{9600});
}

/// Test: BaudRate literals
TEST_CASE("BaudRate literals", "[uart][baudrate]") {
    using namespace alloy::core::literals;

    // Given/When: Using literals
    auto rate1 = 9600_baud;
    auto rate2 = 115200_baud;

    // Then: Values should be correct
    REQUIRE(rate1.value() == 9600);
    REQUIRE(rate2.value() == 115200);
}

/// Test: UartConfig default values
TEST_CASE("UartConfig default values", "[uart][config]") {
    // Given/When: Creating config with defaults
    UartConfig config{baud_rates::Baud115200};

    // Then: Should have 8N1 defaults
    REQUIRE(config.data_bits == DataBits::Eight);
    REQUIRE(config.parity == Parity::None);
    REQUIRE(config.stop_bits == StopBits::One);
}

/// Test: UartConfig custom values
TEST_CASE("UartConfig custom values", "[uart][config]") {
    // Given/When: Creating config with custom values
    UartConfig config{baud_rates::Baud9600, DataBits::Seven, Parity::Even, StopBits::Two};

    // Then: Values should match
    REQUIRE(config.baud_rate == baud_rates::Baud9600);
    REQUIRE(config.data_bits == DataBits::Seven);
    REQUIRE(config.parity == Parity::Even);
    REQUIRE(config.stop_bits == StopBits::Two);
}
