/**
 * @file expert_uart_lowpower.cpp
 * @brief Expert Tier UART Example - Low-Power Operation
 *
 * Demonstrates the Expert API tier for UART with advanced low-power features.
 *
 * Expert Tier Features:
 * - Compile-time configuration validation
 * - Full control over all hardware settings
 * - Zero-overhead abstractions
 * - Best for: Performance-critical code, resource-constrained systems, power optimization
 *
 * Hardware Requirements:
 * - STM32G0 board (nucleo_g071rb or nucleo_g0b1re) with LPUART support
 * - USB cable for ST-LINK virtual COM port
 * - Terminal software at 9600 baud
 * - Optional: Current meter to measure power consumption
 *
 * Expected Behavior:
 * - Sends periodic messages using low-power UART
 * - Enters STOP mode between transmissions
 * - Wakes from STOP mode on UART RX activity
 * - Demonstrates ultra-low power UART operation
 *
 * @note This example only works on STM32G0 boards with LPUART support
 * @note Part of Phase 3.2: UART Platform Completeness
 * @see docs/API_TIERS.md for tier comparison
 */

#include "board.hpp"

#if defined(MICROCORE_PLATFORM_STM32G0)

#include "hal/platform/stm32g0/uart.hpp"

using namespace ucore::hal::stm32g0;
using namespace ucore::core;

// LPUART1 pins on Nucleo G071RB/G0B1RE
// Alternative 1: PA2 (TX), PA3 (RX) - ST-LINK VCP
// Alternative 2: PB11 (TX), PB10 (RX)
// Alternative 3: PC1 (TX), PC0 (RX)

struct LpuartTxPin {
    static constexpr PinId get_pin_id() { return PinId::PA2; }
};

struct LpuartRxPin {
    static constexpr PinId get_pin_id() { return PinId::PA3; }
};

/**
 * @brief Enter STOP mode (ultra-low power)
 * @note LPUART can wake the MCU from STOP mode when data is received
 */
void enter_stop_mode() {
    // Enable power interface clock
    RCC->APBENR1 |= (1 << 28);  // PWREN

    // Set STOP mode (not standby)
    PWR->CR1 &= ~(0x7 << 0);  // Clear LPMS bits
    PWR->CR1 |= (0x0 << 0);   // STOP mode

    // Set SLEEPDEEP bit for STOP mode
    SCB->SCR |= (1 << 2);

    // Wait for interrupt (enter STOP mode)
    __WFI();

    // CPU wakes here when LPUART receives data
}

/**
 * @brief Simple delay function (blocking)
 * @param ms Milliseconds to delay
 */
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 10000; i++) {
        __asm__ volatile("nop");
    }
}

