/**
 * @file test_dma_infrastructure.cpp
 * @brief Tests for DMA Type-Safe Infrastructure
 *
 * Tests compile-time DMA connection validation, registry management,
 * and UART DMA integration.
 */

#include <cassert>
#include <iostream>

#include "../../src/hal/dma_connection.hpp"
#include "../../src/hal/dma_registry.hpp"
#include "../../src/hal/dma_config.hpp"
#include "../../src/hal/uart_dma.hpp"

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
// Test Suite: DMA Connection Types
// ============================================================================

TEST_SUITE(DmaConnectionTypes) {

// Test valid USART0 TX connection
TEST(usart0_tx_connection_is_valid) {
    using Uart0TxDma = DmaConnection<
        PeripheralId::USART0,
        DmaRequest::USART0_TX,
        DmaStream::Stream0
    >;

    ASSERT(Uart0TxDma::is_compatible());
    ASSERT(Uart0TxDma::peripheral == PeripheralId::USART0);
    ASSERT(Uart0TxDma::request == DmaRequest::USART0_TX);
    ASSERT(Uart0TxDma::stream == DmaStream::Stream0);
}

// Test valid USART0 RX connection
TEST(usart0_rx_connection_is_valid) {
    using Uart0RxDma = DmaConnection<
        PeripheralId::USART0,
        DmaRequest::USART0_RX,
        DmaStream::Stream1
    >;

    ASSERT(Uart0RxDma::is_compatible());
    ASSERT(Uart0RxDma::peripheral == PeripheralId::USART0);
    ASSERT(Uart0RxDma::request == DmaRequest::USART0_RX);
    ASSERT(Uart0RxDma::stream == DmaStream::Stream1);
}

// Test invalid connection (wrong peripheral-request pair)
TEST(invalid_peripheral_request_pair) {
    using InvalidDma = DmaConnection<
        PeripheralId::USART0,
        DmaRequest::SPI0_TX,  // Wrong! SPI request for UART peripheral
        DmaStream::Stream0
    >;

    ASSERT(!InvalidDma::is_compatible());
}

// Test SPI connections
TEST(spi0_tx_connection_is_valid) {
    using Spi0TxDma = DmaConnection<
        PeripheralId::SPI0,
        DmaRequest::SPI0_TX,
        DmaStream::Stream2
    >;

    ASSERT(Spi0TxDma::is_compatible());
}

// Test I2C connections
TEST(i2c1_tx_connection_is_valid) {
    using I2c1TxDma = DmaConnection<
        PeripheralId::I2C1,
        DmaRequest::I2C1_TX,
        DmaStream::Stream3
    >;

    ASSERT(I2c1TxDma::is_compatible());
}

// Test conflict detection
TEST(same_stream_conflicts) {
    using Dma1 = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Dma2 = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream0>;

    ASSERT(Dma1::conflicts_with<Dma2>());
    ASSERT(Dma2::conflicts_with<Dma1>());
}

// Test no conflict with different streams
TEST(different_streams_no_conflict) {
    using Dma1 = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Dma2 = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream1>;

    ASSERT(!Dma1::conflicts_with<Dma2>());
    ASSERT(!Dma2::conflicts_with<Dma1>());
}

void run_all_tests() {
    run_test_usart0_tx_connection_is_valid();
    run_test_usart0_rx_connection_is_valid();
    run_test_invalid_peripheral_request_pair();
    run_test_spi0_tx_connection_is_valid();
    run_test_i2c1_tx_connection_is_valid();
    run_test_same_stream_conflicts();
    run_test_different_streams_no_conflict();
}

}  // TEST_SUITE(DmaConnectionTypes)

// ============================================================================
// Test Suite: DMA Registry
// ============================================================================

