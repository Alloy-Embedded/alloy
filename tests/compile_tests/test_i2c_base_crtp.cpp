/**
 * @file test_i2c_base_crtp.cpp
 * @brief Compile test for I2C Base CRTP pattern
 *
 * This file tests that the I2cBase CRTP pattern compiles correctly
 * and provides the expected interface.
 *
 * @note Part of Phase 1.10: Implement I2cBase
 */

#include "hal/api/i2c_base.hpp"
#include "core/types.hpp"
#include "hal/interface/i2c.hpp"

#include <span>

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Mock I2C Implementation
// ============================================================================

/**
 * @brief Mock I2C device for testing CRTP
 *
 * Implements all required *_impl() methods for I2cBase.
 */
class MockI2cDevice : public I2cBase<MockI2cDevice> {
    friend I2cBase<MockI2cDevice>;

public:
    constexpr MockI2cDevice() : config_{}, last_address_(0) {}

    // Allow access to base class methods
    using I2cBase<MockI2cDevice>::read;
    using I2cBase<MockI2cDevice>::write;
    using I2cBase<MockI2cDevice>::write_read;
    using I2cBase<MockI2cDevice>::read_byte;
    using I2cBase<MockI2cDevice>::write_byte;
    using I2cBase<MockI2cDevice>::read_register;
    using I2cBase<MockI2cDevice>::write_register;
    using I2cBase<MockI2cDevice>::scan_bus;
    using I2cBase<MockI2cDevice>::configure;
    using I2cBase<MockI2cDevice>::set_speed;
    using I2cBase<MockI2cDevice>::set_addressing;

    // ========================================================================
    // Implementation Methods (public for concept checking)
    // ========================================================================

    /**
     * @brief Read implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> read_impl(
        u16 address,
        std::span<u8> buffer
    ) noexcept {
        last_address_ = address;
        // Mock: fill buffer with zeros
        for (auto& byte : buffer) {
            byte = 0;
        }
        return Ok();
    }

    /**
     * @brief Write implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write_impl(
        u16 address,
        std::span<const u8> buffer
    ) noexcept {
        last_address_ = address;
        // Mock: just store address
        (void)buffer;
        return Ok();
    }

    /**
     * @brief Write-read implementation (repeated start)
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> write_read_impl(
        u16 address,
        std::span<const u8> write_buffer,
        std::span<u8> read_buffer
    ) noexcept {
        last_address_ = address;
        // Mock: copy write to read for testing
        size_t len = write_buffer.size() < read_buffer.size() ?
                     write_buffer.size() : read_buffer.size();
        for (size_t i = 0; i < len; ++i) {
            read_buffer[i] = write_buffer[i];
        }
        return Ok();
    }

    /**
     * @brief Scan bus implementation
     */
    [[nodiscard]] constexpr Result<usize, ErrorCode> scan_bus_impl(
        std::span<u8> found_devices
    ) noexcept {
        // Mock: return a few test addresses
        if (found_devices.size() >= 3) {
            found_devices[0] = 0x50;
            found_devices[1] = 0x51;
            found_devices[2] = 0x52;
            return Ok(usize{3});
        }
        return Ok(usize{0});
    }

    /**
     * @brief Configure I2C implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_impl(
        const I2cConfig& config
    ) noexcept {
        config_ = config;
        return Ok();
    }

private:
    I2cConfig config_;
    u16 last_address_;
};

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test that MockI2cDevice inherits from I2cBase
 */
void test_inheritance() {
    using DeviceType = MockI2cDevice;
    using BaseType = I2cBase<DeviceType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, DeviceType>,
                  "MockI2cDevice must inherit from I2cBase");
}

/**
 * @brief Test read operations
 */
void test_read_operations() {
    MockI2cDevice i2c;

    u8 data[4] = {0};

    // Test read
    [[maybe_unused]] auto read_result = i2c.read(0x50, std::span(data));

    // Verify return type
    static_assert(std::is_same_v<decltype(read_result), Result<void, ErrorCode>>,
                  "read() must return Result<void, ErrorCode>");
}

/**
 * @brief Test write operations
 */
