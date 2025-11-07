/**
 * @file uart_test.cpp
 * @brief Unit tests for SAME70 UART template implementation
 *
 * These tests verify the UART template behavior without requiring hardware.
 * Uses mock registers to simulate hardware behavior.
 *
 * Test coverage:
 * - Open/close lifecycle
 * - Write operations
 * - Read operations
 * - Baudrate configuration
 * - Error handling
 * - State management
 */

#include <catch2/catch_test_macros.hpp>
#include "uart_mock.hpp"

// Define mock injection macros BEFORE including uart.hpp
#define ALLOY_UART_MOCK_HW() (reinterpret_cast<volatile alloy::hal::atmel::same70::atsame70q21::uart0::UART0_Registers*>(alloy::hal::test::g_mock_uart))
#define ALLOY_UART_MOCK_PMC() (&alloy::hal::test::g_mock_pmc->PCER0)

// Define test hooks for register access
#define ALLOY_UART_TEST_HOOK_THR(hw, value) alloy::hal::test::g_mock_uart->transmitted_data.push_back(value)
#define ALLOY_UART_TEST_HOOK_RHR(hw) alloy::hal::test::g_mock_uart->sync_rhr()

#include "hal/platform/same70/uart.hpp"

using namespace alloy::hal::same70;
using namespace alloy::hal;
using namespace alloy::core;
using namespace alloy::hal::test;

// ============================================================================
// Test Fixture
// ============================================================================

/**
 * @brief Test fixture for UART tests
 *
 * Sets up mock environment for each test case.
 */
class UartTestFixture {
public:
    UartTestFixture() {
        // Mock fixture sets up global mocks
    }

    MockUartRegisters& uart_regs() { return mock.uart(); }
    MockPmc& pmc() { return mock.pmc(); }

private:
    MockUartFixture mock;
};

// ============================================================================
// Open/Close Lifecycle Tests
// ============================================================================

TEST_CASE("UART open() enables peripheral clock", "[uart][lifecycle]") {
    UartTestFixture fixture;

    auto uart = Uart0{};

    SECTION("Clock is initially disabled") {
        REQUIRE_FALSE(fixture.pmc().is_clock_enabled(7));
    }

    SECTION("open() enables clock") {
        auto result = uart.open();

        REQUIRE(result.is_ok());
        REQUIRE(fixture.pmc().is_clock_enabled(7));
    }
}

TEST_CASE("UART open() configures registers correctly", "[uart][lifecycle]") {
    UartTestFixture fixture;
    auto uart = Uart0{};

    auto result = uart.open();
    REQUIRE(result.is_ok());

    SECTION("Mode register is configured") {
        // Should be set to 8-bit, no parity (PAR = 4)
        uint32_t expected_mr = (4u << 9);  // PAR_NO at bit 9
        REQUIRE(fixture.uart_regs().MR == expected_mr);
    }

    SECTION("Baudrate is set to default 115200") {
        // BRGR = 150MHz / (16 * 115200) = 81
        REQUIRE(fixture.uart_regs().BRGR == 81);
    }

    SECTION("Transmitter and receiver are enabled") {
        // CR should have RXEN (bit 4) and TXEN (bit 6) set
        // Note: This checks the last write to CR
        REQUIRE((fixture.uart_regs().CR & (1u << 4)) != 0);  // RXEN
        REQUIRE((fixture.uart_regs().CR & (1u << 6)) != 0);  // TXEN
    }
}

TEST_CASE("UART isOpen() reflects state correctly", "[uart][state]") {
    UartTestFixture fixture;
    auto uart = Uart0{};

    SECTION("Initially closed") {
        REQUIRE_FALSE(uart.isOpen());
    }

    SECTION("Open after open()") {
        uart.open();
        REQUIRE(uart.isOpen());
    }

    SECTION("Closed after close()") {
        uart.open();
        uart.close();
        REQUIRE_FALSE(uart.isOpen());
    }
}

TEST_CASE("UART cannot open twice", "[uart][lifecycle][error]") {
    UartTestFixture fixture;
    auto uart = Uart0{};

    uart.open();

    SECTION("Second open() returns error") {
        auto result = uart.open();

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::AlreadyInitialized);
    }
}

TEST_CASE("UART close() disables transmitter and receiver", "[uart][lifecycle]") {
    UartTestFixture fixture;
    auto uart = Uart0{};

    uart.open();
    uart.close();

    SECTION("RXDIS and TXDIS bits are set") {
        // CR should have RXDIS (bit 5) and TXDIS (bit 7) set
        REQUIRE((fixture.uart_regs().CR & (1u << 5)) != 0);  // RXDIS
        REQUIRE((fixture.uart_regs().CR & (1u << 7)) != 0);  // TXDIS
    }
}

TEST_CASE("UART operations fail when not open", "[uart][error]") {
    UartTestFixture fixture;
    auto uart = Uart0{};

    uint8_t dummy_data[] = {0x01, 0x02};

    SECTION("write() fails when not open") {
        auto result = uart.write(dummy_data, 2);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }

    SECTION("read() fails when not open") {
        auto result = uart.read(dummy_data, 2);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }

    SECTION("setBaudrate() fails when not open") {
        auto result = uart.setBaudrate(Baudrate::e9600);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }

    SECTION("getBaudrate() fails when not open") {
        auto result = uart.getBaudrate();

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotInitialized);
    }
}

// ============================================================================
// Write Operation Tests
// ============================================================================