TEST_SUITE(DmaRegistry) {

// Test empty registry
TEST(empty_registry_has_no_allocations) {
    ASSERT(EmptyDmaRegistry::size == 0);
    ASSERT(!EmptyDmaRegistry::has_conflicts());
    ASSERT(EmptyDmaRegistry::is_valid());
}

// Test single allocation
TEST(single_allocation) {
    using Dma1 = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Reg1 = AddDmaAllocation_t<EmptyDmaRegistry, Dma1>;

    ASSERT(Reg1::size == 1);
    ASSERT(!Reg1::has_conflicts());
    ASSERT(Reg1::is_stream_allocated(DmaStream::Stream0));
    ASSERT(!Reg1::is_stream_allocated(DmaStream::Stream1));
}

// Test multiple allocations without conflicts
TEST(multiple_allocations_no_conflict) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;
    using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream2>;

    using Reg1 = AddDmaAllocation_t<EmptyDmaRegistry, Uart0TxDma>;
    using Reg2 = AddDmaAllocation_t<Reg1, Uart0RxDma>;
    using Reg3 = AddDmaAllocation_t<Reg2, Spi0TxDma>;

    ASSERT(Reg3::size == 3);
    ASSERT(!Reg3::has_conflicts());
    ASSERT(Reg3::is_stream_allocated(DmaStream::Stream0));
    ASSERT(Reg3::is_stream_allocated(DmaStream::Stream1));
    ASSERT(Reg3::is_stream_allocated(DmaStream::Stream2));
}

// Test conflict detection
TEST(detect_stream_conflict) {
    using Dma1 = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Dma2 = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream0>;  // Same stream!

    using Reg1 = AddDmaAllocation_t<EmptyDmaRegistry, Dma1>;
    using ConflictReg = AddDmaAllocation_t<Reg1, Dma2>;

    ASSERT(ConflictReg::has_conflicts());
    ASSERT(!ConflictReg::is_valid());
    ASSERT(ConflictReg::count_stream_allocations(DmaStream::Stream0) == 2);
}

// Test peripheral query
TEST(query_peripheral_dma) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Spi0TxDma = DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream1>;

    using Reg1 = AddDmaAllocation_t<EmptyDmaRegistry, Uart0TxDma>;
    using Reg2 = AddDmaAllocation_t<Reg1, Spi0TxDma>;

    ASSERT(Reg2::has_peripheral_dma(PeripheralId::USART0));
    ASSERT(Reg2::has_peripheral_dma(PeripheralId::SPI0));
    ASSERT(!Reg2::has_peripheral_dma(PeripheralId::I2C1));
}

// Test request query
TEST(query_request_allocation) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Reg1 = AddDmaAllocation_t<EmptyDmaRegistry, Uart0TxDma>;

    ASSERT(Reg1::is_request_allocated(DmaRequest::USART0_TX));
    ASSERT(!Reg1::is_request_allocated(DmaRequest::USART0_RX));
    ASSERT(!Reg1::is_request_allocated(DmaRequest::SPI0_TX));
}

// Test get stream for request
TEST(get_stream_for_request) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream3>;
    using Reg1 = AddDmaAllocation_t<EmptyDmaRegistry, Uart0TxDma>;

    ASSERT(Reg1::get_stream_for_request(DmaRequest::USART0_TX) == DmaStream::Stream3);
}

// Test count allocations
TEST(count_stream_allocations) {
    using Dma1 = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Dma2 = DmaConnection<PeripheralId::USART1, DmaRequest::USART1_TX, DmaStream::Stream1>;

    using Reg1 = AddDmaAllocation_t<EmptyDmaRegistry, Dma1>;
    using Reg2 = AddDmaAllocation_t<Reg1, Dma2>;

    ASSERT(Reg2::count_stream_allocations(DmaStream::Stream0) == 1);
    ASSERT(Reg2::count_stream_allocations(DmaStream::Stream1) == 1);
    ASSERT(Reg2::count_stream_allocations(DmaStream::Stream2) == 0);
}

void run_all_tests() {
    run_test_empty_registry_has_no_allocations();
    run_test_single_allocation();
    run_test_multiple_allocations_no_conflict();
    run_test_detect_stream_conflict();
    run_test_query_peripheral_dma();
    run_test_query_request_allocation();
    run_test_get_stream_for_request();
    run_test_count_stream_allocations();
}

}  // TEST_SUITE(DmaRegistry)

