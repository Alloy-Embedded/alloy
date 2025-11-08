/**
 * @file main.cpp
 * @brief SAME70 UART Template Example - Zero Overhead HAL
 *
 * This example demonstrates the NEW template-based UART implementation.
 * Features:
 * - ZERO virtual functions
 * - ZERO runtime overhead
 * - Compile-time validation via concepts
 * - Identical assembly to manual register access
 *
 * Hardware: Atmel SAME70 Xplained
 * UART: UART0 (connected to EDBG USB via PA9/PA10)
 * Baudrate: 115200
 *
 * Connect via USB and open serial terminal at 115200 baud.
 */

#include "hal/platform/same70/uart.hpp"

#include "board.hpp"

using namespace alloy::hal::same70;
using namespace alloy::hal;
using namespace alloy::core;

// Helper function to send string via UART
template <typename TUart>
void uart_puts(TUart& uart, const char* str) {
    size_t len = 0;
    while (str[len] != '\0')
        len++;
    uart.write(reinterpret_cast<const uint8_t*>(str), len);
}

int main() {
    // Initialize board (clocks, GPIO ports, etc.)
    Board::initialize();

    // Initialize LED for visual feedback
    Board::Led::init();

    // Create UART0 instance using type alias
    // This is resolved entirely at compile-time!
    auto uart = Uart0{};

    // Open UART (enables clock, configures peripheral)
    if (auto result = uart.open(); result.is_error()) {
        // Error opening UART - fast blink LED
        while (true) {
            Board::Led::toggle();
            Board::delay_ms(100);
        }
    }

    // Set baudrate to 115200
    uart.setBaudrate(Baudrate::e115200);

    // Send welcome message
    uart_puts(uart, "\r\n");
    uart_puts(uart, "==================================\r\n");
    uart_puts(uart, "SAME70 Template UART Example\r\n");
    uart_puts(uart, "==================================\r\n");
    uart_puts(uart, "Platform: template-based (zero overhead)\r\n");
    uart_puts(uart, "Baudrate: 115200\r\n");
    uart_puts(uart, "Concept: UartConcept validated!\r\n");
    uart_puts(uart, "==================================\r\n\r\n");

    // Main loop: send counter and toggle LED
    uint32_t counter = 0;

    while (true) {
        // Toggle LED (visual feedback)
        Board::Led::toggle();

        // Format message
        char buffer[64];
        int len = 0;

        // Simple integer to string conversion
        uint32_t num = counter;
        char temp[16];
        int idx = 0;

        if (num == 0) {
            temp[idx++] = '0';
        } else {
            while (num > 0 && idx < 15) {
                temp[idx++] = '0' + (num % 10);
                num /= 10;
            }
        }

        // Copy to buffer (reverse)
        len = 0;
        const char* prefix = "Counter: ";
        while (*prefix)
            buffer[len++] = *prefix++;

        for (int i = idx - 1; i >= 0; i--) {
            buffer[len++] = temp[i];
        }

        buffer[len++] = '\r';
        buffer[len++] = '\n';

        // Send message
        uart.write(reinterpret_cast<const uint8_t*>(buffer), len);

        // Increment counter
        counter++;

        // Wait 1 second
        Board::delay_ms(1000);
    }

    // Never reached, but demonstrate proper cleanup
    uart.close();

    return 0;
}
