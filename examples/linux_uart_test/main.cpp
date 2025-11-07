/**
 * @file main.cpp
 * @brief Linux UART Example - Serial Communication Test
 *
 * This example demonstrates the Linux UART implementation using POSIX termios.
 * It shows how to:
 * 1. Open a serial port
 * 2. Configure baudrate
 * 3. Write data
 * 4. Read data
 * 5. Handle errors
 *
 * Hardware: Any Linux system with a serial port
 * Device: /dev/ttyUSB0 (USB-to-serial adapter)
 *
 * To test with a loopback:
 * - Connect TX to RX on your serial adapter
 * - Or use a virtual serial port (socat)
 */

#include "hal/platform/linux/uart.hpp"
#include <iostream>
#include <cstring>

using namespace alloy::hal::linux;
using namespace alloy::core;

int main() {
    std::cout << "=== Linux UART Example ===\n\n";

    // Create UART instance for /dev/ttyUSB0
    auto uart = UartUsb0{};

    // Open the device
    std::cout << "Opening /dev/ttyUSB0...\n";
    auto result = uart.open();
    if (result.is_error()) {
        std::cerr << "ERROR: Failed to open UART. ";
        std::cerr << "Make sure /dev/ttyUSB0 exists and you have permissions.\n";
        std::cerr << "Error code: " << static_cast<int>(result.error()) << "\n";
        std::cerr << "\nTips:\n";
        std::cerr << "  - Check device exists: ls -l /dev/ttyUSB*\n";
        std::cerr << "  - Add user to dialout group: sudo usermod -a -G dialout $USER\n";
        std::cerr << "  - Create virtual serial ports: socat -d -d pty,raw,echo=0 pty,raw,echo=0\n";
        return 1;
    }
    std::cout << "✓ UART opened successfully\n\n";

    // Set baudrate
    std::cout << "Setting baudrate to 115200...\n";
    result = uart.setBaudrate(Baudrate::e115200);
    if (result.is_error()) {
        std::cerr << "ERROR: Failed to set baudrate\n";
        uart.close();
        return 1;
    }
    std::cout << "✓ Baudrate set to 115200\n\n";

    // Write data
    const char* message = "Hello, UART!\n";
    std::cout << "Writing: \"" << message << "\"\n";

    auto write_result = uart.write(
        reinterpret_cast<const uint8_t*>(message),
        std::strlen(message)
    );

    if (write_result.is_error()) {
        std::cerr << "ERROR: Failed to write data\n";
        uart.close();
        return 1;
    }

    std::cout << "✓ Wrote " << write_result.value() << " bytes\n\n";

    // Read data (if available - requires loopback)
    std::cout << "Checking for received data...\n";

    uint8_t buffer[128];
    auto read_result = uart.read(buffer, sizeof(buffer));

    if (read_result.is_ok() && read_result.value() > 0) {
        std::cout << "✓ Read " << read_result.value() << " bytes: ";
        std::cout.write(reinterpret_cast<char*>(buffer), read_result.value());
        std::cout << "\n";
    } else {
        std::cout << "  No data available (this is OK if no loopback)\n";
    }

    // Test available() method
    auto avail_result = uart.available();
    if (avail_result.is_ok()) {
        std::cout << "\nBytes available in buffer: " << avail_result.value() << "\n";
    }

    // Close UART
    std::cout << "\nClosing UART...\n";
    result = uart.close();
    if (result.is_error()) {
        std::cerr << "ERROR: Failed to close UART\n";
        return 1;
    }
    std::cout << "✓ UART closed successfully\n\n";

    std::cout << "=== Test Complete ===\n";
    return 0;
}
