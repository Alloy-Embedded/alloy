/**
 * @file board.hpp
 * @brief Linux Host Board Configuration (Template-Based HAL)
 *
 * Modern C++20 board support using template-based HAL for Linux host platform.
 * This file provides board-specific peripheral mappings for development and
 * testing on Linux systems.
 *
 * Board: Linux Host (PC, Raspberry Pi, BeagleBone, etc.)
 * Platform: x86_64, ARM, etc.
 *
 * Features:
 *   - UART via /dev/tty* devices
 *   - GPIO via sysfs (optional)
 *   - Compatible API with embedded boards
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 * @see openspec/changes/platform-abstraction/
 */

#ifndef ALLOY_BOARD_LINUX_HOST_HPP
#define ALLOY_BOARD_LINUX_HOST_HPP

#include <stdint.h>
#include <chrono>
#include <thread>
#include "hal/platform/linux/uart.hpp"

namespace board {

// Import platform HAL
using namespace alloy::hal::linux;
using namespace alloy::core;

// Import common types
using alloy::hal::Baudrate;

// ==============================================================================
// Board Information
// ==============================================================================

inline constexpr const char* name = "Linux Host";
inline constexpr const char* platform = "Linux";
inline constexpr const char* architecture = "x86_64/ARM";

// ==============================================================================
// Peripheral Mappings (Type Aliases)
// ==============================================================================

// UARTs - map to common Linux serial devices
using uart_debug = UartUsb0;    // /dev/ttyUSB0 - Primary USB-to-serial adapter
using uart_ext1 = UartUsb1;     // /dev/ttyUSB1 - Secondary USB adapter
using uart_console = UartAcm0;  // /dev/ttyACM0 - Arduino/USB CDC devices
using uart_serial0 = UartS0;    // /dev/ttyS0 - Hardware serial port 0
using uart_serial1 = UartS1;    // /dev/ttyS1 - Hardware serial port 1

// Note: GPIO support can be added via sysfs or libgpiod in the future
// For now, this is a UART-focused board configuration

// ==============================================================================
// Board Initialization
// ==============================================================================

/**
 * @brief Initialize board peripherals
 *
 * On Linux, this is mostly a no-op since the OS manages hardware.
 * This function exists for API compatibility with embedded boards.
 *
 * @return Result<void> Always returns Ok()
 */
inline Result<void> initialize() {
    // Linux manages hardware initialization
    // No explicit setup needed
    return Result<void>::ok();
}

// ==============================================================================
// Delay Functions (Using std::this_thread::sleep_for)
// ==============================================================================

/**
 * @brief Delay for specified milliseconds
 *
 * Uses std::this_thread::sleep_for for accurate, OS-managed delays.
 * Unlike embedded platforms, this yields the CPU to other tasks.
 *
 * @param ms Milliseconds to delay
 */
inline void delay_ms(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/**
 * @brief Delay for specified microseconds
 *
 * Uses std::this_thread::sleep_for for accurate, OS-managed delays.
 *
 * @param us Microseconds to delay
 */
inline void delay_us(uint32_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

// ==============================================================================
// UART Helper Functions
// ==============================================================================

namespace uart {
    // Note: UART objects cannot be returned by value since they manage file descriptors
    // Users should create UART instances directly: auto uart = board::uart_debug{};

    /**
     * @brief Open and configure a UART device
     *
     * Opens the specified UART device with the given baudrate.
     * This is a helper to reduce boilerplate.
     *
     * @param uart UART instance to configure
     * @param baudrate Desired baudrate
     * @return Result<void> Ok() if successful
     */
    template<const char* DEVICE_PATH>
    inline Result<void> configure(Uart<DEVICE_PATH>& uart, Baudrate baudrate = Baudrate::e115200) {
        auto result = uart.open();
        if (result.is_error()) {
            return result;
        }

        result = uart.setBaudrate(baudrate);
        if (result.is_error()) {
            uart.close();
            return result;
        }

        return Result<void>::ok();
    }

    /**
     * @brief Print string to UART
     *
     * Convenience function to print C-string to UART.
     *
     * @param uart UART instance
     * @param str Null-terminated string
     * @return Result<size_t> Bytes written or error
     */
    template<const char* DEVICE_PATH>
    inline Result<size_t> print(Uart<DEVICE_PATH>& uart, const char* str) {
        size_t len = 0;
        while (str[len] != '\0') {
            len++;
        }
        return uart.write(reinterpret_cast<const uint8_t*>(str), len);
    }

    /**
     * @brief Print string with newline to UART
     *
     * @param uart UART instance
     * @param str Null-terminated string
     * @return Result<size_t> Bytes written or error
     */
    template<const char* DEVICE_PATH>
    inline Result<size_t> println(Uart<DEVICE_PATH>& uart, const char* str) {
        auto result = print(uart, str);
        if (result.is_error()) {
            return result;
        }

        size_t total = result.value();

        auto newline_result = uart.write(reinterpret_cast<const uint8_t*>("\n"), 1);
        if (newline_result.is_error()) {
            return newline_result;
        }

        return Result<size_t>::ok(total + newline_result.value());
    }
}

// ==============================================================================
// Board Information Functions
// ==============================================================================

namespace info {
    inline constexpr const char* get_name() { return name; }
    inline constexpr const char* get_platform() { return platform; }
    inline constexpr const char* get_architecture() { return architecture; }

    /**
     * @brief Get system uptime in milliseconds
     *
     * @return Milliseconds since system boot (approximation)
     */
    inline uint64_t get_uptime_ms() {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
}

// ==============================================================================
// Linux-Specific Utilities
// ==============================================================================

namespace linux {
    /**
     * @brief List available serial devices
     *
     * Returns a list of common serial device paths.
     * Note: This doesn't check if devices actually exist.
     *
     * @return Array of device path strings
     */
    inline constexpr const char* const serial_devices[] = {
        "/dev/ttyUSB0",
        "/dev/ttyUSB1",
        "/dev/ttyUSB2",
        "/dev/ttyUSB3",
        "/dev/ttyACM0",
        "/dev/ttyACM1",
        "/dev/ttyS0",
        "/dev/ttyS1",
        nullptr  // Null-terminated
    };

    /**
     * @brief Get serial device list
     *
     * @return Pointer to null-terminated array of device paths
     */
    inline constexpr const char* const* get_serial_devices() {
        return serial_devices;
    }
}

} // namespace board

#endif // ALLOY_BOARD_LINUX_HOST_HPP
