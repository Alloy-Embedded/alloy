/**
 * @file main.cpp
 * @brief Simple Linux platform test using CMake
 */

#include "hal/platform/linux/uart.hpp"
#include <iostream>

int main() {
    std::cout << "Linux Platform CMake Test\n";
    std::cout << "==========================\n\n";

    // Verify POSIX defines are set
#ifdef _POSIX_C_SOURCE
    std::cout << "✓ _POSIX_C_SOURCE defined: " << _POSIX_C_SOURCE << "\n";
#else
    std::cout << "✗ _POSIX_C_SOURCE not defined\n";
#endif

#ifdef _DEFAULT_SOURCE
    std::cout << "✓ _DEFAULT_SOURCE defined\n";
#else
    std::cout << "✗ _DEFAULT_SOURCE not defined\n";
#endif

    // Test UART type
    std::cout << "\nUART Type Test:\n";
    std::cout << "  UartUsb0 device: " << alloy::hal::linux::UartUsb0::device_path << "\n";

    std::cout << "\n✓ Linux platform configured correctly!\n";
    return 0;
}