// ============================================================================
// Test Suite: DMA Configuration
// ============================================================================

TEST_SUITE(DmaConfiguration) {

// Test memory-to-peripheral configuration
TEST(memory_to_peripheral_config) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;

    u8 buffer[32];
    auto config = DmaTransferConfig<Uart0TxDma>::memory_to_peripheral(
        buffer,
        sizeof(buffer),
        DmaDataWidth::Bits8
    );

    ASSERT(config.config.direction == DmaDirection::MemoryToPeripheral);
    ASSERT(config.config.data_width == DmaDataWidth::Bits8);
    ASSERT(config.source_address == buffer);
    ASSERT(config.destination_address == nullptr);
    ASSERT(config.transfer_size == sizeof(buffer));
}

// Test peripheral-to-memory configuration
TEST(peripheral_to_memory_config) {
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;

    u8 buffer[32];
    auto config = DmaTransferConfig<Uart0RxDma>::peripheral_to_memory(
        buffer,
        sizeof(buffer),
        DmaDataWidth::Bits8
    );

    ASSERT(config.config.direction == DmaDirection::PeripheralToMemory);
    ASSERT(config.config.data_width == DmaDataWidth::Bits8);
    ASSERT(config.source_address == nullptr);
    ASSERT(config.destination_address == buffer);
    ASSERT(config.transfer_size == sizeof(buffer));
}

// Test memory-to-memory configuration
TEST(memory_to_memory_config) {
    using AnyDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;

    u8 src[32];
    u8 dst[32];
    auto config = DmaTransferConfig<AnyDma>::memory_to_memory(
        src,
        dst,
        sizeof(src),
        DmaDataWidth::Bits32
    );

    ASSERT(config.config.direction == DmaDirection::MemoryToMemory);
    ASSERT(config.config.data_width == DmaDataWidth::Bits32);
    ASSERT(config.source_address == src);
    ASSERT(config.destination_address == dst);
    ASSERT(config.transfer_size == sizeof(src));
}

// Test circular mode
TEST(circular_mode_config) {
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;

    u8 buffer[32];
    auto config = DmaTransferConfig<Uart0RxDma>::peripheral_to_memory(
        buffer,
        sizeof(buffer),
        DmaDataWidth::Bits8,
        DmaMode::Circular
    );

    ASSERT(config.config.mode == DmaMode::Circular);
}

// Test priority setting
TEST(priority_setting) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;

    u8 buffer[32];
    auto config = DmaTransferConfig<Uart0TxDma>::memory_to_peripheral(
        buffer,
        sizeof(buffer),
        DmaDataWidth::Bits8,
        DmaMode::Normal,
        DmaPriority::VeryHigh
    );

    ASSERT(config.config.priority == DmaPriority::VeryHigh);
}

// Test helper function for UART TX
TEST(uart_tx_dma_helper) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;

    u8 buffer[32];
    auto config = create_uart_tx_dma<Uart0TxDma>(buffer, sizeof(buffer));

    ASSERT(config.config.direction == DmaDirection::MemoryToPeripheral);
    ASSERT(config.config.data_width == DmaDataWidth::Bits8);
    ASSERT(config.config.mode == DmaMode::Normal);
    ASSERT(config.config.priority == DmaPriority::Medium);
}

// Test helper function for UART RX
TEST(uart_rx_dma_helper) {
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;

    u8 buffer[32];
    auto config = create_uart_rx_dma<Uart0RxDma>(buffer, sizeof(buffer), true);

    ASSERT(config.config.direction == DmaDirection::PeripheralToMemory);
    ASSERT(config.config.data_width == DmaDataWidth::Bits8);
    ASSERT(config.config.mode == DmaMode::Circular);
}

void run_all_tests() {
    run_test_memory_to_peripheral_config();
    run_test_peripheral_to_memory_config();
    run_test_memory_to_memory_config();
    run_test_circular_mode_config();
    run_test_priority_setting();
    run_test_uart_tx_dma_helper();
    run_test_uart_rx_dma_helper();
}

}  // TEST_SUITE(DmaConfiguration)

