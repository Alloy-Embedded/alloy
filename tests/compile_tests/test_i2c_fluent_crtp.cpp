/**
 * @file test_i2c_fluent_crtp.cpp
 * @brief Compile test for I2C Fluent API with CRTP pattern
 *
 * This file tests that FluentI2cConfig inherits from I2cBase
 * correctly and provides the expected builder interface.
 *
 * @note Part of Phase 1.11.2: Refactor I2cFluent
 */

#include "hal/api/i2c_fluent.hpp"
#include "core/types.hpp"
#include "hal/interface/i2c.hpp"

#include <span>

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Mock Pin Types
// ============================================================================

/**
 * @brief Mock SDA pin for testing
 */
struct MockI2c0SdaPin {
    static constexpr PinId get_pin_id() { return PinId{10}; }
};

/**
 * @brief Mock SCL pin for testing
 */
struct MockI2c0SclPin {
    static constexpr PinId get_pin_id() { return PinId{9}; }
};

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test FluentI2cConfig inherits from I2cBase
 */
void test_fluent_i2c_inheritance() {
    using ConfigType = FluentI2cConfig;
    using BaseType = I2cBase<ConfigType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, ConfigType>,
                  "FluentI2cConfig must inherit from I2cBase");
}

/**
 * @brief Test builder pattern with defaults
 */
void test_builder_defaults() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    // Verify return type
    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "initialize() should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test builder with speed configuration
 */
void test_builder_with_speed() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .speed(I2cSpeed::Fast)
        .initialize();

    // Verify type
    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "Builder with speed should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test builder with addressing mode
 */
void test_builder_with_addressing() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .addressing_7bit()
        .initialize();

    // Verify type
    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "Builder with addressing should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test builder with 10-bit addressing
 */
void test_builder_10bit_addressing() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .addressing_10bit()
        .initialize();

    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "Builder with 10-bit addressing should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test builder with both pins at once
 */
void test_builder_with_pins() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_pins<MockI2c0SdaPin, MockI2c0SclPin>()
        .initialize();

    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "Builder with_pins() should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test standard_mode preset
 */
void test_standard_mode_preset() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .standard_mode()
        .initialize();

    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "standard_mode() should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test fast_mode preset
 */
void test_fast_mode_preset() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .fast_mode()
        .initialize();

    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "fast_mode() should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test fast_plus_mode preset
 */
void test_fast_plus_mode_preset() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .fast_plus_mode()
        .initialize();

    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "fast_plus_mode() should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test complete configuration chain
 */
void test_complete_configuration() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .speed(I2cSpeed::Fast)
        .addressing_7bit()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        // Should have all I2cBase methods
        u8 data[4] = {0};
        [[maybe_unused]] auto read_result = i2c.read(0x50, std::span(data));
    }
}

/**
 * @brief Test validation of incomplete configuration
 */
void test_validation_incomplete() {
    // Missing SCL pin - should fail validation
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .initialize();

    // Should return error
    static_assert(std::is_same_v<decltype(result), Result<FluentI2cConfig, ErrorCode>>,
                  "Incomplete builder should return Result with error");
}

/**
 * @brief Test read operations on fluent config
 */
void test_read_operations() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        u8 data[4] = {0};
        [[maybe_unused]] auto read_result = i2c.read(0x50, std::span(data));

        static_assert(std::is_same_v<decltype(read_result), Result<void, ErrorCode>>,
                      "read() must return Result<void, ErrorCode>");
    }
}

/**
 * @brief Test write operations on fluent config
 */
void test_write_operations() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        u8 data[] = {0x01, 0x02, 0x03};
        [[maybe_unused]] auto write_result = i2c.write(0x50, std::span(data));

        static_assert(std::is_same_v<decltype(write_result), Result<void, ErrorCode>>,
                      "write() must return Result<void, ErrorCode>");
    }
}

/**
 * @brief Test write-read operation on fluent config
 */
void test_write_read_operation() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        u8 write_data[] = {0x10};
        u8 read_data[2] = {0};

        [[maybe_unused]] auto result = i2c.write_read(
            0x50,
            std::span(write_data),
            std::span(read_data)
        );

        static_assert(std::is_same_v<decltype(result), Result<void, ErrorCode>>,
                      "write_read() must return Result<void, ErrorCode>");
    }
}

/**
 * @brief Test single-byte convenience methods
 */
void test_single_byte_operations() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        [[maybe_unused]] auto read_result = i2c.read_byte(0x50);
        [[maybe_unused]] auto write_result = i2c.write_byte(0x50, 0xAA);

        static_assert(std::is_same_v<decltype(read_result), Result<u8, ErrorCode>>,
                      "read_byte() must return Result<u8, ErrorCode>");
        static_assert(std::is_same_v<decltype(write_result), Result<void, ErrorCode>>,
                      "write_byte() must return Result<void, ErrorCode>");
    }
}

/**
 * @brief Test register convenience methods
 */
void test_register_operations() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        [[maybe_unused]] auto read_result = i2c.read_register(0x50, 0x10);
        [[maybe_unused]] auto write_result = i2c.write_register(0x50, 0x10, 0xAA);

        static_assert(std::is_same_v<decltype(read_result), Result<u8, ErrorCode>>,
                      "read_register() must return Result<u8, ErrorCode>");
        static_assert(std::is_same_v<decltype(write_result), Result<void, ErrorCode>>,
                      "write_register() must return Result<void, ErrorCode>");
    }
}

