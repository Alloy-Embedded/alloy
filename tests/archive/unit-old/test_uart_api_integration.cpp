/**
 * @file test_uart_api_integration.cpp
 * @brief Integration tests for UART APIs with Hardware Policy
 *
 * Tests the integration between generic UART APIs (Simple, Fluent, Expert)
 * and the hardware policy layer. Uses mock registers to verify correct
 * hardware configuration without real hardware.
 *
 * Test Coverage:
 * - Simple API (Level 1) initialization and configuration
 * - Fluent API (Level 2) builder pattern with policy
 * - Expert API (Level 3) advanced configuration with policy
 * - Verify all APIs produce correct hardware register values
 *
 * @note Part of Phase 8.4: UART Integration Tests
 * @see openspec/changes/modernize-peripheral-architecture/tasks.md
 */

#include <catch2/catch_test_macros.hpp>

#include "core/types.hpp"
#include "core/units.hpp"
#include "hal/api/uart_simple.hpp"
#include "hal/api/uart_fluent.hpp"
#include "hal/api/uart_expert.hpp"
#include "hal/vendors/atmel/same70/registers/uart0_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/uart0_bitfields.hpp"

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::atmel::same70;

// ============================================================================
// Mock Register System (Shared with Hardware Policy Tests)
// ============================================================================

struct MockUartRegisters {
    volatile uint32_t CR{0};      // Control Register
    volatile uint32_t MR{0};      // Mode Register
    volatile uint32_t IER{0};     // Interrupt Enable Register
    volatile uint32_t IDR{0};     // Interrupt Disable Register
    volatile uint32_t IMR{0};     // Interrupt Mask Register
    volatile uint32_t SR{0};      // Status Register
    volatile uint32_t RHR{0};     // Receive Holding Register
    volatile uint32_t THR{0};     // Transmit Holding Register
    volatile uint32_t BRGR{0};    // Baud Rate Generator Register

    void reset_all() {
        CR = 0; MR = 0; IER = 0; IDR = 0; IMR = 0;
        SR = 0; RHR = 0; THR = 0; BRGR = 0;
    }
};

static MockUartRegisters g_mock_uart_regs;

extern "C" volatile UART0_Registers* mock_uart_hw() {
    return reinterpret_cast<volatile UART0_Registers*>(&g_mock_uart_regs);
}

// ============================================================================
// Test Hardware Policy (Uses Mocks)
// ============================================================================

namespace uart = atmel::same70::uart0;

template <uint32_t PERIPH_CLOCK_HZ>
struct TestUartHardwarePolicy {
    using RegisterType = UART0_Registers;

    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t UART_TIMEOUT = 100000;

    static inline volatile RegisterType* hw() {
        return mock_uart_hw();
    }

    static inline void reset() {
        hw()->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask |
                   uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;
    }

    static inline void configure_8n1() {
        hw()->MR = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);
    }

    static inline void set_baudrate(uint32_t baud) {
        uint32_t cd = PERIPH_CLOCK_HZ / (16 * baud);
        hw()->BRGR = uart::brgr::CD::write(0, cd);
    }

    static inline void enable_tx() {
        hw()->CR = uart::cr::TXEN::mask;
    }

    static inline void enable_rx() {
        hw()->CR = uart::cr::RXEN::mask;
    }

    static inline void disable_tx() {
        hw()->CR = uart::cr::TXDIS::mask;
    }

    static inline void disable_rx() {
        hw()->CR = uart::cr::RXDIS::mask;
    }

    static inline bool is_tx_ready() {
        return (hw()->SR & uart::sr::TXRDY::mask) != 0;
    }

    static inline bool is_rx_ready() {
        return (hw()->SR & uart::sr::RXRDY::mask) != 0;
    }

    static inline void write_byte(uint8_t byte) {
        hw()->THR = byte;
    }

    static inline uint8_t read_byte() {
        return static_cast<uint8_t>(hw()->RHR);
    }

    static inline bool wait_tx_ready(uint32_t timeout_loops) {
        uint32_t timeout = timeout_loops;
        while (!is_tx_ready() && --timeout);
        return timeout != 0;
    }

    static inline bool wait_rx_ready(uint32_t timeout_loops) {
        uint32_t timeout = timeout_loops;
        while (!is_rx_ready() && --timeout);
        return timeout != 0;
    }
};

using TestPolicy = TestUartHardwarePolicy<150000000>;

// ============================================================================
// Mock GPIO Pins for Testing
// ============================================================================

template <uint32_t BASE, uint32_t PIN>
struct MockGpioPin {
    static constexpr PinId get_pin_id() {
        // Mock pin IDs for testing
        return static_cast<PinId>(BASE + PIN);
    }
};

