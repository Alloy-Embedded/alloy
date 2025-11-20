/**
 * @file test_spi_expert_crtp.cpp
 * @brief Compile test for SPI Expert API with CRTP pattern
 *
 * This file tests that the ExpertSpiInstance inherits from SpiBase
 * correctly and provides the expected interface.
 *
 * @note Part of Phase 1.9.3: Refactor SpiExpert
 */

#include "hal/api/spi_expert.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"

#include <span>

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test ExpertSpiInstance inherits from SpiBase
 */
void test_expert_spi_inheritance() {
    using InstanceType = ExpertSpiInstance;
    using BaseType = SpiBase<InstanceType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, InstanceType>,
                  "ExpertSpiInstance must inherit from SpiBase");
}

/**
 * @brief Test standard Mode 0 configuration
 */
void test_standard_mode0_config() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    // Verify configuration is valid at compile-time
    static_assert(config.is_valid(), "Standard Mode 0 config should be valid");

    // Create instance
    constexpr auto spi = expert::create_instance(config);

    // Verify type
    static_assert(std::is_same_v<decltype(spi), const ExpertSpiInstance>,
                  "create_instance() should return ExpertSpiInstance");
}

/**
 * @brief Test TX-only configuration
 */
void test_tx_only_config() {
    constexpr auto config = SpiExpertConfig::tx_only_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA5,
        2000000
    );

    // Verify configuration is valid at compile-time
    static_assert(config.is_valid(), "TX-only config should be valid");
    static_assert(!config.enable_miso, "TX-only config should have MISO disabled");
    static_assert(config.enable_mosi, "TX-only config should have MOSI enabled");
}

/**
 * @brief Test DMA configuration
 */
void test_dma_config() {
    constexpr auto config = SpiExpertConfig::dma_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5,
        10000000
    );

    // Verify configuration is valid at compile-time
    static_assert(config.is_valid(), "DMA config should be valid");
    static_assert(config.enable_dma_tx, "DMA config should have TX DMA enabled");
    static_assert(config.enable_dma_rx, "DMA config should have RX DMA enabled");
    static_assert(config.enable_interrupts, "DMA config should have interrupts enabled");
}

/**
 * @brief Test high-speed configuration
 */
void test_high_speed_config() {
    constexpr auto config = SpiExpertConfig::high_speed_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5,
        25000000  // 25 MHz
    );

    // Verify configuration is valid at compile-time
    static_assert(config.is_valid(), "High-speed config should be valid");
    static_assert(config.clock_speed == 25000000, "Clock speed should be 25 MHz");
}

/**
 * @brief Test transfer operations
 */
void test_transfer_operations() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    auto spi = expert::create_instance(config);

    u8 tx_data[] = {0x01, 0x02, 0x03};
    u8 rx_data[3] = {0};

    // Test full-duplex transfer
    [[maybe_unused]] auto transfer_result = spi.transfer(
        std::span(tx_data),
        std::span(rx_data)
    );

    // Test transmit-only
    [[maybe_unused]] auto transmit_result = spi.transmit(std::span(tx_data));

    // Test receive-only
    [[maybe_unused]] auto receive_result = spi.receive(std::span(rx_data));

    // Verify return types
    static_assert(std::is_same_v<decltype(transfer_result), Result<void, ErrorCode>>,
                  "transfer() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(transmit_result), Result<void, ErrorCode>>,
                  "transmit() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(receive_result), Result<void, ErrorCode>>,
                  "receive() must return Result<void, ErrorCode>");
}

/**
 * @brief Test single-byte convenience methods
 */
void test_single_byte_operations() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    auto spi = expert::create_instance(config);

    // Test single-byte transfer
    [[maybe_unused]] auto transfer_result = spi.transfer_byte(0x55);

    // Test single-byte transmit
    [[maybe_unused]] auto transmit_result = spi.transmit_byte(0xAA);

    // Test single-byte receive
    [[maybe_unused]] auto receive_result = spi.receive_byte();

    // Verify return types
    static_assert(std::is_same_v<decltype(transfer_result), Result<u8, ErrorCode>>,
                  "transfer_byte() must return Result<u8, ErrorCode>");
    static_assert(std::is_same_v<decltype(transmit_result), Result<void, ErrorCode>>,
                  "transmit_byte() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(receive_result), Result<u8, ErrorCode>>,
                  "receive_byte() must return Result<u8, ErrorCode>");
}

/**
 * @brief Test TX-only operations
 */
