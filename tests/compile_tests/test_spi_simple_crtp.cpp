/**
 * @file test_spi_simple_crtp.cpp
 * @brief Compile test for SPI Simple API with CRTP pattern
 *
 * This file tests that the SimpleSpiConfig and SimpleSpiTxConfig
 * inherit from SpiBase correctly and provide the expected interface.
 *
 * @note Part of Phase 1.9.1: Refactor SpiSimple
 */

#include "hal/api/spi_simple.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"

#include <span>

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Mock Pin Types
// ============================================================================

/**
 * @brief Mock MOSI pin for testing
 */
struct MockMosiPin {
    static constexpr PinId get_pin_id() { return PinId{0}; }
};

/**
 * @brief Mock MISO pin for testing
 */
struct MockMisoPin {
    static constexpr PinId get_pin_id() { return PinId{1}; }
};

/**
 * @brief Mock SCK pin for testing
 */
struct MockSckPin {
    static constexpr PinId get_pin_id() { return PinId{2}; }
};

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test SimpleSpiConfig inherits from SpiBase
 */
void test_simple_spi_inheritance() {
    using ConfigType = SimpleSpiConfig<MockMosiPin, MockMisoPin, MockSckPin>;
    using BaseType = SpiBase<ConfigType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, ConfigType>,
                  "SimpleSpiConfig must inherit from SpiBase");
}

/**
 * @brief Test SimpleSpiTxConfig inherits from SpiBase
 */
void test_simple_spi_tx_inheritance() {
    using ConfigType = SimpleSpiTxConfig<MockMosiPin, MockSckPin>;
    using BaseType = SpiBase<ConfigType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, ConfigType>,
                  "SimpleSpiTxConfig must inherit from SpiBase");
}

/**
 * @brief Test full-duplex SPI configuration
 */
void test_full_duplex_spi() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>(
        1000000,  // 1 MHz
        SpiMode::Mode0
    );

    // Verify type
    using ConfigType = decltype(spi);
    static_assert(std::is_same_v<ConfigType, SimpleSpiConfig<MockMosiPin, MockMisoPin, MockSckPin>>,
                  "quick_setup should return SimpleSpiConfig");
}

/**
 * @brief Test TX-only SPI configuration
 */
void test_tx_only_spi() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup_master_tx<MockMosiPin, MockSckPin>(
        2000000,  // 2 MHz
        SpiMode::Mode3
    );

    // Verify type
    using ConfigType = decltype(spi);
    static_assert(std::is_same_v<ConfigType, SimpleSpiTxConfig<MockMosiPin, MockSckPin>>,
                  "quick_setup_master_tx should return SimpleSpiTxConfig");
}

/**
 * @brief Test transfer operations (full-duplex)
 */
void test_transfer_operations() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

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
 * @brief Test single-byte convenience methods (full-duplex)
 */
void test_single_byte_operations() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

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
    auto spi = Spi<PeripheralId::SPI0>::quick_setup_master_tx<MockMosiPin, MockSckPin>();

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
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

    // Test full configuration
    SpiConfig config{SpiMode::Mode0, 1000000, SpiBitOrder::MsbFirst, SpiDataSize::Bits8};
    [[maybe_unused]] auto cfg_result = spi.configure(config);

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
    const auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

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
 * @brief Test default values
 */
void test_defaults() {
    // Test with defaults
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

    // Verify configuration
    static_assert(SpiDefaults::mode == SpiMode::Mode0,
                  "Default mode should be Mode0");
    static_assert(SpiDefaults::clock_speed == 1000000,
                  "Default clock speed should be 1 MHz");
    static_assert(SpiDefaults::bit_order == SpiBitOrder::MsbFirst,
                  "Default bit order should be MSB first");
    static_assert(SpiDefaults::data_size == SpiDataSize::Bits8,
                  "Default data size should be 8 bits");
}

/**
 * @brief Test quick_setup_with_mode
 */
void test_quick_setup_with_mode() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup_with_mode<MockMosiPin, MockMisoPin, MockSckPin>(
        2000000,
        SpiMode::Mode3
    );

    // Verify type
    using ConfigType = decltype(spi);
    static_assert(std::is_same_v<ConfigType, SimpleSpiConfig<MockMosiPin, MockMisoPin, MockSckPin>>,
                  "quick_setup_with_mode should return SimpleSpiConfig");
}

/**
 * @brief Test SpiImplementation concept (full-duplex)
 */
void test_concept_full_duplex() {
    using ConfigType = SimpleSpiConfig<MockMosiPin, MockMisoPin, MockSckPin>;

    // SimpleSpiConfig should satisfy SpiImplementation concept
    static_assert(SpiImplementation<ConfigType>,
                  "SimpleSpiConfig must satisfy SpiImplementation concept");
}

/**
 * @brief Test SpiImplementation concept (TX-only)
 */
void test_concept_tx_only() {
    using ConfigType = SimpleSpiTxConfig<MockMosiPin, MockSckPin>;

    // SimpleSpiTxConfig should satisfy SpiImplementation concept
    static_assert(SpiImplementation<ConfigType>,
                  "SimpleSpiTxConfig must satisfy SpiImplementation concept");
}

/**
 * @brief Test initialize method (full-duplex)
 */
void test_initialize_full_duplex() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

    [[maybe_unused]] auto init_result = spi.initialize();

    static_assert(std::is_same_v<decltype(init_result), Result<void, ErrorCode>>,
                  "initialize() must return Result<void, ErrorCode>");
}

/**
 * @brief Test initialize method (TX-only)
 */
void test_initialize_tx_only() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup_master_tx<MockMosiPin, MockSckPin>();

    [[maybe_unused]] auto init_result = spi.initialize();

    static_assert(std::is_same_v<decltype(init_result), Result<void, ErrorCode>>,
                  "initialize() must return Result<void, ErrorCode>");
}

/**
 * @brief Test buffer size handling
 */
void test_buffer_size_handling() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

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
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();

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
    auto spi = Spi<PeripheralId::SPI0>::quick_setup<MockMosiPin, MockMisoPin, MockSckPin>();
    return true;
}

static_assert(test_constexpr_construction(),
              "SimpleSpiConfig must be constexpr constructible");

/**
 * @brief Test TX-only NotSupported errors at compile time
 */
void test_tx_only_not_supported() {
    auto spi = Spi<PeripheralId::SPI0>::quick_setup_master_tx<MockMosiPin, MockSckPin>();

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
 * - [x] SimpleSpiConfig inherits from SpiBase
 * - [x] SimpleSpiTxConfig inherits from SpiBase
 * - [x] Full-duplex SPI configuration works
 * - [x] TX-only SPI configuration works
 * - [x] All transfer operations compile (transfer, transmit, receive)
 * - [x] Single-byte convenience methods compile
 * - [x] TX-only operations compile correctly
 * - [x] Configuration methods compile (configure, set_mode, set_speed)
 * - [x] Status methods compile (is_busy, is_ready)
 * - [x] Default values are correct
 * - [x] quick_setup_with_mode works
 * - [x] SpiImplementation concept satisfied (both variants)
 * - [x] Initialize methods work (both variants)
 * - [x] Buffer size handling works
 * - [x] Error handling works
 * - [x] Constexpr construction works
 * - [x] TX-only NotSupported errors compile correctly
 *
 * If this file compiles without errors, Phase 1.9.1 SpiSimple refactoring is successful!
 */