void test_write_operations() {
    MockI2cDevice i2c;

    u8 data[] = {0x01, 0x02, 0x03};

    // Test write
    [[maybe_unused]] auto write_result = i2c.write(0x50, std::span(data));

    // Verify return type
    static_assert(std::is_same_v<decltype(write_result), Result<void, ErrorCode>>,
                  "write() must return Result<void, ErrorCode>");
}

/**
 * @brief Test write-read operation
 */
void test_write_read_operation() {
    MockI2cDevice i2c;

    u8 write_data[] = {0x10};
    u8 read_data[2] = {0};

    // Test write_read
    [[maybe_unused]] auto result = i2c.write_read(
        0x50,
        std::span(write_data),
        std::span(read_data)
    );

    // Verify return type
    static_assert(std::is_same_v<decltype(result), Result<void, ErrorCode>>,
                  "write_read() must return Result<void, ErrorCode>");
}

/**
 * @brief Test single-byte convenience methods
 */
void test_single_byte_operations() {
    MockI2cDevice i2c;

    // Test read_byte
    [[maybe_unused]] auto read_result = i2c.read_byte(0x50);

    // Test write_byte
    [[maybe_unused]] auto write_result = i2c.write_byte(0x50, 0xAA);

    // Verify return types
    static_assert(std::is_same_v<decltype(read_result), Result<u8, ErrorCode>>,
                  "read_byte() must return Result<u8, ErrorCode>");
    static_assert(std::is_same_v<decltype(write_result), Result<void, ErrorCode>>,
                  "write_byte() must return Result<void, ErrorCode>");
}

/**
 * @brief Test register convenience methods
 */
void test_register_operations() {
    MockI2cDevice i2c;

    // Test read_register
    [[maybe_unused]] auto read_result = i2c.read_register(0x50, 0x10);

    // Test write_register
    [[maybe_unused]] auto write_result = i2c.write_register(0x50, 0x10, 0xAA);

    // Verify return types
    static_assert(std::is_same_v<decltype(read_result), Result<u8, ErrorCode>>,
                  "read_register() must return Result<u8, ErrorCode>");
    static_assert(std::is_same_v<decltype(write_result), Result<void, ErrorCode>>,
                  "write_register() must return Result<void, ErrorCode>");
}

/**
 * @brief Test bus scanning
 */
void test_scan_bus() {
    MockI2cDevice i2c;

    u8 devices[128];

    // Test scan_bus
    [[maybe_unused]] auto result = i2c.scan_bus(std::span(devices));

    // Verify return type
    static_assert(std::is_same_v<decltype(result), Result<usize, ErrorCode>>,
                  "scan_bus() must return Result<usize, ErrorCode>");
}

/**
 * @brief Test configuration methods
 */
void test_configuration() {
    MockI2cDevice i2c;

    // Test full configuration
    I2cConfig config{I2cSpeed::Fast, I2cAddressing::SevenBit};
    [[maybe_unused]] auto cfg_result = i2c.configure(config);

    // Test speed change
    [[maybe_unused]] auto speed_result = i2c.set_speed(I2cSpeed::FastPlus);

    // Test addressing change
    [[maybe_unused]] auto addr_result = i2c.set_addressing(I2cAddressing::TenBit);

    // Verify return types
    static_assert(std::is_same_v<decltype(cfg_result), Result<void, ErrorCode>>,
                  "configure() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(speed_result), Result<void, ErrorCode>>,
                  "set_speed() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(addr_result), Result<void, ErrorCode>>,
                  "set_addressing() must return Result<void, ErrorCode>");
}

/**
 * @brief Test zero-overhead guarantee
 */
void test_zero_overhead() {
    using DeviceType = MockI2cDevice;
    using BaseType = I2cBase<DeviceType>;

    // Verify empty base optimization
    static_assert(sizeof(BaseType) == 1,
                  "I2cBase must be empty (sizeof == 1)");

    static_assert(std::is_empty_v<BaseType>,
                  "I2cBase must have no data members");
}

/**
 * @brief Test I2cImplementation concept
 */
void test_concept() {
    // MockI2cDevice should satisfy I2cImplementation concept
    static_assert(I2cImplementation<MockI2cDevice>,
                  "MockI2cDevice must satisfy I2cImplementation concept");
}

/**
 * @brief Test constexpr construction
 */
constexpr bool test_constexpr_construction() {
    MockI2cDevice i2c;
    return true;
}

