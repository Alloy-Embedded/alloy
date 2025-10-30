/// Unit tests for host UART implementation
///
/// Tests the host UART using stdout/stdin simulation.

#include "hal/host/uart.hpp"
#include "core/units.hpp"
#include <gtest/gtest.h>

using namespace alloy::core;
using namespace alloy::hal;

// Test fixture for UART host tests
class UartHostTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/// Test: UART requires configuration before use
TEST_F(UartHostTest, RequiresConfiguration) {
    // Given: Unconfigured UART
    alloy::hal::host::Uart uart;

    // When/Then: read_byte() should fail with NotInitialized
    auto result = uart.read_byte();
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ErrorCode::NotInitialized);
}

/// Test: UART configuration succeeds with valid parameters
TEST_F(UartHostTest, ConfigurationSucceeds) {
    // Given: UART and valid config
    alloy::hal::host::Uart uart;
    UartConfig config{baud_rates::Baud115200};

    // When: Configuring UART
    auto result = uart.configure(config);

    // Then: Configuration should succeed
    EXPECT_TRUE(result.is_ok());
}

/// Test: UART rejects zero baud rate
TEST_F(UartHostTest, RejectsZeroBaudRate) {
    // Given: UART and invalid config (zero baud rate)
    alloy::hal::host::Uart uart;
    UartConfig config{BaudRate{0}};

    // When: Configuring with invalid baud rate
    auto result = uart.configure(config);

    // Then: Configuration should fail
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ErrorCode::InvalidParameter);
}

/// Test: Write byte succeeds after configuration
TEST_F(UartHostTest, WriteByteSucceeds) {
    // Given: Configured UART
    alloy::hal::host::Uart uart;
    auto config_result = uart.configure(UartConfig{baud_rates::Baud115200});
    ASSERT_TRUE(config_result.is_ok());

    // When: Writing a byte
    auto result = uart.write_byte('A');

    // Then: Write should succeed
    EXPECT_TRUE(result.is_ok());
}

/// Test: Write fails without configuration
TEST_F(UartHostTest, WriteFailsWithoutConfig) {
    // Given: Unconfigured UART
    alloy::hal::host::Uart uart;

    // When/Then: write_byte() should fail
    auto result = uart.write_byte('A');
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ErrorCode::NotInitialized);
}

/// Test: Read byte returns valid Result type
TEST_F(UartHostTest, ReadReturnsValidResult) {
    // Given: Configured UART
    alloy::hal::host::Uart uart;
    auto config_result = uart.configure(UartConfig{baud_rates::Baud115200});
    ASSERT_TRUE(config_result.is_ok());

    // When: Reading from stdin
    auto result = uart.read_byte();

    // Then: Should return either:
    // - Ok with data byte (if stdin has data)
    // - Timeout error (if no data available)
    // - HardwareError (if read fails)
    // This test validates the Result type works correctly
    if (result.is_ok()) {
        // Data was successfully read
        EXPECT_GE(result.value(), 0);
        EXPECT_LE(result.value(), 255);
    } else {
        // Error occurred - should be Timeout or HardwareError
        EXPECT_TRUE(result.error() == ErrorCode::Timeout ||
                   result.error() == ErrorCode::HardwareError);
    }
}

/// Test: Available returns sensible value
TEST_F(UartHostTest, AvailableReturnsSensibleValue) {
    // Given: Configured UART
    alloy::hal::host::Uart uart;
    auto config_result = uart.configure(UartConfig{baud_rates::Baud115200});
    ASSERT_TRUE(config_result.is_ok());

    // When/Then: available() should return a non-negative value
    auto count = uart.available();
    EXPECT_GE(count, 0);
}

/// Test: UART satisfies UartDevice concept
TEST(UartConceptTest, HostUartSatisfiesConcept) {
    // Compile-time check already in uart.hpp
    // This test just validates the concept compiles
    EXPECT_TRUE((UartDevice<alloy::hal::host::Uart>));
}

/// Test: BaudRate type safety
TEST(BaudRateTest, TypeSafety) {
    // Given: BaudRate values
    BaudRate rate1 = baud_rates::Baud9600;
    BaudRate rate2 = baud_rates::Baud115200;

    // Then: Values should be correct
    EXPECT_EQ(rate1.value(), 9600);
    EXPECT_EQ(rate2.value(), 115200);

    // And: Comparison should work
    EXPECT_NE(rate1, rate2);
    EXPECT_EQ(rate1, BaudRate{9600});
}

/// Test: BaudRate literals
TEST(BaudRateTest, Literals) {
    using namespace alloy::core::literals;

    // Given/When: Using literals
    auto rate1 = 9600_baud;
    auto rate2 = 115200_baud;

    // Then: Values should be correct
    EXPECT_EQ(rate1.value(), 9600);
    EXPECT_EQ(rate2.value(), 115200);
}

/// Test: UartConfig default values
TEST(UartConfigTest, DefaultValues) {
    // Given/When: Creating config with defaults
    UartConfig config{baud_rates::Baud115200};

    // Then: Should have 8N1 defaults
    EXPECT_EQ(config.data_bits, DataBits::Eight);
    EXPECT_EQ(config.parity, Parity::None);
    EXPECT_EQ(config.stop_bits, StopBits::One);
}

/// Test: UartConfig custom values
TEST(UartConfigTest, CustomValues) {
    // Given/When: Creating config with custom values
    UartConfig config{
        baud_rates::Baud9600,
        DataBits::Seven,
        Parity::Even,
        StopBits::Two
    };

    // Then: Values should match
    EXPECT_EQ(config.baud_rate, baud_rates::Baud9600);
    EXPECT_EQ(config.data_bits, DataBits::Seven);
    EXPECT_EQ(config.parity, Parity::Even);
    EXPECT_EQ(config.stop_bits, StopBits::Two);
}