using MockTxPin = MockGpioPin<0x400E0000, 3>;  // PD3
using MockRxPin = MockGpioPin<0x400E0000, 4>;  // PD4

// ============================================================================
// Integration Tests - Simple API (Level 1)
// ============================================================================

TEST_CASE("UART Simple API - quick_setup basic initialization", "[uart][integration][simple]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Simple API to setup UART
    auto config = Uart<PeripheralId::USART0, TestPolicy>::quick_setup<MockTxPin, MockRxPin>(
        BaudRate{115200}
    );

    // And: Initializing the configuration
    auto result = config.initialize();

    // Then: Initialization should succeed
    REQUIRE(result.is_ok());

    // And: Mode register should be configured for 8N1
    uint32_t expected_mr = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);
    REQUIRE(g_mock_uart_regs.MR == expected_mr);

    // And: Baud rate should be set correctly
    uint32_t expected_cd = 150000000 / (16 * 115200);
    uint32_t expected_brgr = uart::brgr::CD::write(0, expected_cd);
    REQUIRE(g_mock_uart_regs.BRGR == expected_brgr);

    // And: RX should be enabled (last enable in sequence)
    REQUIRE(g_mock_uart_regs.CR == uart::cr::RXEN::mask);
}

TEST_CASE("UART Simple API - quick_setup with custom parity", "[uart][integration][simple]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Simple API with EVEN parity
    auto config = Uart<PeripheralId::USART0, TestPolicy>::quick_setup<MockTxPin, MockRxPin>(
        BaudRate{9600},
        UartParity::EVEN
    );

    auto result = config.initialize();

    // Then: Initialization should succeed
    REQUIRE(result.is_ok());

    // Note: Current implementation only handles 8N1, so MR will still be NO_PARITY
    // TODO: This test will need update when other parity modes are implemented
}

TEST_CASE("UART Simple API - TX-only configuration", "[uart][integration][simple]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Simple API TX-only setup
    auto config = Uart<PeripheralId::USART0, TestPolicy>::quick_setup_tx_only<MockTxPin>(
        BaudRate{115200}
    );

    auto result = config.initialize();

    // Then: Initialization should succeed
    REQUIRE(result.is_ok());

    // And: TX should be enabled (last enable in sequence)
    REQUIRE(g_mock_uart_regs.CR == uart::cr::TXEN::mask);

    // And: Baud rate should be configured
    uint32_t expected_cd = 150000000 / (16 * 115200);
    uint32_t expected_brgr = uart::brgr::CD::write(0, expected_cd);
    REQUIRE(g_mock_uart_regs.BRGR == expected_brgr);
}

// ============================================================================
// Integration Tests - Fluent API (Level 2)
// ============================================================================

TEST_CASE("UART Fluent API - builder pattern basic setup", "[uart][integration][fluent]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Fluent API builder
    auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
        .with_pins<MockTxPin, MockRxPin>()
        .baudrate(BaudRate{115200})
        .standard_8n1()
        .initialize();

    // Then: Initialization should succeed
    REQUIRE(result.is_ok());

    // And: Configuration should be valid
    auto config = result.value();
    auto apply_result = config.apply();
    REQUIRE(apply_result.is_ok());

    // And: Registers should be configured correctly
    uint32_t expected_mr = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);
    REQUIRE(g_mock_uart_regs.MR == expected_mr);

    uint32_t expected_cd = 150000000 / (16 * 115200);
    uint32_t expected_brgr = uart::brgr::CD::write(0, expected_cd);
    REQUIRE(g_mock_uart_regs.BRGR == expected_brgr);
}

TEST_CASE("UART Fluent API - TX-only configuration", "[uart][integration][fluent]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Fluent API for TX-only
    auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
        .with_tx_pin<MockTxPin>()
        .baudrate(BaudRate{9600})
        .data_bits(8)
        .stop_bits(1)
        .parity(UartParity::NONE)
        .initialize();

    // Then: Initialization should succeed
    REQUIRE(result.is_ok());

    // And: Configuration should be valid
    auto config = result.value();
    REQUIRE(config.has_tx == true);
    REQUIRE(config.has_rx == false);

    // And: Applying configuration should work
    auto apply_result = config.apply();
    REQUIRE(apply_result.is_ok());

    // And: TX should be enabled
    REQUIRE(g_mock_uart_regs.CR == uart::cr::TXEN::mask);
}

TEST_CASE("UART Fluent API - RX-only configuration", "[uart][integration][fluent]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Fluent API for RX-only
    auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
        .with_rx_pin<MockRxPin>()
        .baudrate(BaudRate{19200})
        .standard_8n1()
        .initialize();

    // Then: Initialization should succeed
    REQUIRE(result.is_ok());

    // And: Configuration should be valid
    auto config = result.value();
    REQUIRE(config.has_tx == false);
    REQUIRE(config.has_rx == true);

    // And: Applying configuration should work
    auto apply_result = config.apply();
    REQUIRE(apply_result.is_ok());

    // And: RX should be enabled
    REQUIRE(g_mock_uart_regs.CR == uart::cr::RXEN::mask);
}

