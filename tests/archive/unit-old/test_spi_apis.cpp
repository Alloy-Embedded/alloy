/**
 * @file test_spi_apis.cpp
 * @brief Tests for SPI Multi-Level APIs
 *
 * Tests all 3 levels of SPI configuration APIs and DMA integration.
 */

#include <cassert>
#include <iostream>

#include "../../src/hal/spi_simple.hpp"
#include "../../src/hal/spi_fluent.hpp"
#include "../../src/hal/spi_expert.hpp"
#include "../../src/hal/spi_dma.hpp"
#include "../../src/hal/dma_connection.hpp"

using namespace alloy::hal;
using namespace alloy::core;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                \
    void test_##name();                                           \
    void run_test_##name() {                                      \
        tests_run++;                                              \
        try {                                                     \
            test_##name();                                        \
            tests_passed++;                                       \
            std::cout << "  ✓ " << #name << std::endl;            \
        } catch (const std::exception& e) {                       \
            std::cout << "  ✗ " << #name << ": " << e.what()      \
                      << std::endl;                               \
        }                                                         \
    }                                                             \
    void test_##name()

#define ASSERT(condition)                                         \
    do {                                                          \
        if (!(condition)) {                                       \
            throw std::runtime_error("Assertion failed: " #condition); \
        }                                                         \
    } while (0)

#define TEST_SUITE(name)                                          \
    namespace test_suite_##name {                                 \
        void run_all_tests();                                     \
    }                                                             \
    namespace test_suite_##name

#define RUN_TEST_SUITE(name)                                      \
    do {                                                          \
        std::cout << "\n" << #name << ":" << std::endl;           \
        test_suite_##name::run_all_tests();                       \
    } while (0)

#define PRINT_TEST_SUMMARY()                                      \
    std::cout << "\n========================================\n";   \
    std::cout << "Tests run: " << tests_run << std::endl;         \
    std::cout << "Tests passed: " << tests_passed << std::endl;   \
    std::cout << "Tests failed: " << (tests_run - tests_passed)  \
              << std::endl;                                       \
    std::cout << "========================================\n"

#define TEST_RESULT() (tests_run == tests_passed ? 0 : 1)

// ============================================================================
// Mock GPIO Pins for Testing
// ============================================================================

// Mock GPIO pin for testing
template <u32 Port, u32 Pin>
struct MockGpioPin {
    static constexpr PinId get_pin_id() {
        return static_cast<PinId>(Port * 32 + Pin);
    }
};

// SPI0 pins
using Spi0_MOSI = MockGpioPin<0, 7>;   // PA7
using Spi0_MISO = MockGpioPin<0, 6>;   // PA6
using Spi0_SCK = MockGpioPin<0, 5>;    // PA5

// SPI1 pins
using Spi1_MOSI = MockGpioPin<1, 15>;  // PB15
using Spi1_MISO = MockGpioPin<1, 14>;  // PB14
using Spi1_SCK = MockGpioPin<1, 13>;   // PB13

// ============================================================================
// Test Suite: SPI Simple API
// ============================================================================

TEST_SUITE(SpiSimpleApi) {

// Test basic quick_setup
TEST(quick_setup_creates_config) {
    auto config = Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI, Spi0_MISO, Spi0_SCK>();

    ASSERT(config.peripheral == PeripheralId::SPI0);
    ASSERT(config.config.mode == SpiMode::Mode0);
    ASSERT(config.config.clock_speed == 1000000);  // Default 1 MHz
}

// Test quick_setup with custom speed
TEST(quick_setup_with_custom_speed) {
    auto config = Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI, Spi0_MISO, Spi0_SCK>(
        2000000  // 2 MHz
    );

    ASSERT(config.config.clock_speed == 2000000);
}

// Test quick_setup with custom mode
TEST(quick_setup_with_custom_mode) {
    auto config = Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI, Spi0_MISO, Spi0_SCK>(
        1000000,
        SpiMode::Mode3
    );

    ASSERT(config.config.mode == SpiMode::Mode3);
}

// Test TX-only setup
TEST(quick_setup_master_tx) {
    auto config = Spi<PeripheralId::SPI0>::quick_setup_master_tx<Spi0_MOSI, Spi0_SCK>();

    ASSERT(config.peripheral == PeripheralId::SPI0);
    ASSERT(config.config.mode == SpiMode::Mode0);
}

// Test TX-only with custom speed
TEST(quick_setup_master_tx_custom_speed) {
    auto config = Spi<PeripheralId::SPI0>::quick_setup_master_tx<Spi0_MOSI, Spi0_SCK>(
        4000000  // 4 MHz
    );

    ASSERT(config.config.clock_speed == 4000000);
}

void run_all_tests() {
    run_test_quick_setup_creates_config();
    run_test_quick_setup_with_custom_speed();
    run_test_quick_setup_with_custom_mode();
    run_test_quick_setup_master_tx();
    run_test_quick_setup_master_tx_custom_speed();
}

}  // TEST_SUITE(SpiSimpleApi)

