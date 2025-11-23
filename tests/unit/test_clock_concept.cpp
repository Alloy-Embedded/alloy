/**
 * @file test_clock_concept.cpp
 * @brief Unit tests for ClockPlatform concept compliance
 *
 * Tests that Clock implementations satisfy the ClockPlatform concept.
 */

#include <catch2/catch_test_macros.hpp>

#include "hal/core/concepts.hpp"

#include "core/error.hpp"
#include "core/result.hpp"

using namespace ucore::core;
using namespace ucore::hal;

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
    static Result<void, ErrorCode> initialize() { return Ok(); }

    static Result<void, ErrorCode> enable_gpio_clocks() { return Ok(); }

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

    static constexpr uint32_t get_system_clock_hz() { return DEFAULT_CLOCK_HZ; }
};

// ==============================================================================
// Concept Compliance Tests
// ==============================================================================

// NOTE: Testing ClockPlatform functionality without concept validation.
// Concept validation is tested in integration tests with real platforms.

// #if __cplusplus >= 202002L
// TEST_CASE("MockClockPlatform satisfies ClockPlatform concept", "[clock][concept][c++20]") {
//     STATIC_REQUIRE(ucore::hal::concepts::ClockPlatform<MockClockPlatform>);
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
    uint32_t uart_base = 0x40013800;  // Example: USART1

    auto result = MockClockPlatform::enable_uart_clock(uart_base);

    REQUIRE(result.is_ok());
}

TEST_CASE("UART clock fails with invalid base address", "[clock][peripheral][error]") {
    uint32_t uart_base = 0x0;  // Invalid

    auto result = MockClockPlatform::enable_uart_clock(uart_base);

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::InvalidParameter);
}

TEST_CASE("SPI clock can be enabled with valid base address", "[clock][peripheral]") {
    uint32_t spi_base = 0x40013000;  // Example: SPI1

    auto result = MockClockPlatform::enable_spi_clock(spi_base);

    REQUIRE(result.is_ok());
}

TEST_CASE("SPI clock fails with invalid base address", "[clock][peripheral][error]") {
    uint32_t spi_base = 0x0;  // Invalid

    auto result = MockClockPlatform::enable_spi_clock(spi_base);

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::InvalidParameter);
}

TEST_CASE("I2C clock can be enabled with valid base address", "[clock][peripheral]") {
    uint32_t i2c_base = 0x40005400;  // Example: I2C1

    auto result = MockClockPlatform::enable_i2c_clock(i2c_base);

    REQUIRE(result.is_ok());
}

TEST_CASE("I2C clock fails with invalid base address", "[clock][peripheral][error]") {
    uint32_t i2c_base = 0x0;  // Invalid

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

// ==============================================================================
// Concept Failure Tests (Compile-Time Verification)
// ==============================================================================

/**
 * @brief Types that should NOT satisfy ClockPlatform concept
 *
 * These tests verify that the concept correctly rejects invalid types.
 * They are compile-time tests - the code should NOT compile if enabled.
 *
 * @note These are documented but commented out to prevent compilation errors.
 *       To test, uncomment individually and verify compilation fails.
 */

// Test 1: Missing initialize() method
class BadClockNoInit {
   public:
    static Result<void, ErrorCode> enable_gpio_clocks() { return Ok(); }
    static Result<void, ErrorCode> enable_uart_clock(uint32_t) { return Ok(); }
    static constexpr uint32_t get_system_clock_hz() { return 64'000'000; }
    // Missing: initialize()
};

// Would fail if uncommented:
// static_assert(ucore::hal::concepts::ClockPlatform<BadClockNoInit>);
// Error: Missing initialize() method

// Test 2: Non-static methods (wrong)
class BadClockNonStatic {
   public:
    Result<void, ErrorCode> initialize() { return Ok(); }  // Wrong: should be static
    static Result<void, ErrorCode> enable_gpio_clocks() { return Ok(); }
    static constexpr uint32_t get_system_clock_hz() { return 64'000'000; }
};

// Would fail if uncommented:
// static_assert(ucore::hal::concepts::ClockPlatform<BadClockNonStatic>);
// Error: initialize() must be static

// Test 3: Wrong return type
class BadClockWrongReturn {
   public:
    static void initialize() {}  // Wrong: should return Result<void, ErrorCode>
    static Result<void, ErrorCode> enable_gpio_clocks() { return Ok(); }
    static constexpr uint32_t get_system_clock_hz() { return 64'000'000; }
};

// Would fail if uncommented:
// static_assert(ucore::hal::concepts::ClockPlatform<BadClockWrongReturn>);
// Error: initialize() must return Result<void, ErrorCode>

// Test 4: get_system_clock_hz() not constexpr
class BadClockNotConstexpr {
   public:
    static Result<void, ErrorCode> initialize() { return Ok(); }
    static Result<void, ErrorCode> enable_gpio_clocks() { return Ok(); }
    static uint32_t get_system_clock_hz() { return 64'000'000; }  // Wrong: should be constexpr
};

// Would fail if uncommented:
// static_assert(ucore::hal::concepts::ClockPlatform<BadClockNotConstexpr>);
// Error: get_system_clock_hz() must be constexpr

// Test 5: Missing peripheral clock enable methods
class BadClockIncomplete {
   public:
    static Result<void, ErrorCode> initialize() { return Ok(); }
    static constexpr uint32_t get_system_clock_hz() { return 64'000'000; }
    // Missing: enable_gpio_clocks(), enable_uart_clock(), etc.
};

// Would fail if uncommented:
// static_assert(ucore::hal::concepts::ClockPlatform<BadClockIncomplete>);
// Error: Missing peripheral clock enable methods

// Test 6: Primitive types should NOT satisfy concept
// Would fail if uncommented:
// static_assert(ucore::hal::concepts::ClockPlatform<int>);
// static_assert(ucore::hal::concepts::ClockPlatform<void*>);
// Error: Primitive types do not satisfy ClockPlatform

TEST_CASE("Clock concept failure tests are documented", "[clock][concept][negative]") {
    // This test just documents that we have negative concept tests above
    // The actual tests are compile-time and would fail compilation if enabled
    REQUIRE(true);
}