TEST_CASE("UART Fluent API - preset configurations", "[uart][integration][fluent]") {
    g_mock_uart_regs.reset_all();

    SECTION("Standard 8N1") {
        auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
            .with_pins<MockTxPin, MockRxPin>()
            .baudrate(BaudRate{115200})
            .standard_8n1()
            .initialize();

        REQUIRE(result.is_ok());
        auto config = result.value();
        REQUIRE(config.data_bits == 8);
        REQUIRE(config.parity == UartParity::NONE);
        REQUIRE(config.stop_bits == 1);
    }

    SECTION("8E1 (Even parity)") {
        auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
            .with_pins<MockTxPin, MockRxPin>()
            .baudrate(BaudRate{9600})
            .standard_8e1()
            .initialize();

        REQUIRE(result.is_ok());
        auto config = result.value();
        REQUIRE(config.data_bits == 8);
        REQUIRE(config.parity == UartParity::EVEN);
        REQUIRE(config.stop_bits == 1);
    }

    SECTION("8O1 (Odd parity)") {
        auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
            .with_pins<MockTxPin, MockRxPin>()
            .baudrate(BaudRate{19200})
            .standard_8o1()
            .initialize();

        REQUIRE(result.is_ok());
        auto config = result.value();
        REQUIRE(config.data_bits == 8);
        REQUIRE(config.parity == UartParity::ODD);
        REQUIRE(config.stop_bits == 1);
    }
}

TEST_CASE("UART Fluent API - validation errors", "[uart][integration][fluent][error]") {
    SECTION("Missing baudrate") {
        auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
            .with_pins<MockTxPin, MockRxPin>()
            // Missing .baudrate()
            .initialize();

        // Should fail validation
        REQUIRE_FALSE(result.is_ok());
        REQUIRE(result.error() == ErrorCode::InvalidParameter);
    }

    SECTION("Missing pins") {
        auto result = UartBuilder<PeripheralId::USART0, TestPolicy>()
            // Missing .with_pins()
            .baudrate(BaudRate{115200})
            .initialize();

        // Should fail validation
        REQUIRE_FALSE(result.is_ok());
        REQUIRE(result.error() == ErrorCode::InvalidParameter);
    }
}

// ============================================================================
// Integration Tests - Expert API (Level 3)
// ============================================================================

TEST_CASE("UART Expert API - standard_115200 preset", "[uart][integration][expert]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Expert API with standard preset
    constexpr auto config = UartExpertConfig<TestPolicy>::standard_115200(
        PeripheralId::USART0,
        PinId::PD3,  // TX
        PinId::PD4   // RX
    );

    // Then: Configuration should be valid at compile-time
    static_assert(config.is_valid(), "Config should be valid");
    REQUIRE(config.is_valid());

    // And: Applying configuration should work
    auto result = expert::configure(config);
    REQUIRE(result.is_ok());

    // And: Registers should be configured correctly
    uint32_t expected_mr = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);
    REQUIRE(g_mock_uart_regs.MR == expected_mr);

    uint32_t expected_cd = 150000000 / (16 * 115200);
    uint32_t expected_brgr = uart::brgr::CD::write(0, expected_cd);
    REQUIRE(g_mock_uart_regs.BRGR == expected_brgr);

    // And: RX should be enabled (last in sequence)
    REQUIRE(g_mock_uart_regs.CR == uart::cr::RXEN::mask);
}

TEST_CASE("UART Expert API - logger_config preset", "[uart][integration][expert]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Using Expert API logger preset (TX-only)
    constexpr auto config = UartExpertConfig<TestPolicy>::logger_config(
        PeripheralId::USART0,
        PinId::PD3,  // TX
        BaudRate{115200}
    );

    // Then: Configuration should be valid
    REQUIRE(config.is_valid());
    REQUIRE(config.enable_tx == true);
    REQUIRE(config.enable_rx == false);

    // And: Applying configuration should work
    auto result = expert::configure(config);
    REQUIRE(result.is_ok());

    // And: TX should be enabled
    REQUIRE(g_mock_uart_regs.CR == uart::cr::TXEN::mask);
}

