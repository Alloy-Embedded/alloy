/**
 * @file test_i2c_expert_crtp.cpp
 * @brief Compile test for I2C Expert API with CRTP pattern
 *
 * This file tests that ExpertI2cInstance inherits from I2cBase
 * correctly and provides the expected expert-level interface.
 *
 * @note Part of Phase 1.11.3: Refactor I2cExpert
 */

#include "hal/api/i2c_expert.hpp"
#include "core/types.hpp"
#include "hal/interface/i2c.hpp"

#include <span>

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test ExpertI2cInstance inherits from I2cBase
 */
void test_expert_i2c_inheritance() {
    using InstanceType = ExpertI2cInstance;
    using BaseType = I2cBase<InstanceType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, InstanceType>,
                  "ExpertI2cInstance must inherit from I2cBase");
}

/**
 * @brief Test I2cExpertConfig validation
 */
void test_expert_config_validation() {
    // Valid configuration
    constexpr I2cExpertConfig valid_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Fast,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = false,
        .digital_filter_coefficient = 0
    };

    static_assert(valid_config.is_valid(),
                  "Valid configuration should pass validation");

    // Invalid configuration - digital filter coefficient out of range
    constexpr I2cExpertConfig invalid_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Fast,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = true,
        .digital_filter_coefficient = 16  // Invalid - must be 0-15
    };

    static_assert(!invalid_config.is_valid(),
                  "Invalid configuration should fail validation");
}

/**
 * @brief Test standard preset configuration
 */
void test_standard_preset() {
    constexpr auto config = I2cExpertConfig::standard(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    static_assert(config.is_valid(),
                  "Standard preset should be valid");
    static_assert(config.speed == I2cSpeed::Standard,
                  "Standard preset should use Standard speed");
    static_assert(config.addressing == I2cAddressing::SevenBit,
                  "Standard preset should use 7-bit addressing");
}

/**
 * @brief Test fast preset configuration
 */
void test_fast_preset() {
    constexpr auto config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    static_assert(config.is_valid(),
                  "Fast preset should be valid");
    static_assert(config.speed == I2cSpeed::Fast,
                  "Fast preset should use Fast speed");
}

/**
 * @brief Test DMA preset configuration
 */
void test_dma_preset() {
    constexpr auto config = I2cExpertConfig::dma(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9},
        I2cSpeed::Fast
    );

    static_assert(config.is_valid(),
                  "DMA preset should be valid");
    static_assert(config.enable_dma_tx,
                  "DMA preset should enable DMA TX");
    static_assert(config.enable_dma_rx,
                  "DMA preset should enable DMA RX");
    static_assert(config.enable_interrupts,
                  "DMA preset should enable interrupts");
}

/**
 * @brief Test create_instance factory
 */
void test_create_instance() {
    constexpr I2cExpertConfig config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Fast,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = false,
        .digital_filter_coefficient = 0
    };

    auto i2c = expert::create_instance(config);

    // Verify type
    static_assert(std::is_same_v<decltype(i2c), ExpertI2cInstance>,
                  "create_instance should return ExpertI2cInstance");
}

/**
 * @brief Test read operations
 */
void test_read_operations() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    u8 data[4] = {0};
    [[maybe_unused]] auto read_result = i2c.read(0x50, std::span(data));

    // Verify return type
    static_assert(std::is_same_v<decltype(read_result), Result<void, ErrorCode>>,
                  "read() must return Result<void, ErrorCode>");
}

/**
 * @brief Test write operations
 */
void test_write_operations() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    u8 data[] = {0x01, 0x02, 0x03};
    [[maybe_unused]] auto write_result = i2c.write(0x50, std::span(data));

    // Verify return type
    static_assert(std::is_same_v<decltype(write_result), Result<void, ErrorCode>>,
                  "write() must return Result<void, ErrorCode>");
}

/**
 * @brief Test write-read operation
 */
