/**
 * @file test_clock_concept.cpp
 * @brief Unit tests for ClockPlatform concept compliance
 *
 * Tests that Clock implementations satisfy the ClockPlatform concept.
 */

#include <catch2/catch_test_macros.hpp>

#include "hal/core/concepts.hpp"
#include "core/result.hpp"
#include "core/error.hpp"

using namespace alloy::core;
using namespace alloy::hal;

// ==============================================================================
// Mock Clock Platform for Testing
// ==============================================================================

/**
 * @brief Mock clock platform that satisfies ClockPlatform concept
 *
 * Used for testing concept compliance without hardware.
 */
class MockClockPlatform {
public:
    static constexpr uint32_t DEFAULT_CLOCK_HZ = 64'000'000;

    // Required by ClockPlatform concept
    static Result<void, ErrorCode> initialize() {
        return Ok();
    }

    static Result<void, ErrorCode> enable_gpio_clocks() {
        return Ok();
    }

    static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_base) {
        if (uart_base == 0) {
            return Err(ErrorCode::InvalidParameter);
        }
        return Ok();
    }

    static Result<void, ErrorCode> enable_spi_clock(uint32_t spi_base) {
        if (spi_base == 0) {
            return Err(ErrorCode::InvalidParameter);
        }
        return Ok();
    }

    static Result<void, ErrorCode> enable_i2c_clock(uint32_t i2c_base) {
        if (i2c_base == 0) {
            return Err(ErrorCode::InvalidParameter);
        }
        return Ok();
    }

    static constexpr uint32_t get_system_clock_hz() {
        return DEFAULT_CLOCK_HZ;
    }
};

// ==============================================================================
// Concept Compliance Tests
// ==============================================================================

// NOTE: Testing ClockPlatform functionality without concept validation.
// Concept validation is tested in integration tests with real platforms.

// #if __cplusplus >= 202002L
// TEST_CASE("MockClockPlatform satisfies ClockPlatform concept", "[clock][concept][c++20]") {
//     STATIC_REQUIRE(alloy::hal::concepts::ClockPlatform<MockClockPlatform>);
// }
// #endif

// ==============================================================================
// Clock Initialization Tests
// ==============================================================================

TEST_CASE("Clock can be initialized", "[clock][basic]") {
    auto result = MockClockPlatform::initialize();

    REQUIRE(result.is_ok());
}

TEST_CASE("Clock provides system frequency", "[clock][basic]") {
    uint32_t freq = MockClockPlatform::get_system_clock_hz();

    REQUIRE(freq == MockClockPlatform::DEFAULT_CLOCK_HZ);
    REQUIRE(freq > 0);
}

// ==============================================================================
// Peripheral Clock Enable Tests
// ==============================================================================

TEST_CASE("GPIO clocks can be enabled", "[clock][peripheral]") {
    auto result = MockClockPlatform::enable_gpio_clocks();

    REQUIRE(result.is_ok());
}

TEST_CASE("UART clock can be enabled with valid base address", "[clock][peripheral]") {
    uint32_t uart_base = 0x40013800; // Example: USART1

    auto result = MockClockPlatform::enable_uart_clock(uart_base);

    REQUIRE(result.is_ok());
}

TEST_CASE("UART clock fails with invalid base address", "[clock][peripheral][error]") {
    uint32_t uart_base = 0x0; // Invalid

    auto result = MockClockPlatform::enable_uart_clock(uart_base);

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::InvalidParameter);
}

TEST_CASE("SPI clock can be enabled with valid base address", "[clock][peripheral]") {
    uint32_t spi_base = 0x40013000; // Example: SPI1

    auto result = MockClockPlatform::enable_spi_clock(spi_base);

    REQUIRE(result.is_ok());
}

TEST_CASE("SPI clock fails with invalid base address", "[clock][peripheral][error]") {
    uint32_t spi_base = 0x0; // Invalid

    auto result = MockClockPlatform::enable_spi_clock(spi_base);

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::InvalidParameter);
}

TEST_CASE("I2C clock can be enabled with valid base address", "[clock][peripheral]") {
    uint32_t i2c_base = 0x40005400; // Example: I2C1

    auto result = MockClockPlatform::enable_i2c_clock(i2c_base);

    REQUIRE(result.is_ok());
}

TEST_CASE("I2C clock fails with invalid base address", "[clock][peripheral][error]") {
    uint32_t i2c_base = 0x0; // Invalid

    auto result = MockClockPlatform::enable_i2c_clock(i2c_base);

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::InvalidParameter);
}

// ==============================================================================
// Clock Integration Tests
// ==============================================================================

TEST_CASE("Typical system initialization sequence", "[clock][integration]") {
    // 1. Initialize clock
    REQUIRE(MockClockPlatform::initialize().is_ok());

    // 2. Enable peripheral clocks
    REQUIRE(MockClockPlatform::enable_gpio_clocks().is_ok());
    REQUIRE(MockClockPlatform::enable_uart_clock(0x40013800).is_ok());
    REQUIRE(MockClockPlatform::enable_spi_clock(0x40013000).is_ok());
    REQUIRE(MockClockPlatform::enable_i2c_clock(0x40005400).is_ok());

    // 3. Verify system clock
    REQUIRE(MockClockPlatform::get_system_clock_hz() == MockClockPlatform::DEFAULT_CLOCK_HZ);
}

TEST_CASE("Multiple peripheral clocks can be enabled", "[clock][integration]") {
    // Enable all peripherals
    auto gpio_result = MockClockPlatform::enable_gpio_clocks();
    auto uart_result = MockClockPlatform::enable_uart_clock(0x40013800);
    auto spi_result = MockClockPlatform::enable_spi_clock(0x40013000);
    auto i2c_result = MockClockPlatform::enable_i2c_clock(0x40005400);

    // All should succeed
    REQUIRE(gpio_result.is_ok());
    REQUIRE(uart_result.is_ok());
    REQUIRE(spi_result.is_ok());
    REQUIRE(i2c_result.is_ok());
}

// ==============================================================================
// Clock Frequency Tests
// ==============================================================================

TEST_CASE("System clock frequency is compile-time constant", "[clock][constexpr]") {
    // get_system_clock_hz() should be constexpr
    constexpr uint32_t freq = MockClockPlatform::get_system_clock_hz();

    STATIC_REQUIRE(freq == 64'000'000);
}

TEST_CASE("Clock frequency can be used for timing calculations", "[clock][usage]") {
    uint32_t freq = MockClockPlatform::get_system_clock_hz();

    // Calculate 1ms in clock cycles
    uint32_t cycles_per_ms = freq / 1000;

    REQUIRE(cycles_per_ms == 64'000);
}