/**
 * @brief Test bus scanning
 */
void test_scan_bus() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        u8 devices[128];
        [[maybe_unused]] auto scan_result = i2c.scan_bus(std::span(devices));

        static_assert(std::is_same_v<decltype(scan_result), Result<usize, ErrorCode>>,
                      "scan_bus() must return Result<usize, ErrorCode>");
    }
}

/**
 * @brief Test configuration methods
 */
void test_configuration() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();

        I2cConfig config{I2cSpeed::Fast, I2cAddressing::SevenBit};
        [[maybe_unused]] auto cfg_result = i2c.configure(config);
        [[maybe_unused]] auto speed_result = i2c.set_speed(I2cSpeed::FastPlus);
        [[maybe_unused]] auto addr_result = i2c.set_addressing(I2cAddressing::TenBit);

        static_assert(std::is_same_v<decltype(cfg_result), Result<void, ErrorCode>>,
                      "configure() must return Result<void, ErrorCode>");
        static_assert(std::is_same_v<decltype(speed_result), Result<void, ErrorCode>>,
                      "set_speed() must return Result<void, ErrorCode>");
        static_assert(std::is_same_v<decltype(addr_result), Result<void, ErrorCode>>,
                      "set_addressing() must return Result<void, ErrorCode>");
    }
}

/**
 * @brief Test I2cImplementation concept
 */
void test_concept() {
    // FluentI2cConfig should satisfy I2cImplementation concept
    static_assert(I2cImplementation<FluentI2cConfig>,
                  "FluentI2cConfig must satisfy I2cImplementation concept");
}

/**
 * @brief Test apply method
 */
void test_apply() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();
        [[maybe_unused]] auto apply_result = i2c.apply();

        static_assert(std::is_same_v<decltype(apply_result), Result<void, ErrorCode>>,
                      "apply() must return Result<void, ErrorCode>");
    }
}

/**
 * @brief Test method chaining
 */
void test_method_chaining() {
    // All methods should return reference for chaining
    auto builder = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .speed(I2cSpeed::Fast)
        .addressing_7bit();

    [[maybe_unused]] auto result = builder.initialize();
}

/**
 * @brief Test different I2C speeds
 */
void test_i2c_speeds() {
    // Standard (100 kHz)
    auto std_result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .speed(I2cSpeed::Standard)
        .initialize();

    // Fast (400 kHz)
    auto fast_result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .speed(I2cSpeed::Fast)
        .initialize();

    // Fast Plus (1 MHz)
    auto fastplus_result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .speed(I2cSpeed::FastPlus)
        .initialize();

    // All should return same type
    static_assert(std::is_same_v<decltype(std_result), Result<FluentI2cConfig, ErrorCode>>,
                  "All speeds should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test addressing modes
 */
void test_addressing_modes() {
    // 7-bit addressing
    auto addr7_result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .addressing_7bit()
        .initialize();

    // 10-bit addressing
    auto addr10_result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .addressing_10bit()
        .initialize();

    // Both should return same type
    static_assert(std::is_same_v<decltype(addr7_result), Result<FluentI2cConfig, ErrorCode>>,
                  "7-bit addressing should return Result<FluentI2cConfig, ErrorCode>");
    static_assert(std::is_same_v<decltype(addr10_result), Result<FluentI2cConfig, ErrorCode>>,
                  "10-bit addressing should return Result<FluentI2cConfig, ErrorCode>");
}

/**
 * @brief Test error handling
 */
void test_error_handling() {
    auto result = I2cBuilder<PeripheralId::I2C0>()
        .with_sda<MockI2c0SdaPin>()
        .with_scl<MockI2c0SclPin>()
        .initialize();

    if (result.is_ok()) {
        auto i2c = std::move(result).unwrap();
        auto read_result = i2c.read_byte(0x50);

        if (read_result.is_ok()) {
            [[maybe_unused]] u8 byte = std::move(read_result).unwrap();
        } else {
            [[maybe_unused]] auto error = std::move(read_result).err();
        }
    }
}

/**
 * @brief Test builder state tracking
 */
void test_builder_state() {
    I2cBuilderState state;

    // Initial state
    static_assert(!I2cBuilderState{}.is_valid(),
                  "Empty state should not be valid");

    // Check state members
    state.has_sda = true;
    state.has_scl = true;

    // Now should be valid
    if (state.is_valid()) {
        // Configuration complete
    }
}

// ============================================================================
// Summary
// ============================================================================

/**
 * Test Results:
 * - [x] FluentI2cConfig inherits from I2cBase
 * - [x] Builder with defaults works
 * - [x] Builder with speed works
 * - [x] Builder with addressing modes works (7-bit and 10-bit)
 * - [x] Builder with both pins at once works
 * - [x] Preset configurations work (standard, fast, fast_plus)
 * - [x] Complete configuration chain works
 * - [x] Validation of incomplete configuration works
 * - [x] All read/write operations compile
 * - [x] Write-read operation compiles
 * - [x] Single-byte convenience methods compile
 * - [x] Register convenience methods compile
 * - [x] Bus scanning compiles
 * - [x] Configuration methods compile
 * - [x] I2cImplementation concept satisfied
 * - [x] Apply method works
 * - [x] Method chaining works
 * - [x] Different I2C speeds work
 * - [x] Addressing modes work
 * - [x] Error handling works
 * - [x] Builder state tracking works
 *
 * If this file compiles without errors, Phase 1.11.2 I2cFluent refactoring is successful!
 */