// ============================================================================
// Test Suite: UART DMA Integration
// ============================================================================

TEST_SUITE(UartDmaIntegration) {

// Test UART DMA config creation
TEST(create_uart_dma_config) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;

    constexpr auto config = UartDmaConfig<Uart0TxDma, Uart0RxDma>::create(
        PinId::PD3,
        PinId::PD4,
        BaudRate{115200}
    );

    ASSERT(config.is_valid());
    ASSERT(config.uart_config.enable_dma_tx);
    ASSERT(config.uart_config.enable_dma_rx);
    ASSERT(config.uart_config.enable_interrupts);
}

// Test TX-only UART DMA
TEST(tx_only_uart_dma) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;

    constexpr auto config = UartDmaConfig<Uart0TxDma, void>::create(
        PinId::PD3,
        PinId::PA0, // Unused
        BaudRate{115200}
    );

    ASSERT(config.is_valid());
    ASSERT(config.uart_config.enable_dma_tx);
    ASSERT(!config.uart_config.enable_dma_rx);
    constexpr bool has_tx = UartDmaConfig<Uart0TxDma, void>::has_tx_dma();
    constexpr bool has_rx = UartDmaConfig<Uart0TxDma, void>::has_rx_dma();
    ASSERT(has_tx);
    ASSERT(!has_rx);
}

// Test RX-only UART DMA
TEST(rx_only_uart_dma) {
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;

    constexpr auto config = UartDmaConfig<void, Uart0RxDma>::create(
        PinId::PA0, // Unused
        PinId::PD4,
        BaudRate{115200}
    );

    ASSERT(config.is_valid());
    ASSERT(!config.uart_config.enable_dma_tx);
    ASSERT(config.uart_config.enable_dma_rx);
    constexpr bool has_tx2 = UartDmaConfig<void, Uart0RxDma>::has_tx_dma();
    constexpr bool has_rx2 = UartDmaConfig<void, Uart0RxDma>::has_rx_dma();
    ASSERT(!has_tx2);
    ASSERT(has_rx2);
}

// Test full-duplex preset
TEST(full_duplex_dma_preset) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;

    constexpr auto config = create_uart_full_duplex_dma<Uart0TxDma, Uart0RxDma>(
        PinId::PD3,
        PinId::PD4,
        BaudRate{115200}
    );

    ASSERT(config.is_valid());
    ASSERT(config.uart_config.enable_dma_tx);
    ASSERT(config.uart_config.enable_dma_rx);
}

// Test TX-only preset
TEST(tx_only_dma_preset) {
    using Uart0TxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_TX, DmaStream::Stream0>;

    constexpr auto config = create_uart_tx_only_dma<Uart0TxDma>(
        PinId::PD3,
        BaudRate{115200}
    );

    ASSERT(config.is_valid());
    ASSERT(config.uart_config.enable_dma_tx);
    ASSERT(!config.uart_config.enable_dma_rx);
}

// Test RX-only preset
TEST(rx_only_dma_preset) {
    using Uart0RxDma = DmaConnection<PeripheralId::USART0, DmaRequest::USART0_RX, DmaStream::Stream1>;

    constexpr auto config = create_uart_rx_only_dma<Uart0RxDma>(
        PinId::PD4,
        BaudRate{115200}
    );

    ASSERT(config.is_valid());
    ASSERT(!config.uart_config.enable_dma_tx);
    ASSERT(config.uart_config.enable_dma_rx);
}

void run_all_tests() {
    run_test_create_uart_dma_config();
    run_test_tx_only_uart_dma();
    run_test_rx_only_uart_dma();
    run_test_full_duplex_dma_preset();
    run_test_tx_only_dma_preset();
    run_test_rx_only_dma_preset();
}

}  // TEST_SUITE(UartDmaIntegration)

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    RUN_TEST_SUITE(DmaConnectionTypes);
    RUN_TEST_SUITE(DmaRegistry);
    RUN_TEST_SUITE(DmaConfiguration);
    RUN_TEST_SUITE(UartDmaIntegration);

    PRINT_TEST_SUMMARY();
    return TEST_RESULT();
}