// ============================================================================
// Test Suite: SPI Fluent API
// ============================================================================

TEST_SUITE(SpiFluentApi) {

// Test basic builder
TEST(builder_basic_setup) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_mosi<Spi0_MOSI>()
        .with_miso<Spi0_MISO>()
        .with_sck<Spi0_SCK>()
        .clock_speed(2000000)
        .initialize();

    ASSERT(result.is_ok());
    auto config = result.unwrap();
    ASSERT(config.peripheral == PeripheralId::SPI0);
    ASSERT(config.config.clock_speed == 2000000);
}

// Test builder with mode
TEST(builder_with_mode) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_pins<Spi0_MOSI, Spi0_MISO, Spi0_SCK>()
        .clock_speed(1000000)
        .mode(SpiMode::Mode2)
        .initialize();

    ASSERT(result.is_ok());
    auto config = result.unwrap();
    ASSERT(config.config.mode == SpiMode::Mode2);
}

// Test builder with bit order
TEST(builder_with_lsb_first) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_pins<Spi0_MOSI, Spi0_MISO, Spi0_SCK>()
        .clock_speed(1000000)
        .lsb_first()
        .initialize();

    ASSERT(result.is_ok());
    auto config = result.unwrap();
    ASSERT(config.config.bit_order == SpiBitOrder::LsbFirst);
}

// Test builder with 16-bit data
TEST(builder_with_16bit_data) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_pins<Spi0_MOSI, Spi0_MISO, Spi0_SCK>()
        .clock_speed(1000000)
        .data_16bit()
        .initialize();

    ASSERT(result.is_ok());
    auto config = result.unwrap();
    ASSERT(config.config.data_size == SpiDataSize::Bits16);
}

// Test builder preset Mode0
TEST(builder_standard_mode0) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_pins<Spi0_MOSI, Spi0_MISO, Spi0_SCK>()
        .clock_speed(1000000)
        .standard_mode0()
        .initialize();

    ASSERT(result.is_ok());
    auto config = result.unwrap();
    ASSERT(config.config.mode == SpiMode::Mode0);
    ASSERT(config.config.bit_order == SpiBitOrder::MsbFirst);
}

// Test builder preset Mode3
TEST(builder_standard_mode3) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_pins<Spi0_MOSI, Spi0_MISO, Spi0_SCK>()
        .clock_speed(1000000)
        .standard_mode3()
        .initialize();

    ASSERT(result.is_ok());
    auto config = result.unwrap();
    ASSERT(config.config.mode == SpiMode::Mode3);
}

// Test TX-only builder
TEST(builder_tx_only) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_mosi<Spi0_MOSI>()
        .with_sck<Spi0_SCK>()
        .clock_speed(1000000)
        .initialize();

    ASSERT(result.is_ok());
    auto config = result.unwrap();
    ASSERT(config.tx_only == true);
}

// Test invalid builder (missing clock speed)
TEST(builder_validation_missing_clock) {
    auto result = SpiBuilder<PeripheralId::SPI0>()
        .with_pins<Spi0_MOSI, Spi0_MISO, Spi0_SCK>()
        .initialize();

    ASSERT(!result.is_ok());
}

void run_all_tests() {
    run_test_builder_basic_setup();
    run_test_builder_with_mode();
    run_test_builder_with_lsb_first();
    run_test_builder_with_16bit_data();
    run_test_builder_standard_mode0();
    run_test_builder_standard_mode3();
    run_test_builder_tx_only();
    run_test_builder_validation_missing_clock();
}

}  // TEST_SUITE(SpiFluentApi)

// ============================================================================
// Test Suite: SPI Expert API
// ============================================================================

TEST_SUITE(SpiExpertApi) {

// Test basic expert config
TEST(expert_config_basic) {
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
        .enable_ti_mode = false,
        .enable_motorola = true
    };

    ASSERT(config.is_valid());
}

// Test preset standard_mode0
TEST(expert_preset_standard_mode0) {
    constexpr auto config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5
    );

    ASSERT(config.is_valid());
    ASSERT(config.mode == SpiMode::Mode0);
    ASSERT(config.clock_speed == 2000000);
}

// Test preset TX-only
TEST(expert_preset_tx_only) {
    constexpr auto config = SpiExpertConfig::tx_only_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA5,
        4000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.enable_mosi == true);
    ASSERT(config.enable_miso == false);
    ASSERT(config.clock_speed == 4000000);
}

// Test preset DMA config
TEST(expert_preset_dma) {
    constexpr auto config = SpiExpertConfig::dma_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5,
        2000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.enable_dma_tx);
    ASSERT(config.enable_dma_rx);
    ASSERT(config.enable_interrupts);
}

