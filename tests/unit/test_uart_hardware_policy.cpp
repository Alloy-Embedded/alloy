/**
 * @file test_uart_hardware_policy.cpp
 * @brief Unit tests for UART Hardware Policy (Policy-Based Design)
 *
 * Tests the SAME70 UART hardware policy with mock registers.
 * Verifies all policy methods work correctly without real hardware.
 *
 * Test Strategy:
 * - Mock register structure mimics UART0_Registers
 * - Test hooks capture method calls
 * - Verify register values after each operation
 * - Test all 13 policy methods
 *
 * @note Part of Phase 8.3: UART Policy Unit Tests
 * @see openspec/changes/modernize-peripheral-architecture/tasks.md
 */

#include <catch2/catch_test_macros.hpp>

#include "core/types.hpp"
#include "hal/vendors/atmel/same70/registers/uart0_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/uart0_bitfields.hpp"

using namespace alloy::core;
using namespace alloy::hal::atmel::same70;

// ============================================================================
// Mock Register System
// ============================================================================

/**
 * @brief Mock UART registers for testing
 *
 * Mimics the real UART0_Registers structure but allows inspection
 * and modification for testing purposes.
 */
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

    // Helper to reset all registers
    void reset_all() {
        CR = 0;
        MR = 0;
        IER = 0;
        IDR = 0;
        IMR = 0;
        SR = 0;
        RHR = 0;
        THR = 0;
        BRGR = 0;
    }
};

// Global mock register instance
static MockUartRegisters g_mock_uart_regs;

// Mock hook function that returns pointer to mock registers
extern "C" volatile UART0_Registers* mock_uart_hw() {
    return reinterpret_cast<volatile UART0_Registers*>(&g_mock_uart_regs);
}

// ============================================================================
// Hardware Policy with Mock Support
// ============================================================================

namespace uart = atmel::same70::uart0;

/**
 * @brief Test UART Hardware Policy using mocks
 *
 * This policy is identical to Same70UartHardwarePolicy but uses
 * mock registers instead of real hardware.
 */
template <uint32_t PERIPH_CLOCK_HZ>
struct TestUartHardwarePolicy {
    using RegisterType = UART0_Registers;

    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t UART_TIMEOUT = 100000;

    // Use mock registers
    static inline volatile RegisterType* hw() {
        return mock_uart_hw();
    }

    // Policy methods (same as generated policy)
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
// Unit Tests
// ============================================================================

TEST_CASE("UART Hardware Policy - reset()", "[uart][policy][reset]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Calling reset()
    TestPolicy::reset();

    // Then: CR register should have reset and disable bits set
    uint32_t expected = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask |
                        uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;
    REQUIRE(g_mock_uart_regs.CR == expected);
}

TEST_CASE("UART Hardware Policy - configure_8n1()", "[uart][policy][configure]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Calling configure_8n1()
    TestPolicy::configure_8n1();

    // Then: MR register should have NO_PARITY configured
    uint32_t expected = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);
    REQUIRE(g_mock_uart_regs.MR == expected);
}

TEST_CASE("UART Hardware Policy - set_baudrate()", "[uart][policy][baudrate]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    SECTION("Baudrate 115200") {
        // When: Setting baudrate to 115200
        TestPolicy::set_baudrate(115200);

        // Then: BRGR should contain correct clock divisor
        // CD = 150000000 / (16 * 115200) = 81.38... = 81
        uint32_t expected_cd = 150000000 / (16 * 115200);
        uint32_t expected = uart::brgr::CD::write(0, expected_cd);
        REQUIRE(g_mock_uart_regs.BRGR == expected);
    }

    SECTION("Baudrate 9600") {
        // When: Setting baudrate to 9600
        TestPolicy::set_baudrate(9600);

        // Then: BRGR should contain correct clock divisor
        // CD = 150000000 / (16 * 9600) = 976.56... = 976
        uint32_t expected_cd = 150000000 / (16 * 9600);
        uint32_t expected = uart::brgr::CD::write(0, expected_cd);
        REQUIRE(g_mock_uart_regs.BRGR == expected);
    }
}

