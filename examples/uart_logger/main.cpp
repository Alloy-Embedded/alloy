/**
 * @file main.cpp
 * @brief UART Logger Example - Demonstrates Proper HAL Usage
 *
 * This example demonstrates the **correct way** to use MicroCore HAL abstractions
 * for UART communication, avoiding raw register access.
 *
 * ## What This Example Shows
 *
 * - ✅ Board initialization using board abstraction layer
 * - ✅ UART configuration using SimpleUartConfigTxOnly HAL API
 * - ✅ Proper error handling with Result<T> pattern
 * - ✅ Portable code that works across different boards
 * - ✅ Clear separation between hardware and application logic
 *
 * ## Key Principles Demonstrated
 *
 * 1. **Use HAL Abstractions**: Never access registers directly
 * 2. **Handle Errors**: Check Result<T> returns
 * 3. **Board Abstraction**: Let board layer handle platform specifics
 * 4. **Portability**: Same code runs on different boards
 *
 * ## Hardware Requirements
 *
 * - **Boards Supported:**
 *   - SAME70 Xplained Ultra (UART0 via EDBG virtual COM)
 *   - STM32 Nucleo boards (USART2 via ST-Link)
 *   - Any board with debug UART configured
 * - **Baud Rate:** 115200, 8N1
 * - **Connection:** USB cable (debug connector)
 *
 * ## Expected Behavior
 *
 * 1. LED blinks 3 times (board init OK)
 * 2. Sends "Hello, MicroCore!" message once
 * 3. Sends "Counter: N" messages every second
 * 4. LED toggles with each message
 *
 * ## Viewing Output
 *
 * ```bash
 * # macOS
 * screen /dev/tty.usbmodem* 115200
 *
 * # Linux
 * screen /dev/ttyACM0 115200
 * ```
 *
 * ## Anti-Patterns Avoided
 *
 * ❌ **Don't do this:**
 * ```cpp
 * volatile uint32_t* UART0_THR = (volatile uint32_t*)0x400E081C;
 * *UART0_THR = 'H';  // Raw register access!
 * ```
 *
 * ✅ **Do this instead:**
 * ```cpp
 * uart.write_byte('H');  // Use HAL abstraction
 * ```
 *
 * @note This example serves as a **teaching tool** for proper HAL usage.
 *       Study this code to learn MicroCore best practices!
 */

#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/api/uart_simple.hpp"

using namespace ucore::hal;

int main() {
    // ============================================================================
    // Step 1: Initialize Board Hardware
    // ============================================================================
    /**
     * Board initialization handles:
     * - Clock configuration (PLL, peripheral clocks)
     * - SysTick timer setup
     * - GPIO configuration for LEDs/buttons
     * - Platform-specific initialization
     *
     * ALWAYS use board::init() instead of manual register configuration!
     */
    board::init();

    // Visual confirmation: blink LED 3 times
    for (int i = 0; i < 3; i++) {
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(200);
        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(200);
    }

    // ============================================================================
    // Step 2: Configure UART for TX-Only Communication
    // ============================================================================
    /**
     * SimpleUartConfigTxOnly provides a minimal UART interface for logging:
     * - Transmit-only (no RX overhead)
     * - Board-specific pin and peripheral configuration
     * - Result<T> error handling
     *
     * The board:: namespace provides the correct UART configuration for each board.
     * You don't need to know register addresses or pin mappings!
     */

    // Create UART configuration (board-specific, defined in board.hpp)
    using UartConfig = SimpleUartConfigTxOnly<
        board::uart::TxPin,
        board::uart::Policy
    >;

    UartConfig uart{
        board::uart::peripheral_id,
        BaudRate{115200},
        8,                     // data bits
        UartParity::NONE,
        1                      // stop bits
    };

    // ============================================================================
    // Step 3: Initialize UART with Error Handling
    // ============================================================================
    /**
     * HAL initialization can fail (e.g., invalid baud rate, hardware error).
     * ALWAYS check Result<T> return values!
     *
     * Result<T> pattern:
     * - is_ok()  → Check if operation succeeded
     * - error()  → Get error code on failure
     * - value()  → Get result value on success
     */

    auto uart_result = uart.initialize();

    if (!uart_result.is_ok()) {
        // UART initialization failed - indicate error visually
        // Blink LED rapidly forever to signal error state
        while (true) {
            board::led::toggle();
            SysTickTimer::delay_ms<board::BoardSysTick>(100);
        }
    }

    // Initialization succeeded - visual confirmation
    board::led::on();
    SysTickTimer::delay_ms<board::BoardSysTick>(500);
    board::led::off();
    SysTickTimer::delay_ms<board::BoardSysTick>(500);

    // ============================================================================
    // Step 4: Send Startup Message
    // ============================================================================
    /**
     * Helper function to send strings via UART.
     * Uses HAL write_byte() abstraction, not raw register access.
     */
    auto send_string = [&uart](const char* str) {
        while (*str) {
            uart.write_byte(*str++);
        }
    };

    send_string("Hello, MicroCore!\r\n");
    send_string("UART Logger Example - Demonstrating Proper HAL Usage\r\n");
    send_string("=====================================\r\n\r\n");

    // ============================================================================
    // Step 5: Main Loop - Send Periodic Messages
    // ============================================================================
    /**
     * Simple counter demonstration showing:
     * - String output via HAL
     * - Number formatting
     * - Periodic timing with SysTick
     * - Visual feedback with LED
     */

    uint32_t counter = 0;

    while (true) {
        // Send counter message
        send_string("Counter: ");

        // Convert counter to string (simple decimal conversion)
        char num_buffer[12];
        uint32_t n = counter;
        int i = 0;

        if (n == 0) {
            num_buffer[i++] = '0';
        } else {
            // Extract digits in reverse order
            char temp[12];
            int j = 0;
            while (n > 0) {
                temp[j++] = '0' + (n % 10);
                n /= 10;
            }
            // Reverse to get correct order
            for (int k = j - 1; k >= 0; k--) {
                num_buffer[i++] = temp[k];
            }
        }
        num_buffer[i] = '\0';

        send_string(num_buffer);
        send_string("\r\n");

        // Visual feedback: toggle LED
        board::led::toggle();

        // Wait 1 second before next message
        SysTickTimer::delay_ms<board::BoardSysTick>(1000);

        counter++;
    }

    return 0;
}