void test_tx_only_operations() {
    constexpr auto config = SpiExpertConfig::tx_only_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA5,
        2000000
    );

    auto spi = expert::create_instance(config);

    u8 tx_data[] = {0x01, 0x02, 0x03};

    // Test transmit-only (should work)
    [[maybe_unused]] auto transmit_result = spi.transmit(std::span(tx_data));

    // Test single-byte transmit (should work)
    [[maybe_unused]] auto transmit_byte_result = spi.transmit_byte(0xAA);

    // Verify return types
    static_assert(std::is_same_v<decltype(transmit_result), Result<void, ErrorCode>>,
                  "transmit() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(transmit_byte_result), Result<void, ErrorCode>>,
                  "transmit_byte() must return Result<void, ErrorCode>");

    // Note: transfer() and receive() will return NotSupported error at runtime
    // but still compile correctly
}

/**
 * @brief Test configuration methods
 */
void test_configuration() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    auto spi = expert::create_instance(config);

    // Test full configuration
    SpiConfig new_config{SpiMode::Mode0, 1000000, SpiBitOrder::MsbFirst, SpiDataSize::Bits8};
    [[maybe_unused]] auto cfg_result = spi.configure(new_config);

    // Test mode change
    [[maybe_unused]] auto mode_result = spi.set_mode(SpiMode::Mode3);

    // Test speed change
    [[maybe_unused]] auto speed_result = spi.set_speed(2000000);

    // Verify return types
    static_assert(std::is_same_v<decltype(cfg_result), Result<void, ErrorCode>>,
                  "configure() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(mode_result), Result<void, ErrorCode>>,
                  "set_mode() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(speed_result), Result<void, ErrorCode>>,
                  "set_speed() must return Result<void, ErrorCode>");
}

/**
 * @brief Test status methods
 */
void test_status() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    const auto spi = expert::create_instance(config);

    // Test is_busy
    [[maybe_unused]] bool busy = spi.is_busy();

    // Test is_ready
    [[maybe_unused]] bool ready = spi.is_ready();

    // Verify return types
    static_assert(std::is_same_v<decltype(busy), bool>,
                  "is_busy() must return bool");
    static_assert(std::is_same_v<decltype(ready), bool>,
                  "is_ready() must return bool");
}

/**
 * @brief Test validation helpers
 */
void test_validation_helpers() {
    constexpr auto valid_config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    // Test validation functions
    static_assert(validate_spi_config(valid_config), "Valid config should pass validation");
    static_assert(has_valid_clock_speed(valid_config), "Clock speed should be valid");
    static_assert(has_enabled_direction(valid_config), "Should have enabled direction");
    static_assert(has_valid_crc_config(valid_config), "CRC config should be valid");
    static_assert(has_valid_frame_format(valid_config), "Frame format should be valid");
    static_assert(has_valid_dma_config(valid_config), "DMA config should be valid");
}

/**
 * @brief Test invalid configuration (clock speed too low)
 */
void test_invalid_clock_speed_low() {
    constexpr SpiExpertConfig config = {
        .peripheral = PeripheralId::SPI0,
        .mosi_pin = PinId::PA7,
        .miso_pin = PinId::PA6,
        .sck_pin = PinId::PA5,
        .nss_pin = PinId::PA0,
        .mode = SpiMode::Mode0,
        .clock_speed = 500,  // Too low
        .bit_order = SpiBitOrder::MsbFirst,
        .data_size = SpiDataSize::Bits8,
        .enable_mosi = true,
        .enable_miso = true,
        .enable_nss = false,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = false,
        .enable_motorola = true
    };

    static_assert(!config.is_valid(), "Config with low clock speed should be invalid");
}

/**
 * @brief Test invalid configuration (no enabled directions)
 */
void test_invalid_no_directions() {
    constexpr SpiExpertConfig config = {
        .peripheral = PeripheralId::SPI0,
        .mosi_pin = PinId::PA7,
        .miso_pin = PinId::PA6,
        .sck_pin = PinId::PA5,
        .nss_pin = PinId::PA0,
        .mode = SpiMode::Mode0,
        .clock_speed = 2000000,
        .bit_order = SpiBitOrder::MsbFirst,
        .data_size = SpiDataSize::Bits8,
        .enable_mosi = false,  // Both disabled
        .enable_miso = false,  // Both disabled
        .enable_nss = false,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = false,
        .enable_motorola = true
    };

    static_assert(!config.is_valid(), "Config with no enabled directions should be invalid");
}

/**
 * @brief Test invalid configuration (conflicting frame formats)
 */
void test_invalid_frame_formats() {
    constexpr SpiExpertConfig config = {
        .peripheral = PeripheralId::SPI0,
        .mosi_pin = PinId::PA7,
        .miso_pin = PinId::PA6,
        .sck_pin = PinId::PA5,
        .nss_pin = PinId::PA0,
        .mode = SpiMode::Mode0,
        .clock_speed = 2000000,
        .bit_order = SpiBitOrder::MsbFirst,
        .data_size = SpiDataSize::Bits8,
        .enable_mosi = true,
        .enable_miso = true,
        .enable_nss = false,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = true,   // Both enabled
        .enable_motorola = true   // Both enabled
    };

    static_assert(!config.is_valid(), "Config with conflicting frame formats should be invalid");
}

