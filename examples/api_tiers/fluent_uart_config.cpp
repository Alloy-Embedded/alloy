/**
 * @file fluent_uart_config.cpp
 * @brief Fluent Tier UART Example - Advanced Configuration
 *
 * Demonstrates the Fluent API tier for UART with custom configuration.
 *
 * Fluent Tier Features:
 * - Builder pattern with method chaining
 * - More configuration options than Simple tier
 * - Readable, self-documenting code
 * - Best for: Production code, custom protocols, debugging
 *
 * Hardware Requirements:
 * - Any supported board (nucleo_f401re, nucleo_f722ze, nucleo_g071rb, etc.)
 * - USB cable for ST-LINK virtual COM port
 * - Terminal software configured for: 9600 baud, 8E1 (even parity)
 *
 * Expected Behavior:
 * - Sends sensor data messages every second
 * - Uses even parity for error detection
 * - Lower baud rate (9600) for better reliability
 * - Format: 8E1 (8 data bits, even parity, 1 stop bit)
 *
 * @note Part of Phase 3.2: UART Platform Completeness
 * @see docs/API_TIERS.md for tier comparison
 */

#include "board.hpp"

// Platform-specific UART header
#if defined(MICROCORE_PLATFORM_STM32F4)
#include "hal/platform/stm32f4/uart.hpp"
using namespace ucore::hal::stm32f4;
#elif defined(MICROCORE_PLATFORM_STM32F7)
#include "hal/platform/stm32f7/uart.hpp"
using namespace ucore::hal::stm32f7;
#elif defined(MICROCORE_PLATFORM_STM32G0)
#include "hal/platform/stm32g0/uart.hpp"
using namespace ucore::hal::stm32g0;
#elif defined(MICROCORE_PLATFORM_STM32F1)
#include "hal/platform/stm32f1/uart.hpp"
using namespace ucore::hal::stm32f1;
#elif defined(MICROCORE_PLATFORM_SAME70)
#include "hal/platform/same70/uart.hpp"
using namespace ucore::hal::same70;
#else
#error "Unsupported platform for UART"
#endif

using namespace ucore::core;

// Board-specific UART pins
#if defined(MICROCORE_BOARD_NUCLEO_F401RE)
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PA2; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PA3; }
};
using UartBuilder = Usart2Builder;

#elif defined(MICROCORE_BOARD_NUCLEO_F722ZE)
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PD8; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PD9; }
};
using UartBuilder = Usart3Builder;

#elif defined(MICROCORE_BOARD_NUCLEO_G071RB) || defined(MICROCORE_BOARD_NUCLEO_G0B1RE)
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PA2; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PA3; }
};
using UartBuilder = Usart2Builder;

#elif defined(MICROCORE_BOARD_SAME70_XPLAINED)
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PB0; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PB1; }
};
using UartBuilder = Uart0Builder;

#else
#error "Unsupported board - add UART pin configuration"
#endif

/**
 * @brief Simple delay function (blocking)
 * @param ms Milliseconds to delay
 */
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 10000; i++) {
        __asm__ volatile("nop");
    }
}

/**
 * @brief Simple integer to string conversion
 * @param value Value to convert
 * @param buffer Output buffer
 * @param base Numeric base (10 for decimal)
 * @return Pointer to start of string in buffer
 */
char* itoa_simple(int value, char* buffer, int base) {
    char* ptr = buffer;
    char* ptr1 = buffer;
    char tmp_char;
    int tmp_value;

    if (base < 2 || base > 36) {
        *buffer = '\0';
        return buffer;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    } while (value);

    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return buffer;
}

int main() {
    // ========================================================================
    // Fluent Tier: Builder pattern with method chaining
    // ========================================================================

    // Build UART configuration with custom settings
    auto uart_result = UartBuilder()
                           // Pin configuration
                           .with_tx_pin<UartTxPin>()
                           .with_rx_pin<UartRxPin>()

                           // Serial configuration (9600 baud, 8E1 for error detection)
                           .baudrate(BaudRate{9600})
                           .data_bits(8)
                           .parity(UartParity::EVEN)  // Even parity for error detection
                           .stop_bits(1)

                           // Build and initialize
                           .initialize();

    // Handle initialization errors
    if (uart_result.is_err()) {
        // Initialization failed - halt
        while (true) {
            __asm__ volatile("nop");
        }
    }

    auto uart = uart_result.unwrap();

    // ========================================================================
    // Send configuration info
    // ========================================================================

    uart.write_str("MicroCore UART - Fluent Tier Example\r\n");
    uart.write_str("Configuration: 9600 baud, 8E1 (even parity)\r\n");
    uart.write_str("Sending sensor data...\r\n\r\n");

    // ========================================================================
    // Simulated sensor data transmission
    // ========================================================================

    uint32_t counter = 0;
    char buffer[64];

    while (true) {
        // Simulate sensor readings
        int temperature = 20 + (counter % 10);  // 20-29°C
        int humidity = 50 + (counter % 30);     // 50-79%
        int pressure = 1000 + (counter % 50);   // 1000-1049 hPa

        // Format message
        uart.write_str("Sample #");
        itoa_simple(counter, buffer, 10);
        uart.write_str(buffer);
        uart.write_str(": Temp=");
        itoa_simple(temperature, buffer, 10);
        uart.write_str(buffer);
        uart.write_str("C, Humidity=");
        itoa_simple(humidity, buffer, 10);
        uart.write_str(buffer);
        uart.write_str("%, Pressure=");
        itoa_simple(pressure, buffer, 10);
        uart.write_str(buffer);
        uart.write_str("hPa\r\n");

        counter++;
        delay_ms(1000);  // Send data every second
    }

    return 0;
}

/**
 * @example Fluent Tier Advantages
 *
 * **Readability**: Method chaining makes configuration clear and self-documenting
 * @code
 * auto uart = UartBuilder()
 *     .with_tx_pin<TxPin>()
 *     .with_rx_pin<RxPin>()
 *     .baudrate(BaudRate{9600})       // Lower speed for reliability
 *     .parity(UartParity::EVEN)       // Error detection
 *     .initialize();
 * @endcode
 *
 * **Flexibility**: Easy to add/remove configuration options
 * @code
 * // For debugging: Add higher speed, no parity
 * auto debug_uart = UartBuilder()
 *     .with_tx_pin<TxPin>()
 *     .with_rx_pin<RxPin>()
 *     .baudrate(BaudRate{115200})
 *     .parity(UartParity::NONE)
 *     .initialize();
 * @endcode
 *
 * **Type Safety**: Compile-time validation of configuration
 * @code
 * // This will fail at compile time if pins don't support UART
 * auto uart = UartBuilder()
 *     .with_tx_pin<InvalidPin>()  // Compile error!
 *     .initialize();
 * @endcode
 */