TEST_CASE("UART write() transmits data correctly", "[uart][write]") {
    UartTestFixture fixture;
    auto uart = Uart0{};
    uart.open();

    // Simulate transmitter always ready
    fixture.uart_regs().set_tx_ready(true);

    SECTION("Write single byte") {
        uint8_t data[] = {'A'};
        auto result = uart.write(data, 1);

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 1);
        REQUIRE(fixture.uart_regs().transmitted_data.size() == 1);
        REQUIRE(fixture.uart_regs().transmitted_data[0] == 'A');
    }

    SECTION("Write multiple bytes") {
        const char* message = "Hello";
        auto result = uart.write(reinterpret_cast<const uint8_t*>(message), 5);

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 5);
        REQUIRE(fixture.uart_regs().transmitted_matches("Hello"));
    }

    SECTION("Write empty data (size = 0)") {
        uint8_t data[] = {0x00};
        auto result = uart.write(data, 0);

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 0);
        REQUIRE(fixture.uart_regs().transmitted_data.empty());
    }
}

TEST_CASE("UART write() validates parameters", "[uart][write][error]") {
    UartTestFixture fixture;
    auto uart = Uart0{};
    uart.open();

    SECTION("Null pointer returns error") {
        auto result = uart.write(nullptr, 10);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::InvalidParameter);
    }
}

// ============================================================================
// Read Operation Tests
// ============================================================================

TEST_CASE("UART read() receives data correctly", "[uart][read]") {
    UartTestFixture fixture;
    auto uart = Uart0{};
    uart.open();

    SECTION("Read single byte") {
        // Queue data to be received
        uint8_t queued[] = {'X'};
        fixture.uart_regs().queue_receive_data(queued, 1);

        uint8_t received[1];
        auto result = uart.read(received, 1);

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 1);
        REQUIRE(received[0] == 'X');
    }

    SECTION("Read multiple bytes") {
        // Queue data
        const char* queued = "Test";
        fixture.uart_regs().queue_receive_data(
            reinterpret_cast<const uint8_t*>(queued), 4);

        uint8_t received[4];
        auto result = uart.read(received, 4);

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 4);
        REQUIRE(std::memcmp(received, "Test", 4) == 0);
    }

    SECTION("Read with timeout (no data available)") {
        uint8_t received[10];
        auto result = uart.read(received, 10);

        // Should timeout and return 0 bytes
        REQUIRE(result.is_ok());
        REQUIRE(result.value() == 0);
    }
}

TEST_CASE("UART read() validates parameters", "[uart][read][error]") {
    UartTestFixture fixture;
    auto uart = Uart0{};
    uart.open();

    SECTION("Null pointer returns error") {
        auto result = uart.read(nullptr, 10);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::InvalidParameter);
    }
}

// ============================================================================
// Baudrate Configuration Tests
// ============================================================================

TEST_CASE("UART setBaudrate() configures BRGR correctly", "[uart][baudrate]") {
    UartTestFixture fixture;
    auto uart = Uart0{};
    uart.open();

    SECTION("Set to 9600 baud") {
        auto result = uart.setBaudrate(Baudrate::e9600);

        REQUIRE(result.is_ok());
        // BRGR = 150MHz / (16 * 9600) = 976.5 ≈ 976
        REQUIRE(fixture.uart_regs().BRGR == 976);
    }

    SECTION("Set to 115200 baud") {
        auto result = uart.setBaudrate(Baudrate::e115200);

        REQUIRE(result.is_ok());
        // BRGR = 150MHz / (16 * 115200) = 81.38 ≈ 81
        REQUIRE(fixture.uart_regs().BRGR == 81);
    }

    SECTION("Set to 921600 baud") {
        auto result = uart.setBaudrate(Baudrate::e921600);

        REQUIRE(result.is_ok());
        // BRGR = 150MHz / (16 * 921600) = 10.17 ≈ 10
        REQUIRE(fixture.uart_regs().BRGR == 10);
    }
}

TEST_CASE("UART getBaudrate() returns current baudrate", "[uart][baudrate]") {
    UartTestFixture fixture;
    auto uart = Uart0{};
    uart.open();

    SECTION("Default baudrate is 115200") {
        auto result = uart.getBaudrate();

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == Baudrate::e115200);
    }

    SECTION("Returns updated baudrate after set") {
        uart.setBaudrate(Baudrate::e9600);

        auto result = uart.getBaudrate();

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == Baudrate::e9600);
    }
}

// ============================================================================
// Concept Validation Tests
// ============================================================================

TEST_CASE("UART satisfies UartConcept", "[uart][concept]") {
    // This is a compile-time check via static_assert in uart.hpp
    // If this test compiles, the concept is satisfied

    SECTION("Uart0 satisfies concept") {
#if __cplusplus >= 202002L
        STATIC_REQUIRE(concepts::UartConcept<Uart0>);
#else
        STATIC_REQUIRE(concepts::is_uart_v<Uart0>);
#endif
    }

    SECTION("Uart1 satisfies concept") {
#if __cplusplus >= 202002L
        STATIC_REQUIRE(concepts::UartConcept<Uart1>);
#else
        STATIC_REQUIRE(concepts::is_uart_v<Uart1>);
#endif
    }
}

// ============================================================================
// Type Alias Tests
// ============================================================================

TEST_CASE("UART type aliases are distinct types", "[uart][types]") {
    SECTION("Uart0 and Uart1 have different base addresses") {
        STATIC_REQUIRE(Uart0::base_address != Uart1::base_address);
    }

    SECTION("Uart0 has correct base address") {
        STATIC_REQUIRE(Uart0::base_address == 0x400E0800);
    }

    SECTION("Uart1 has correct base address") {
        STATIC_REQUIRE(Uart1::base_address == 0x400E0A00);
    }

    SECTION("Uart0 has correct IRQ ID") {
        STATIC_REQUIRE(Uart0::irq_id == 7);
    }

    SECTION("Uart1 has correct IRQ ID") {
        STATIC_REQUIRE(Uart1::irq_id == 8);
    }
}