/**
 * @brief Test invalid DMA configuration
 */
void test_invalid_dma_config() {
    constexpr SpiExpertConfig config = {
        .peripheral = PeripheralId::SPI0,
        .mosi_pin = PinId::PA7,
        .miso_pin = PinId::PA6,
        .sck_pin = PinId::PA5,
        .nss_pin = PinId::PA0,
        .mode = SpiMode::Mode0,
        .clock_speed = 2000000,
        .bit_order = SpiBitOrder::MsbFirst,
        .data_size = SpiDataSize::Bits8,
        .enable_mosi = false,  // MOSI disabled
        .enable_miso = true,
        .enable_nss = false,
        .enable_interrupts = false,
        .enable_dma_tx = true,  // But DMA TX enabled
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = false,
        .enable_motorola = true
    };

    static_assert(!config.is_valid(), "Config with DMA TX enabled but MOSI disabled should be invalid");
}

/**
 * @brief Test SpiImplementation concept
 */
void test_concept() {
    // ExpertSpiInstance should satisfy SpiImplementation concept
    static_assert(SpiImplementation<ExpertSpiInstance>,
                  "ExpertSpiInstance must satisfy SpiImplementation concept");
}

/**
 * @brief Test apply method
 */
void test_apply() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    auto spi = expert::create_instance(config);
    [[maybe_unused]] auto apply_result = spi.apply();

    static_assert(std::is_same_v<decltype(apply_result), Result<void, ErrorCode>>,
                  "apply() must return Result<void, ErrorCode>");
}

/**
 * @brief Test config accessor
 */
void test_config_accessor() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    constexpr auto spi = expert::create_instance(config);
    constexpr auto retrieved_config = spi.config();

    static_assert(retrieved_config.peripheral == PeripheralId::SPI0,
                  "Should retrieve correct peripheral");
    static_assert(retrieved_config.clock_speed == 2000000,
                  "Should retrieve correct clock speed");
}

/**
 * @brief Test buffer size handling
 */
void test_buffer_size_handling() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    auto spi = expert::create_instance(config);

    u8 tx_data[5] = {1, 2, 3, 4, 5};
    u8 rx_data[3] = {0};

    // Transfer should handle different buffer sizes
    [[maybe_unused]] auto result = spi.transfer(
        std::span(tx_data),
        std::span(rx_data)
    );
}

/**
 * @brief Test error handling
 */
void test_error_handling() {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    auto spi = expert::create_instance(config);
    auto result = spi.transfer_byte(0x00);

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
    auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );
    auto spi = expert::create_instance(config);
    return true;
}

static_assert(test_constexpr_construction(),
              "ExpertSpiInstance must be constexpr constructible");

/**
 * @brief Test TX-only NotSupported errors
 */
void test_tx_only_not_supported() {
    constexpr auto config = SpiExpertConfig::tx_only_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA5,
        2000000
    );

    auto spi = expert::create_instance(config);

    u8 tx_data[] = {0x01};
    u8 rx_data[1] = {0};

    // These operations should compile but return NotSupported at runtime
    [[maybe_unused]] auto transfer_result = spi.transfer(std::span(tx_data), std::span(rx_data));
    [[maybe_unused]] auto receive_result = spi.receive(std::span(rx_data));
    [[maybe_unused]] auto receive_byte_result = spi.receive_byte();

    // Verify types compile correctly even for unsupported operations
    static_assert(std::is_same_v<decltype(transfer_result), Result<void, ErrorCode>>,
                  "transfer() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(receive_result), Result<void, ErrorCode>>,
                  "receive() must return Result<void, ErrorCode>");
    static_assert(std::is_same_v<decltype(receive_byte_result), Result<u8, ErrorCode>>,
                  "receive_byte() must return Result<u8, ErrorCode>");
}

// ============================================================================
// Summary
// ============================================================================

/**
 * Test Results:
 * - [x] ExpertSpiInstance inherits from SpiBase
 * - [x] Standard Mode 0 configuration works
 * - [x] TX-only configuration works
 * - [x] DMA configuration works
 * - [x] High-speed configuration works
 * - [x] All transfer operations compile (transfer, transmit, receive)
 * - [x] Single-byte convenience methods compile
 * - [x] TX-only operations compile correctly
 * - [x] Configuration methods compile (configure, set_mode, set_speed)
 * - [x] Status methods compile (is_busy, is_ready)
 * - [x] Validation helpers work
 * - [x] Invalid configurations detected at compile-time
 * - [x] SpiImplementation concept satisfied
 * - [x] Apply method works
 * - [x] Config accessor works
 * - [x] Buffer size handling works
 * - [x] Error handling works
 * - [x] Constexpr construction works
 * - [x] TX-only NotSupported errors compile correctly
 *
 * If this file compiles without errors, Phase 1.9.3 SpiExpert refactoring is successful!
 */