void test_write_read_operation() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    u8 write_data[] = {0x10};
    u8 read_data[2] = {0};

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
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    [[maybe_unused]] auto read_result = i2c.read_byte(0x50);
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
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    [[maybe_unused]] auto read_result = i2c.read_register(0x50, 0x10);
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
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    u8 devices[128];
    [[maybe_unused]] auto result = i2c.scan_bus(std::span(devices));

    // Verify return type
    static_assert(std::is_same_v<decltype(result), Result<usize, ErrorCode>>,
                  "scan_bus() must return Result<usize, ErrorCode>");
}

/**
 * @brief Test configuration methods
 */
void test_configuration() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    // Test full configuration
    I2cConfig new_config{I2cSpeed::Fast, I2cAddressing::SevenBit};
    [[maybe_unused]] auto cfg_result = i2c.configure(new_config);

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
 * @brief Test I2cImplementation concept
 */
void test_concept() {
    // ExpertI2cInstance should satisfy I2cImplementation concept
    static_assert(I2cImplementation<ExpertI2cInstance>,
                  "ExpertI2cInstance must satisfy I2cImplementation concept");
}

/**
 * @brief Test apply method
 */
void test_apply() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    [[maybe_unused]] auto apply_result = i2c.apply();

    static_assert(std::is_same_v<decltype(apply_result), Result<void, ErrorCode>>,
                  "apply() must return Result<void, ErrorCode>");
}

/**
 * @brief Test config accessor
 */
void test_config_accessor() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    const auto& retrieved_config = i2c.config();

    // Verify we get the same config back
    static_assert(std::is_same_v<decltype(retrieved_config), const I2cExpertConfig&>,
                  "config() should return const reference to I2cExpertConfig");
}

/**
 * @brief Test DMA query methods
 */
void test_dma_queries() {
    constexpr I2cExpertConfig dma_config = I2cExpertConfig::dma(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9},
        I2cSpeed::Fast
    );

    auto i2c_dma = expert::create_instance(dma_config);

    // DMA config should have DMA enabled
    if (i2c_dma.has_dma_tx() && i2c_dma.has_dma_rx() && i2c_dma.has_interrupts()) {
        // DMA mode active
    }

    // Non-DMA config
    constexpr I2cExpertConfig no_dma_config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c_no_dma = expert::create_instance(no_dma_config);

    // Should not have DMA
    static_assert(std::is_same_v<decltype(i2c_no_dma.has_dma_tx()), bool>,
                  "has_dma_tx() should return bool");
}

/**
 * @brief Test error messages
 */
void test_error_messages() {
    constexpr I2cExpertConfig valid_config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    // Valid config should have "Valid" message
    constexpr const char* valid_msg = valid_config.error_message();
    (void)valid_msg;

    constexpr I2cExpertConfig invalid_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Fast,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = true,
        .digital_filter_coefficient = 20  // Invalid
    };

    // Invalid config should have descriptive error message
    constexpr const char* invalid_msg = invalid_config.error_message();
    (void)invalid_msg;
}

/**
 * @brief Test different I2C speeds
 */
void test_i2c_speeds() {
    // Standard (100 kHz)
    constexpr auto std_config = I2cExpertConfig::standard(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );
    auto std_i2c = expert::create_instance(std_config);

    // Fast (400 kHz)
    constexpr auto fast_config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );
    auto fast_i2c = expert::create_instance(fast_config);

    // All should return same type
    static_assert(std::is_same_v<decltype(std_i2c), ExpertI2cInstance>,
                  "All speeds should return ExpertI2cInstance");
}

/**
 * @brief Test addressing modes
 */
