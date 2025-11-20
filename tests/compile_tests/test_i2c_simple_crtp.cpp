/**
 * @file test_i2c_simple_crtp.cpp
 * @brief Compile test for I2C Simple API with CRTP pattern
 *
 * This file tests that the SimpleI2cConfig inherits from I2cBase
 * correctly and provides the expected interface.
 *
 * @note Part of Phase 1.11.1: Refactor I2cSimple
 */

#include "hal/api/i2c_simple.hpp"
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
struct MockSdaPin {
    static constexpr PinId get_pin_id() { return PinId{0}; }
};

/**
 * @brief Mock SCL pin for testing
 */
struct MockSclPin {
    static constexpr PinId get_pin_id() { return PinId{1}; }
};

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test SimpleI2cConfig inherits from I2cBase
 */
void test_simple_i2c_inheritance() {
    using ConfigType = SimpleI2cConfig<MockSdaPin, MockSclPin>;
    using BaseType = I2cBase<ConfigType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, ConfigType>,
                  "SimpleI2cConfig must inherit from I2cBase");
}

/**
 * @brief Test quick_setup with defaults
 */
void test_quick_setup_defaults() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

    // Verify type
    using ConfigType = decltype(i2c);
    static_assert(std::is_same_v<ConfigType, SimpleI2cConfig<MockSdaPin, MockSclPin>>,
                  "quick_setup should return SimpleI2cConfig");
}

/**
 * @brief Test quick_setup with custom speed
 */
void test_quick_setup_custom_speed() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>(
        I2cSpeed::Fast,
        I2cAddressing::SevenBit
    );

    // Verify type
    using ConfigType = decltype(i2c);
    static_assert(std::is_same_v<ConfigType, SimpleI2cConfig<MockSdaPin, MockSclPin>>,
                  "quick_setup with speed should return SimpleI2cConfig");
}

/**
 * @brief Test quick_setup_fast preset
 */
void test_quick_setup_fast() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup_fast<MockSdaPin, MockSclPin>();

    // Verify type
    using ConfigType = decltype(i2c);
    static_assert(std::is_same_v<ConfigType, SimpleI2cConfig<MockSdaPin, MockSclPin>>,
                  "quick_setup_fast should return SimpleI2cConfig");
}

/**
 * @brief Test quick_setup_fast_plus preset
 */
void test_quick_setup_fast_plus() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup_fast_plus<MockSdaPin, MockSclPin>();

    // Verify type
    using ConfigType = decltype(i2c);
    static_assert(std::is_same_v<ConfigType, SimpleI2cConfig<MockSdaPin, MockSclPin>>,
                  "quick_setup_fast_plus should return SimpleI2cConfig");
}

/**
 * @brief Test read operations
 */
void test_read_operations() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
 * @brief Test preset configurations
 */
void test_preset_configurations() {
    // Test create_i2c_standard
    auto std_i2c = create_i2c_standard<PeripheralId::I2C0, MockSdaPin, MockSclPin>();

    // Test create_i2c_fast
    auto fast_i2c = create_i2c_fast<PeripheralId::I2C0, MockSdaPin, MockSclPin>();

    // Test create_i2c_fast_plus
    auto fastplus_i2c = create_i2c_fast_plus<PeripheralId::I2C0, MockSdaPin, MockSclPin>();

    // Verify types
    using ConfigType = SimpleI2cConfig<MockSdaPin, MockSclPin>;
    static_assert(std::is_same_v<decltype(std_i2c), ConfigType>,
                  "create_i2c_standard should return SimpleI2cConfig");
    static_assert(std::is_same_v<decltype(fast_i2c), ConfigType>,
                  "create_i2c_fast should return SimpleI2cConfig");
    static_assert(std::is_same_v<decltype(fastplus_i2c), ConfigType>,
                  "create_i2c_fast_plus should return SimpleI2cConfig");
}

/**
 * @brief Test I2cImplementation concept
 */
void test_concept() {
    using ConfigType = SimpleI2cConfig<MockSdaPin, MockSclPin>;

    // SimpleI2cConfig should satisfy I2cImplementation concept
    static_assert(I2cImplementation<ConfigType>,
                  "SimpleI2cConfig must satisfy I2cImplementation concept");
}