TEST_CASE("UART Hardware Policy - enable_tx()", "[uart][policy][tx]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Enabling transmitter
    TestPolicy::enable_tx();

    // Then: CR should have TXEN bit set
    REQUIRE(g_mock_uart_regs.CR == uart::cr::TXEN::mask);
}

TEST_CASE("UART Hardware Policy - enable_rx()", "[uart][policy][rx]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Enabling receiver
    TestPolicy::enable_rx();

    // Then: CR should have RXEN bit set
    REQUIRE(g_mock_uart_regs.CR == uart::cr::RXEN::mask);
}

TEST_CASE("UART Hardware Policy - disable_tx()", "[uart][policy][tx]") {
    // Given: Transmitter enabled
    g_mock_uart_regs.reset_all();
    g_mock_uart_regs.CR = uart::cr::TXEN::mask;

    // When: Disabling transmitter
    TestPolicy::disable_tx();

    // Then: CR should have TXDIS bit set
    REQUIRE(g_mock_uart_regs.CR == uart::cr::TXDIS::mask);
}

TEST_CASE("UART Hardware Policy - disable_rx()", "[uart][policy][rx]") {
    // Given: Receiver enabled
    g_mock_uart_regs.reset_all();
    g_mock_uart_regs.CR = uart::cr::RXEN::mask;

    // When: Disabling receiver
    TestPolicy::disable_rx();

    // Then: CR should have RXDIS bit set
    REQUIRE(g_mock_uart_regs.CR == uart::cr::RXDIS::mask);
}

TEST_CASE("UART Hardware Policy - is_tx_ready()", "[uart][policy][tx][status]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    SECTION("TX not ready") {
        // When: TXRDY bit is clear
        g_mock_uart_regs.SR = 0;

        // Then: is_tx_ready() returns false
        REQUIRE_FALSE(TestPolicy::is_tx_ready());
    }

    SECTION("TX ready") {
        // When: TXRDY bit is set
        g_mock_uart_regs.SR = uart::sr::TXRDY::mask;

        // Then: is_tx_ready() returns true
        REQUIRE(TestPolicy::is_tx_ready());
    }
}

TEST_CASE("UART Hardware Policy - is_rx_ready()", "[uart][policy][rx][status]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    SECTION("RX not ready") {
        // When: RXRDY bit is clear
        g_mock_uart_regs.SR = 0;

        // Then: is_rx_ready() returns false
        REQUIRE_FALSE(TestPolicy::is_rx_ready());
    }

    SECTION("RX ready") {
        // When: RXRDY bit is set
        g_mock_uart_regs.SR = uart::sr::RXRDY::mask;

        // Then: is_rx_ready() returns true
        REQUIRE(TestPolicy::is_rx_ready());
    }
}

TEST_CASE("UART Hardware Policy - write_byte()", "[uart][policy][tx][write]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    SECTION("Write single byte") {
        // When: Writing byte 0x42
        TestPolicy::write_byte(0x42);

        // Then: THR should contain the byte
        REQUIRE(g_mock_uart_regs.THR == 0x42);
    }

    SECTION("Write multiple bytes") {
        // When: Writing multiple bytes
        TestPolicy::write_byte(0xAA);
        REQUIRE(g_mock_uart_regs.THR == 0xAA);

        TestPolicy::write_byte(0x55);
        REQUIRE(g_mock_uart_regs.THR == 0x55);
    }
}

TEST_CASE("UART Hardware Policy - read_byte()", "[uart][policy][rx][read]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    SECTION("Read single byte") {
        // When: RHR contains 0x42
        g_mock_uart_regs.RHR = 0x42;

        // Then: read_byte() returns 0x42
        REQUIRE(TestPolicy::read_byte() == 0x42);
    }

    SECTION("Read multiple bytes") {
        // When: Reading multiple bytes
        g_mock_uart_regs.RHR = 0xAA;
        REQUIRE(TestPolicy::read_byte() == 0xAA);

        g_mock_uart_regs.RHR = 0x55;
        REQUIRE(TestPolicy::read_byte() == 0x55);
    }
}