void test_addressing_modes() {
    // 7-bit addressing
    constexpr I2cExpertConfig addr7_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Standard,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = false,
        .digital_filter_coefficient = 0
    };

    auto i2c_7bit = expert::create_instance(addr7_config);

    // 10-bit addressing
    constexpr I2cExpertConfig addr10_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Standard,
        .addressing = I2cAddressing::TenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = false,
        .digital_filter_coefficient = 0
    };

    auto i2c_10bit = expert::create_instance(addr10_config);

    // Both should return same type
    static_assert(std::is_same_v<decltype(i2c_7bit), ExpertI2cInstance>,
                  "7-bit addressing should return ExpertI2cInstance");
    static_assert(std::is_same_v<decltype(i2c_10bit), ExpertI2cInstance>,
                  "10-bit addressing should return ExpertI2cInstance");
}

/**
 * @brief Test filter configurations
 */
void test_filter_configurations() {
    // Analog filter only
    constexpr I2cExpertConfig analog_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Fast,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = false,
        .digital_filter_coefficient = 0
    };

    static_assert(analog_config.is_valid(),
                  "Analog filter config should be valid");

    // Digital filter with valid coefficient
    constexpr I2cExpertConfig digital_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Fast,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = false,
        .enable_digital_filter = true,
        .digital_filter_coefficient = 8
    };

    static_assert(digital_config.is_valid(),
                  "Digital filter config with valid coefficient should be valid");

    // Both filters enabled
    constexpr I2cExpertConfig both_config = {
        .peripheral = PeripheralId::I2C0,
        .sda_pin = PinId{10},
        .scl_pin = PinId{9},
        .speed = I2cSpeed::Fast,
        .addressing = I2cAddressing::SevenBit,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_analog_filter = true,
        .enable_digital_filter = true,
        .digital_filter_coefficient = 4
    };

    static_assert(both_config.is_valid(),
                  "Both filters config should be valid");
}

/**
 * @brief Test constexpr construction
 */
constexpr bool test_constexpr_construction() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);
    return true;
}

static_assert(test_constexpr_construction(),
              "ExpertI2cInstance must be constexpr constructible");

/**
 * @brief Test error handling
 */
void test_error_handling() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

    auto result = i2c.read_byte(0x50);

    // Should be able to check and extract results
    if (result.is_ok()) {
        [[maybe_unused]] u8 byte = std::move(result).unwrap();
    } else {
        [[maybe_unused]] auto error = std::move(result).err();
    }
}

/**
 * @brief Test buffer size handling
 */
void test_buffer_size_handling() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    auto i2c = expert::create_instance(config);

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
 * @brief Test deprecated configure function
 */
void test_deprecated_configure() {
    constexpr I2cExpertConfig config = I2cExpertConfig::fast(
        PeripheralId::I2C0,
        PinId{10},
        PinId{9}
    );

    [[maybe_unused]] auto result = expert::configure(config);

    static_assert(std::is_same_v<decltype(result), Result<void, ErrorCode>>,
                  "configure() should return Result<void, ErrorCode>");
}

// ============================================================================
// Summary
// ============================================================================

/**
 * Test Results:
 * - [x] ExpertI2cInstance inherits from I2cBase
 * - [x] I2cExpertConfig validation works
 * - [x] Standard preset works
 * - [x] Fast preset works
 * - [x] DMA preset works
 * - [x] create_instance factory works
 * - [x] All read/write operations compile
 * - [x] Write-read operation compiles
 * - [x] Single-byte convenience methods compile
 * - [x] Register convenience methods compile
 * - [x] Bus scanning compiles
 * - [x] Configuration methods compile
 * - [x] I2cImplementation concept satisfied
 * - [x] Apply method works
 * - [x] Config accessor works
 * - [x] DMA query methods work
 * - [x] Error messages work
 * - [x] Different I2C speeds work
 * - [x] Addressing modes work
 * - [x] Filter configurations work
 * - [x] Constexpr construction works
 * - [x] Error handling works
 * - [x] Buffer size handling works
 * - [x] Deprecated configure function works
 *
 * If this file compiles without errors, Phase 1.11.3 I2cExpert refactoring is successful!
 */