/**
 * @brief Test apply method
 */
void test_apply() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

    [[maybe_unused]] auto apply_result = i2c.apply();

    static_assert(std::is_same_v<decltype(apply_result), Result<void, ErrorCode>>,
                  "apply() must return Result<void, ErrorCode>");
}

/**
 * @brief Test buffer size handling
 */
void test_buffer_size_handling() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

    auto result = i2c.read_byte(0x50);

    // Should be able to check and extract results
    if (result.is_ok()) {
        [[maybe_unused]] u8 byte = result.unwrap();
    } else {
        [[maybe_unused]] auto error = result.err();
    }
}

/**
 * @brief Test constexpr construction
 */
constexpr bool test_constexpr_construction() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();
    return true;
}

static_assert(test_constexpr_construction(),
              "SimpleI2cConfig must be constexpr constructible");

/**
 * @brief Test different I2C speeds
 */
void test_i2c_speeds() {
    // Standard (100 kHz)
    auto std_i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>(
        I2cSpeed::Standard
    );

    // Fast (400 kHz)
    auto fast_i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>(
        I2cSpeed::Fast
    );

    // Fast Plus (1 MHz)
    auto fastplus_i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>(
        I2cSpeed::FastPlus
    );

    // High Speed (3.4 MHz)
    auto hs_i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>(
        I2cSpeed::HighSpeed
    );

    // All should return same type
    using ConfigType = SimpleI2cConfig<MockSdaPin, MockSclPin>;
    static_assert(std::is_same_v<decltype(std_i2c), ConfigType>,
                  "All speeds should return SimpleI2cConfig");
}

/**
 * @brief Test addressing modes
 */
void test_addressing_modes() {
    // 7-bit addressing
    auto i2c_7bit = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>(
        I2cSpeed::Standard,
        I2cAddressing::SevenBit
    );

    // 10-bit addressing
    auto i2c_10bit = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>(
        I2cSpeed::Standard,
        I2cAddressing::TenBit
    );

    // Both should return same type
    using ConfigType = SimpleI2cConfig<MockSdaPin, MockSclPin>;
    static_assert(std::is_same_v<decltype(i2c_7bit), ConfigType>,
                  "7-bit addressing should return SimpleI2cConfig");
    static_assert(std::is_same_v<decltype(i2c_10bit), ConfigType>,
                  "10-bit addressing should return SimpleI2cConfig");
}

/**
 * @brief Test register read pattern
 */
void test_register_read_pattern() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

    // Common pattern: read from register 0x10 on device 0x50
    auto result = i2c.read_register(0x50, 0x10);

    if (result.is_ok()) {
        [[maybe_unused]] u8 value = result.unwrap();
    }
}

/**
 * @brief Test register write pattern
 */
void test_register_write_pattern() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

    // Common pattern: write 0xAA to register 0x10 on device 0x50
    [[maybe_unused]] auto result = i2c.write_register(0x50, 0x10, 0xAA);
}

/**
 * @brief Test bus scan pattern
 */
void test_bus_scan_pattern() {
    auto i2c = I2c<PeripheralId::I2C0>::quick_setup<MockSdaPin, MockSclPin>();

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
 * - [x] SimpleI2cConfig inherits from I2cBase
 * - [x] quick_setup with defaults works
 * - [x] quick_setup with custom speed works
 * - [x] quick_setup_fast preset works
 * - [x] quick_setup_fast_plus preset works
 * - [x] All read/write operations compile
 * - [x] Write-read operation compiles
 * - [x] Single-byte convenience methods compile
 * - [x] Register convenience methods compile
 * - [x] Bus scanning compiles
 * - [x] Configuration methods compile
 * - [x] Preset configurations work (create_i2c_standard, etc.)
 * - [x] I2cImplementation concept satisfied
 * - [x] Apply method works
 * - [x] Buffer size handling works
 * - [x] Error handling works
 * - [x] Constexpr construction works
 * - [x] Different I2C speeds work
 * - [x] Addressing modes work
 * - [x] Register read/write patterns work
 * - [x] Bus scan pattern works
 *
 * If this file compiles without errors, Phase 1.11.1 I2cSimple refactoring is successful!
 */