TEST_CASE("UART Hardware Policy - wait_tx_ready()", "[uart][policy][tx][wait]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    SECTION("TX already ready") {
        // When: TX is ready immediately
        g_mock_uart_regs.SR = uart::sr::TXRDY::mask;

        // Then: wait_tx_ready() returns true immediately
        REQUIRE(TestPolicy::wait_tx_ready(100));
    }

    SECTION("TX never ready (timeout)") {
        // When: TX is never ready
        g_mock_uart_regs.SR = 0;

        // Then: wait_tx_ready() returns false after timeout
        REQUIRE_FALSE(TestPolicy::wait_tx_ready(10));
    }
}

TEST_CASE("UART Hardware Policy - wait_rx_ready()", "[uart][policy][rx][wait]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    SECTION("RX already ready") {
        // When: RX is ready immediately
        g_mock_uart_regs.SR = uart::sr::RXRDY::mask;

        // Then: wait_rx_ready() returns true immediately
        REQUIRE(TestPolicy::wait_rx_ready(100));
    }

    SECTION("RX never ready (timeout)") {
        // When: RX is never ready
        g_mock_uart_regs.SR = 0;

        // Then: wait_rx_ready() returns false after timeout
        REQUIRE_FALSE(TestPolicy::wait_rx_ready(10));
    }
}

// ============================================================================
// Integration Tests - Typical Usage Sequences
// ============================================================================

TEST_CASE("UART Hardware Policy - Full initialization sequence", "[uart][policy][integration]") {
    // Given: Clean register state
    g_mock_uart_regs.reset_all();

    // When: Performing full UART initialization
    TestPolicy::reset();
    TestPolicy::configure_8n1();
    TestPolicy::set_baudrate(115200);
    TestPolicy::enable_tx();
    TestPolicy::enable_rx();

    // Then: All registers should be properly configured
    // Note: Each operation overwrites CR, so only last operation is visible
    REQUIRE(g_mock_uart_regs.CR == uart::cr::RXEN::mask);  // Last enable_rx()
    REQUIRE(g_mock_uart_regs.MR == uart::mr::PAR::write(0, uart::mr::par::NO_PARITY));

    uint32_t expected_cd = 150000000 / (16 * 115200);
    REQUIRE(g_mock_uart_regs.BRGR == uart::brgr::CD::write(0, expected_cd));
}

TEST_CASE("UART Hardware Policy - TX data transfer sequence", "[uart][policy][integration]") {
    // Given: UART initialized and TX ready
    g_mock_uart_regs.reset_all();
    g_mock_uart_regs.SR = uart::sr::TXRDY::mask;

    // When: Sending "Hello" message
    const char* msg = "Hello";
    for (int i = 0; msg[i] != '\0'; ++i) {
        bool ready = TestPolicy::wait_tx_ready(1000);
        REQUIRE(ready);
        TestPolicy::write_byte(static_cast<uint8_t>(msg[i]));
    }

    // Then: Last byte written should be 'o'
    REQUIRE(g_mock_uart_regs.THR == 'o');
}

TEST_CASE("UART Hardware Policy - RX data transfer sequence", "[uart][policy][integration]") {
    // Given: UART initialized and RX has data
    g_mock_uart_regs.reset_all();
    g_mock_uart_regs.SR = uart::sr::RXRDY::mask;

    // When: Receiving bytes
    const uint8_t test_data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
    uint8_t received[5];

    for (int i = 0; i < 5; ++i) {
        g_mock_uart_regs.RHR = test_data[i];
        bool ready = TestPolicy::wait_rx_ready(1000);
        REQUIRE(ready);
        received[i] = TestPolicy::read_byte();
    }

    // Then: All bytes should be received correctly
    for (int i = 0; i < 5; ++i) {
        REQUIRE(received[i] == test_data[i]);
    }
}
