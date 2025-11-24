/**
 * @file simple_uart_echo.cpp
 * @brief Simple Tier UART Example - Echo Server
 *
 * Demonstrates the Simple API tier for UART communication.
 *
 * Simple Tier Features:
 * - Factory method pattern: quick_setup()
 * - Minimal boilerplate code
 * - Arduino-like API for common tasks
 * - Best for: Prototyping, simple applications, quick demos
 *
 * Hardware Requirements:
 * - Any supported board (nucleo_f401re, nucleo_f722ze, nucleo_g071rb, etc.)
 * - USB cable for ST-LINK virtual COM port
 * - Terminal software (minicom, screen, or PuTTY)
 *
 * Expected Behavior:
 * - Echoes received characters back to terminal
 * - Sends "Hello from MicroCore UART!\n" on startup
 * - Baud rate: 115200
 * - Format: 8N1 (8 data bits, no parity, 1 stop bit)
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

// Board-specific UART pins (typically connected to ST-LINK VCP)
#if defined(MICROCORE_BOARD_NUCLEO_F401RE)
// USART2: PA2 (TX), PA3 (RX) - ST-LINK VCP
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PA2; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PA3; }
};
using UartInstance = Usart2;

#elif defined(MICROCORE_BOARD_NUCLEO_F722ZE)
// USART3: PD8 (TX), PD9 (RX) - ST-LINK VCP
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PD8; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PD9; }
};
using UartInstance = Usart3;

#elif defined(MICROCORE_BOARD_NUCLEO_G071RB) || defined(MICROCORE_BOARD_NUCLEO_G0B1RE)
// USART2: PA2 (TX), PA3 (RX) - ST-LINK VCP
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PA2; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PA3; }
};
using UartInstance = Usart2;

#elif defined(MICROCORE_BOARD_SAME70_XPLAINED)
// UART0: PB0 (TX), PB1 (RX) - Debug UART
struct UartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PB0; }
};
struct UartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PB1; }
};
using UartInstance = Uart0;

#else
#error "Unsupported board - add UART pin configuration"
#endif

/**
 * @brief Simple delay function (blocking)
 * @param ms Milliseconds to delay
 */
void delay_ms(uint32_t ms) {
    // Simple delay loop (not accurate, just for demo)
    for (uint32_t i = 0; i < ms * 1000; i++) {
        __asm__ volatile("nop");
    }
}

int main() {
    // ========================================================================
    // Simple Tier: One-liner factory method
    // ========================================================================

    // Quick setup with default 8N1 configuration at 115200 baud
    auto uart = UartInstance::quick_setup<UartTxPin, UartRxPin>(BaudRate{115200});

    // Initialize UART hardware
    auto init_result = uart.initialize();
    if (init_result.is_err()) {
        // Initialization failed - halt
        while (true) {
            __asm__ volatile("nop");
        }
    }

    // ========================================================================
    // Send startup message
    // ========================================================================

    const char* welcome_msg = "Hello from MicroCore UART!\r\n";
    const char* echo_msg = "Echo mode active. Type something...\r\n";

    uart.write_str(welcome_msg);
    uart.write_str(echo_msg);

    // ========================================================================
    // Echo loop: Read and echo back characters
    // ========================================================================

    uint8_t buffer[64];

    while (true) {
        // Read single character (non-blocking)
        auto read_result = uart.read(buffer, 1);

        if (read_result.is_ok()) {
            // Echo the character back
            uart.write(buffer, 1);

            // If newline received, add carriage return for terminal compatibility
            if (buffer[0] == '\n') {
                const char* cr = "\r";
                uart.write(reinterpret_cast<const uint8_t*>(cr), 1);
            }
        }

        // Small delay to avoid busy-waiting
        delay_ms(1);
    }

    return 0;
}

/**
 * @example Simple Tier vs Other Tiers
 *
 * **Simple Tier (this example)**:
 * @code
 * auto uart = Usart2::quick_setup<TxPin, RxPin>(BaudRate{115200});
 * uart.initialize();
 * uart.write_str("Hello!");
 * @endcode
 *
 * **Fluent Tier** (see fluent_uart_config.cpp):
 * @code
 * auto uart = Usart2Builder()
 *     .with_tx_pin<TxPin>()
 *     .with_rx_pin<RxPin>()
 *     .baudrate(BaudRate{115200})
 *     .parity(UartParity::EVEN)
 *     .initialize();
 * @endcode
 *
 * **Expert Tier** (see expert_uart_lowpower.cpp):
 * @code
 * constexpr auto config = Usart2ExpertConfig::standard_115200(
 *     PeripheralId::USART2, PinId::PA2, PinId::PA3);
 * static_assert(config.is_valid(), config.error_message());
 * expert::configure(config);
 * @endcode
 */