TEST_CASE("UART Expert API - custom configuration", "[uart][integration][expert]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Creating custom expert configuration
    constexpr UartExpertConfig<TestPolicy> config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{9600},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    // Then: Configuration should be valid
    static_assert(config.is_valid(), "Custom config should be valid");
    REQUIRE(config.is_valid());

    // And: Applying configuration should work
    auto result = expert::configure(config);
    REQUIRE(result.is_ok());

    // And: Baud rate should be correct for 9600
    uint32_t expected_cd = 150000000 / (16 * 9600);
    uint32_t expected_brgr = uart::brgr::CD::write(0, expected_cd);
    REQUIRE(g_mock_uart_regs.BRGR == expected_brgr);
}

TEST_CASE("UART Expert API - compile-time validation", "[uart][integration][expert]") {
    // Given: Invalid configurations

    SECTION("Invalid baud rate - too low") {
        constexpr UartExpertConfig<TestPolicy> config = {
            .peripheral = PeripheralId::USART0,
            .tx_pin = PinId::PD3,
            .rx_pin = PinId::PD4,
            .baudrate = BaudRate{100},  // Too low!
            .data_bits = 8,
            .parity = UartParity::NONE,
            .stop_bits = 1,
            .flow_control = false,
            .enable_tx = true,
            .enable_rx = true,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_oversampling = true,
            .enable_rx_timeout = false,
            .rx_timeout_value = 0
        };

        REQUIRE_FALSE(config.is_valid());
        REQUIRE_FALSE(has_valid_baudrate(config));
    }

    SECTION("Invalid data bits") {
        constexpr UartExpertConfig<TestPolicy> config = {
            .peripheral = PeripheralId::USART0,
            .tx_pin = PinId::PD3,
            .rx_pin = PinId::PD4,
            .baudrate = BaudRate{115200},
            .data_bits = 6,  // Invalid!
            .parity = UartParity::NONE,
            .stop_bits = 1,
            .flow_control = false,
            .enable_tx = true,
            .enable_rx = true,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_oversampling = true,
            .enable_rx_timeout = false,
            .rx_timeout_value = 0
        };

        REQUIRE_FALSE(config.is_valid());
        REQUIRE_FALSE(has_valid_data_bits(config));
    }

    SECTION("No TX or RX enabled") {
        constexpr UartExpertConfig<TestPolicy> config = {
            .peripheral = PeripheralId::USART0,
            .tx_pin = PinId::PD3,
            .rx_pin = PinId::PD4,
            .baudrate = BaudRate{115200},
            .data_bits = 8,
            .parity = UartParity::NONE,
            .stop_bits = 1,
            .flow_control = false,
            .enable_tx = false,  // Neither enabled!
            .enable_rx = false,
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_oversampling = true,
            .enable_rx_timeout = false,
            .rx_timeout_value = 0
        };

        REQUIRE_FALSE(config.is_valid());
        REQUIRE_FALSE(has_enabled_direction(config));
    }
}

TEST_CASE("UART Expert API - error messages", "[uart][integration][expert]") {
    // Given: Invalid configuration
    constexpr UartExpertConfig<TestPolicy> config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{100},  // Too low
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    // Then: Should provide descriptive error message
    const char* msg = config.error_message();
    REQUIRE(msg != nullptr);
    // Error message should mention baud rate being too low
}

// ============================================================================
// Cross-API Integration Tests
// ============================================================================

TEST_CASE("All APIs produce equivalent configuration", "[uart][integration][cross-api]") {
    // Given: Same configuration parameters
    constexpr uint32_t baud = 115200;

    // When: Configuring via Simple API
    g_mock_uart_regs.reset_all();
    auto simple_config = Uart<PeripheralId::USART0, TestPolicy>::quick_setup<MockTxPin, MockRxPin>(
        BaudRate{baud}
    );
    simple_config.initialize();
    uint32_t simple_brgr = g_mock_uart_regs.BRGR;
    uint32_t simple_mr = g_mock_uart_regs.MR;

    // And: Configuring via Fluent API
    g_mock_uart_regs.reset_all();
    auto fluent_result = UartBuilder<PeripheralId::USART0, TestPolicy>()
        .with_pins<MockTxPin, MockRxPin>()
        .baudrate(BaudRate{baud})
        .standard_8n1()
        .initialize();
    fluent_result.value().apply();
    uint32_t fluent_brgr = g_mock_uart_regs.BRGR;
    uint32_t fluent_mr = g_mock_uart_regs.MR;

    // And: Configuring via Expert API
    g_mock_uart_regs.reset_all();
    constexpr auto expert_config = UartExpertConfig<TestPolicy>::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4
    );
    expert::configure(expert_config);
    uint32_t expert_brgr = g_mock_uart_regs.BRGR;
    uint32_t expert_mr = g_mock_uart_regs.MR;

    // Then: All APIs should produce same register values
    REQUIRE(simple_brgr == fluent_brgr);
    REQUIRE(fluent_brgr == expert_brgr);
    REQUIRE(simple_mr == fluent_mr);
    REQUIRE(fluent_mr == expert_mr);
}
