/**
 * @file main.cpp
 * @brief Linux Board Configuration Example
 *
 * This example demonstrates the Linux board abstraction layer that provides
 * application-friendly names and convenience functions.
 *
 * Features demonstrated:
 * 1. Board initialization
 * 2. Using board:: namespace for peripheral access
 * 3. UART convenience functions (print/println)
 * 4. Board information queries
 * 5. Platform-independent delays
 *
 * Hardware: Any Linux system
 * Device: /dev/ttyUSB0 (USB-to-serial adapter)
 */

#include "../../boards/linux_host/board.hpp"
#include <iostream>

int main() {
    std::cout << "=== Linux Board Configuration Example ===\n\n";

    // ========================================================================
    // Board Information
    // ========================================================================

    std::cout << "Board Information:\n";
    std::cout << "  Name:         " << board::info::get_name() << "\n";
    std::cout << "  Platform:     " << board::info::get_platform() << "\n";
    std::cout << "  Architecture: " << board::info::get_architecture() << "\n";
    std::cout << "  Uptime:       " << board::info::get_uptime_ms() << " ms\n";
    std::cout << "\n";

    // ========================================================================
    // Board Initialization
    // ========================================================================

    std::cout << "Initializing board...\n";
    auto result = board::initialize();
    if (result.is_error()) {
        std::cerr << "ERROR: Failed to initialize board\n";
        return 1;
    }
    std::cout << "✓ Board initialized successfully\n\n";

    // ========================================================================
    // Available Serial Devices
    // ========================================================================

    std::cout << "Available serial device paths:\n";
    auto devices = board::linux::get_serial_devices();
    for (int i = 0; devices[i] != nullptr; i++) {
        std::cout << "  - " << devices[i] << "\n";
    }
    std::cout << "\n";

    // ========================================================================
    // Pattern 1: Using board type aliases and configure helper
    // ========================================================================

    std::cout << "Pattern 1: Using board type aliases with configure helper\n";
    std::cout << "Opening board::uart_debug (/dev/ttyUSB0)...\n";

    auto uart = board::uart_debug{};
    result = board::uart::configure(uart, board::Baudrate::e115200);

    if (result.is_error()) {
        std::cout << "  ⚠ No UART device available (this is OK for demo)\n";
        std::cout << "  Error code: " << static_cast<int>(result.error()) << "\n\n";

        // Show delay functions work even without UART
        std::cout << "Testing delay functions...\n";
        auto start = board::info::get_uptime_ms();
        board::delay_ms(100);
        auto end = board::info::get_uptime_ms();
        std::cout << "✓ Delayed " << (end - start) << " ms\n\n";

        std::cout << "=== Demo Complete (no hardware) ===\n";
        return 0;
    }

    std::cout << "✓ UART opened and configured successfully\n\n";

    // ========================================================================
    // Pattern 2: Using board::uart helpers
    // ========================================================================

    std::cout << "Pattern 2: Using board::uart helpers\n";

    // Print without newline
    auto write_result = board::uart::print(uart, "Hello from board::uart::print()");
    if (write_result.is_ok()) {
        std::cout << "✓ Printed " << write_result.value() << " bytes\n";
    }

    // Print with newline
    write_result = board::uart::println(uart, "Hello from board::uart::println()");
    if (write_result.is_ok()) {
        std::cout << "✓ Printed " << write_result.value() << " bytes (with newline)\n";
    }
    std::cout << "\n";

    // ========================================================================
    // Pattern 3: Direct UART access
    // ========================================================================

    std::cout << "Pattern 3: Direct UART access\n";

    const char* message = "Direct write test\n";
    write_result = uart.write(
        reinterpret_cast<const uint8_t*>(message),
        __builtin_strlen(message)
    );

    if (write_result.is_ok()) {
        std::cout << "✓ Wrote " << write_result.value() << " bytes directly\n";
    }
    std::cout << "\n";

    // ========================================================================
    // Testing delay functions
    // ========================================================================

    std::cout << "Testing delay functions...\n";

    // Millisecond delay
    auto start = board::info::get_uptime_ms();
    board::delay_ms(50);
    auto end = board::info::get_uptime_ms();
    std::cout << "  delay_ms(50): actual delay = " << (end - start) << " ms\n";

    // Microsecond delay (measure in ms)
    start = board::info::get_uptime_ms();
    board::delay_us(10000);  // 10ms in microseconds
    end = board::info::get_uptime_ms();
    std::cout << "  delay_us(10000): actual delay = " << (end - start) << " ms\n";
    std::cout << "\n";

    // ========================================================================
    // Cleanup
    // ========================================================================

    std::cout << "Closing UART...\n";
    result = uart.close();
    if (result.is_error()) {
        std::cerr << "ERROR: Failed to close UART\n";
        return 1;
    }
    std::cout << "✓ UART closed successfully\n\n";

    std::cout << "=== Example Complete ===\n";
    return 0;
}