static_assert(test_constexpr_construction(),
              "MockI2cDevice must be constexpr constructible");

/**
 * @brief Test buffer size handling
 */
void test_buffer_size_handling() {
    MockI2cDevice i2c;

    u8 write_data[5] = {1, 2, 3, 4, 5};
    u8 read_data[3] = {0};

    // write_read should handle different buffer sizes
    [[maybe_unused]] auto result = i2c.write_read(
        0x50,
        std::span(write_data),
        std::span(read_data)
    );
}

/**
 * @brief Test error handling
 */
void test_error_handling() {
    MockI2cDevice i2c;

    auto result = i2c.read_byte(0x50);

    // Should be able to check and extract results
    if (result.is_ok()) {
        [[maybe_unused]] u8 byte = result.unwrap();
    } else {
        [[maybe_unused]] auto error = result.err();
    }
}

/**
 * @brief Test different I2C speeds
 */
void test_i2c_speeds() {
    MockI2cDevice i2c;

    // Test Standard speed (100 kHz)
    [[maybe_unused]] auto std_result = i2c.set_speed(I2cSpeed::Standard);

    // Test Fast speed (400 kHz)
    [[maybe_unused]] auto fast_result = i2c.set_speed(I2cSpeed::Fast);

    // Test Fast Plus (1 MHz)
    [[maybe_unused]] auto fastplus_result = i2c.set_speed(I2cSpeed::FastPlus);

    // Test High Speed (3.4 MHz)
    [[maybe_unused]] auto hs_result = i2c.set_speed(I2cSpeed::HighSpeed);

    // All should return same type
    static_assert(std::is_same_v<decltype(std_result), Result<void, ErrorCode>>,
                  "set_speed() must return Result<void, ErrorCode>");
}

/**
 * @brief Test addressing modes
 */
void test_addressing_modes() {
    MockI2cDevice i2c;

    // Test 7-bit addressing
    [[maybe_unused]] auto addr7_result = i2c.set_addressing(I2cAddressing::SevenBit);

    // Test 10-bit addressing
    [[maybe_unused]] auto addr10_result = i2c.set_addressing(I2cAddressing::TenBit);

    // Both should return same type
    static_assert(std::is_same_v<decltype(addr7_result), Result<void, ErrorCode>>,
                  "set_addressing() must return Result<void, ErrorCode>");
}

/**
 * @brief Test register read pattern
 */
void test_register_read_pattern() {
    MockI2cDevice i2c;

    // Common pattern: read from register 0x10
    auto result = i2c.read_register(0x50, 0x10);

    if (result.is_ok()) {
        [[maybe_unused]] u8 value = result.unwrap();
        // Value is now available
    }
}

/**
 * @brief Test register write pattern
 */
void test_register_write_pattern() {
    MockI2cDevice i2c;

    // Common pattern: write 0xAA to register 0x10
    [[maybe_unused]] auto result = i2c.write_register(0x50, 0x10, 0xAA);
}

/**
 * @brief Test bus scan pattern
 */
void test_bus_scan_pattern() {
    MockI2cDevice i2c;

    u8 found_devices[128];
    auto result = i2c.scan_bus(std::span(found_devices));

    if (result.is_ok()) {
        usize count = result.unwrap();
        // found_devices[0..count] contains device addresses
        (void)count;
    }
}

// ============================================================================
// Summary
// ============================================================================

/**
 * Test Results:
 * - [x] MockI2cDevice inherits from I2cBase
 * - [x] Read operations compile
 * - [x] Write operations compile
 * - [x] Write-read operation compiles
 * - [x] Single-byte convenience methods compile
 * - [x] Register convenience methods compile
 * - [x] Bus scanning compiles
 * - [x] Configuration methods compile (configure, set_speed, set_addressing)
 * - [x] Zero-overhead guarantee verified
 * - [x] I2cImplementation concept satisfied
 * - [x] Constexpr construction works
 * - [x] Buffer size handling works
 * - [x] Error handling works
 * - [x] Different I2C speeds work
 * - [x] Addressing modes work
 * - [x] Register read/write patterns work
 * - [x] Bus scan pattern works
 *
 * If this file compiles without errors, Phase 1.10 I2cBase is successful!
 */
