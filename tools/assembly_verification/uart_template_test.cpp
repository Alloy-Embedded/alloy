/**
 * @file uart_template_test.cpp
 * @brief Assembly verification test - Template UART vs Manual Register Access
 *
 * This file contains two implementations of the same functionality:
 * 1. Template-based UART (using HAL)
 * 2. Manual register access (direct)
 *
 * Compile with:
 *   arm-none-eabi-g++ -mcpu=cortex-m7 -mthumb -O2 -S uart_template_test.cpp
 *
 * Then compare generated assembly for test_template() vs test_manual()
 * They should be IDENTICAL (proving zero overhead).
 */

#include "hal/platform/same70/uart.hpp"
#include <cstdint>

// ============================================================================
// Manual Register Access (Baseline)
// ============================================================================

// UART0 registers (manual definition)
struct UART0_Regs {
    volatile uint32_t CR;      // 0x0000 - Control Register
    volatile uint32_t MR;      // 0x0004 - Mode Register
    volatile uint32_t IER;     // 0x0008 - Interrupt Enable Register
    volatile uint32_t IDR;     // 0x000C - Interrupt Disable Register
    volatile uint32_t IMR;     // 0x0010 - Interrupt Mask Register
    volatile uint32_t SR;      // 0x0014 - Status Register
    volatile uint32_t RHR;     // 0x0018 - Receive Holding Register
    volatile uint32_t THR;     // 0x001C - Transmit Holding Register
    volatile uint32_t BRGR;    // 0x0020 - Baud Rate Generator Register
};

#define UART0_BASE  0x400E0800
#define PMC_PCER0   (*reinterpret_cast<volatile uint32_t*>(0x400E0610))

// Control register bits
#define UART_CR_RXEN    (1u << 4)
#define UART_CR_TXEN    (1u << 6)
#define UART_CR_RSTRX   (1u << 2)
#define UART_CR_RSTTX   (1u << 3)
#define UART_CR_RSTSTA  (1u << 8)

// Status register bits
#define UART_SR_TXRDY   (1u << 1)

// Mode register bits
#define UART_MR_PAR_NO  (4u << 9)

// ============================================================================
// Test Function 1: Manual Register Access
// ============================================================================

__attribute__((noinline))
void test_manual(const uint8_t* data, size_t size) {
    volatile UART0_Regs* uart = reinterpret_cast<volatile UART0_Regs*>(UART0_BASE);

    // Enable peripheral clock
    PMC_PCER0 = (1u << 7);

    // Reset and disable receiver/transmitter
    uart->CR = UART_CR_RSTRX | UART_CR_RSTTX;

    // Configure mode: 8-bit, no parity
    uart->MR = UART_MR_PAR_NO;

    // Set baud rate (115200 @ 150MHz peripheral clock)
    // BRGR = 150000000 / (16 * 115200) = 81.38 ≈ 81
    uart->BRGR = 81;

    // Reset status
    uart->CR = UART_CR_RSTSTA;

    // Enable receiver and transmitter
    uart->CR = UART_CR_RXEN | UART_CR_TXEN;

    // Write data
    for (size_t i = 0; i < size; ++i) {
        // Wait for TXRDY
        while (!(uart->SR & UART_SR_TXRDY)) {
            // Busy wait
        }

        // Write byte
        uart->THR = data[i];
    }
}

// ============================================================================
// Test Function 2: Template HAL
// ============================================================================

__attribute__((noinline))
void test_template(const uint8_t* data, size_t size) {
    using namespace alloy::hal::same70;

    // Create UART0 instance
    auto uart = Uart0{};

    // Open UART
    uart.open();

    // Set baudrate to 115200
    uart.setBaudrate(alloy::hal::Baudrate::e115200);

    // Write data
    uart.write(data, size);
}

// ============================================================================
// Test Function 3: Template HAL (Inline-friendly version)
// ============================================================================

__attribute__((noinline))
void test_template_inline(const uint8_t* data, size_t size) {
    using namespace alloy::hal::same70;

    // Create UART0 instance (zero-cost abstraction)
    volatile auto* HW =
        reinterpret_cast<volatile alloy::hal::atmel::same70::atsame70q21::uart0::UART0_Registers*>(0x400E0800);

    volatile uint32_t* PMC =
        reinterpret_cast<volatile uint32_t*>(0x400E0610);

    // Enable clock
    *PMC = (1u << 7);

    // Reset
    HW->CR = (1u << 2) | (1u << 3);

    // Configure mode
    HW->MR = (4u << 9);

    // Set baudrate
    HW->BRGR = 81;

    // Reset status
    HW->CR = (1u << 8);

    // Enable TX/RX
    HW->CR = (1u << 4) | (1u << 6);

    // Write loop
    for (size_t i = 0; i < size; ++i) {
        while (!(HW->SR & (1u << 1))) {}
        HW->THR = data[i];
    }
}

// ============================================================================
// Main (for compilation test)
// ============================================================================

// Test data
static const uint8_t test_data[] = "Hello, World!\r\n";

int main() {
    // Call both versions
    test_manual(test_data, sizeof(test_data) - 1);
    test_template(test_data, sizeof(test_data) - 1);
    test_template_inline(test_data, sizeof(test_data) - 1);

    return 0;
}