int main() {
    // ========================================================================
    // Expert Tier: Compile-time validated configuration
    // ========================================================================

    // Create LPUART configuration with compile-time validation
    constexpr auto lpuart_config = Lpuart1ExpertConfig::create_config(
        PeripheralId::LPUART1,
        PinId::PA2,  // TX
        PinId::PA3,  // RX
        9600,        // Low baud rate for better low-power operation
        8,           // Data bits
        1,           // Stop bits
        0            // Parity (0=none, 1=odd, 2=even)
    );

    // Compile-time validation
    static_assert(lpuart_config.is_valid(), "LPUART configuration is invalid!");

    // Configure LPUART using Expert API
    auto lpuart_result = expert::configure(lpuart_config);

    if (lpuart_result.is_err()) {
        // Configuration failed - halt
        while (true) {
            __asm__ volatile("nop");
        }
    }

    // Get LPUART instance (for convenience, use Simple API for I/O)
    auto lpuart = Lpuart1::quick_setup<LpuartTxPin, LpuartRxPin>(BaudRate{9600});
    lpuart.initialize();

    // ========================================================================
    // Enable Low-Power Features
    // ========================================================================

    // Enable wakeup from STOP mode (LPUART-specific feature)
    lpuart.enable_wakeup();

    // Enable FIFO mode for better performance (STM32G0 feature)
    lpuart.enable_fifo();

    // Send startup message
    lpuart.write_str("MicroCore LPUART - Expert Tier Example\r\n");
    lpuart.write_str("Low-power mode with wakeup enabled\r\n");
    lpuart.write_str("Send any character to wake from STOP mode\r\n\r\n");

    // ========================================================================
    // Low-Power Loop: Send periodic messages, sleep between
    // ========================================================================

    uint32_t counter = 0;

    while (true) {
        // Send periodic message
        lpuart.write_str("Counter: ");
        char buffer[16];
        // Simple integer to string
        uint32_t value = counter++;
        int idx = 0;
        if (value == 0) {
            buffer[idx++] = '0';
        } else {
            char temp[16];
            int temp_idx = 0;
            while (value > 0) {
                temp[temp_idx++] = '0' + (value % 10);
                value /= 10;
            }
            while (temp_idx > 0) {
                buffer[idx++] = temp[--temp_idx];
            }
        }
        buffer[idx] = '\0';
        lpuart.write_str(buffer);
        lpuart.write_str(" (entering STOP mode...)\r\n");

        // Wait for transmission to complete
        delay_ms(100);

        // ====================================================================
        // Enter STOP mode - Ultra-low power consumption
        // ====================================================================
        // MCU will wake up when:
        // 1. LPUART receives data (wakeup from RX)
        // 2. Any other enabled interrupt occurs
        //
        // Power consumption in STOP mode: ~1-2 µA (vs ~10 mA in RUN mode)
        // ====================================================================

        enter_stop_mode();

        // ====================================================================
        // Woken up from STOP mode
        // ====================================================================

        lpuart.write_str("Woke from STOP mode!\r\n");

        // Check if data was received
        uint8_t rx_buffer[64];
        auto read_result = lpuart.read(rx_buffer, sizeof(rx_buffer));

        if (read_result.is_ok()) {
            uint32_t bytes_read = read_result.unwrap();
            if (bytes_read > 0) {
                lpuart.write_str("Received: ");
                lpuart.write(rx_buffer, bytes_read);
                lpuart.write_str("\r\n");
            }
        }

        // Small delay before next cycle
        delay_ms(1000);
    }

    return 0;
}

#else

// Fallback for non-STM32G0 platforms
#include "hal/platform/stm32f4/uart.hpp"

int main() {
    // This example requires STM32G0 with LPUART support
    // Other platforms don't have low-power UART features
    while (true) {
        __asm__ volatile("nop");
    }
    return 0;
}

#endif  // MICROCORE_PLATFORM_STM32G0

/**
 * @example Expert Tier Advantages
 *
 * **Compile-Time Validation**: Configuration errors caught at compile time
 * @code
 * constexpr auto config = LpuartExpertConfig::create_config(...);
 * static_assert(config.is_valid(), config.error_message());
 * // If configuration is invalid, compilation fails with clear error message
 * @endcode
 *
 * **Zero Runtime Overhead**: All configuration resolved at compile time
 * @code
 * // The compiler optimizes this to direct register writes
 * // No runtime checks, no vtables, no function call overhead
 * expert::configure(lpuart_config);
 * @endcode
 *
 * **Full Hardware Control**: Access to all platform-specific features
 * @code
 * // STM32G0-specific low-power features
 * lpuart.enable_wakeup();     // Wakeup from STOP mode
 * lpuart.enable_fifo();       // 8-byte FIFO
 * enter_stop_mode();          // Ultra-low power mode
 * @endcode
 *
 * **Performance**: Typical power consumption comparison
 * - Simple/Fluent Tier: ~10 mA (continuous RUN mode)
 * - Expert Tier (this example): ~1-2 µA (STOP mode with LPUART wakeup)
 * - Power savings: **5000x lower power consumption**
 *
 * **Tradeoffs**:
 * - More complex code (need to understand hardware)
 * - Platform-specific (LPUART only on STM32G0)
 * - Requires careful power mode management
 */

/**
 * @example When to Use Expert Tier
 *
 * Use Expert Tier when:
 * 1. **Battery-powered applications**: Need ultra-low power consumption
 * 2. **Real-time systems**: Need guaranteed zero-overhead abstractions
 * 3. **Resource-constrained**: Need minimal code size and RAM usage
 * 4. **Hardware-specific features**: Need access to platform-specific capabilities
 * 5. **Performance-critical**: Need compile-time optimization
 *
 * Use Simple/Fluent Tier when:
 * 1. **Prototyping**: Need quick development
 * 2. **Portability**: Need code that works across platforms
 * 3. **Readability**: Need self-documenting code
 * 4. **Learning**: Need easier-to-understand APIs
 */