// Test preset high-speed
TEST(expert_preset_high_speed) {
    constexpr auto config = SpiExpertConfig::high_speed_config(
        PeripheralId::SPI0,
        PinId::PA7,
        PinId::PA6,
        PinId::PA5,
        10000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.clock_speed == 10000000);
}

// Test validation: no MOSI or MISO
TEST(expert_validation_no_pins) {
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
        .enable_mosi = false,  // Both disabled!
        .enable_miso = false,
        .enable_nss = false,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = false,
        .enable_motorola = true
    };

    ASSERT(!config.is_valid());
}

// Test validation: clock speed too low
TEST(expert_validation_clock_too_low) {
    constexpr SpiExpertConfig config = {
        .peripheral = PeripheralId::SPI0,
        .mosi_pin = PinId::PA7,
        .miso_pin = PinId::PA6,
        .sck_pin = PinId::PA5,
        .nss_pin = PinId::PA0,
        .mode = SpiMode::Mode0,
        .clock_speed = 500,  // Too low!
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

    ASSERT(!config.is_valid());
}

// Test validation: DMA TX without MOSI
TEST(expert_validation_dma_tx_no_mosi) {
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
        .enable_dma_tx = true,  // But DMA TX enabled!
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = false,
        .enable_motorola = true
    };

    ASSERT(!config.is_valid());
}

void run_all_tests() {
    run_test_expert_config_basic();
    run_test_expert_preset_standard_mode0();
    run_test_expert_preset_tx_only();
    run_test_expert_preset_dma();
    run_test_expert_preset_high_speed();
    run_test_expert_validation_no_pins();
    run_test_expert_validation_clock_too_low();
    run_test_expert_validation_dma_tx_no_mosi();
}

}  // TEST_SUITE(SpiExpertApi)

// ============================================================================
// Test Suite: SPI DMA Integration
// ============================================================================

TEST_SUITE(SpiDmaIntegration) {

// Test SPI DMA config creation
TEST(create_spi_dma_config) {
    using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;
    using Spi0RxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_RX, DmaStream::Stream2>;

    constexpr auto config = SpiDmaConfig<Spi0TxDma, Spi0RxDma>::create(
        PinId::PA7,
        PinId::PA6,
        PinId::PA5,
        2000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.spi_config.enable_dma_tx);
    ASSERT(config.spi_config.enable_dma_rx);
    ASSERT(config.spi_config.enable_interrupts);
}

// Test TX-only SPI DMA
TEST(tx_only_spi_dma) {
    using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;

    constexpr auto config = SpiDmaConfig<Spi0TxDma, void>::create(
        PinId::PA7,
        PinId::PA0,  // Unused MISO
        PinId::PA5,
        2000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.spi_config.enable_dma_tx);
    ASSERT(!config.spi_config.enable_dma_rx);
    constexpr bool has_tx = SpiDmaConfig<Spi0TxDma, void>::has_tx_dma();
    constexpr bool has_rx = SpiDmaConfig<Spi0TxDma, void>::has_rx_dma();
    ASSERT(has_tx);
    ASSERT(!has_rx);
}

// Test full-duplex preset
TEST(full_duplex_dma_preset) {
    using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;
    using Spi0RxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_RX, DmaStream::Stream2>;

    constexpr auto config = create_spi_full_duplex_dma<Spi0TxDma, Spi0RxDma>(
        PinId::PA7,
        PinId::PA6,
        PinId::PA5,
        2000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.spi_config.enable_dma_tx);
    ASSERT(config.spi_config.enable_dma_rx);
}

// Test TX-only preset
TEST(tx_only_dma_preset) {
    using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;

    constexpr auto config = create_spi_tx_only_dma<Spi0TxDma>(
        PinId::PA7,
        PinId::PA5,
        4000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.spi_config.enable_dma_tx);
    ASSERT(!config.spi_config.enable_dma_rx);
}

// Test high-speed preset
TEST(high_speed_dma_preset) {
    using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;
    using Spi0RxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_RX, DmaStream::Stream2>;

    constexpr auto config = create_spi_high_speed_dma<Spi0TxDma, Spi0RxDma>(
        PinId::PA7,
        PinId::PA6,
        PinId::PA5,
        10000000
    );

    ASSERT(config.is_valid());
    ASSERT(config.spi_config.clock_speed == 10000000);
}

void run_all_tests() {
    run_test_create_spi_dma_config();
    run_test_tx_only_spi_dma();
    run_test_full_duplex_dma_preset();
    run_test_tx_only_dma_preset();
    run_test_high_speed_dma_preset();
}

}  // TEST_SUITE(SpiDmaIntegration)

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    RUN_TEST_SUITE(SpiSimpleApi);
    RUN_TEST_SUITE(SpiFluentApi);
    RUN_TEST_SUITE(SpiExpertApi);
    RUN_TEST_SUITE(SpiDmaIntegration);

    PRINT_TEST_SUMMARY();
    return TEST_RESULT();
}
