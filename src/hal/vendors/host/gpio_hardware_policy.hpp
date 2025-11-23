/**
 * @file gpio_hardware_policy.hpp
 * @brief Host GPIO Hardware Policy - Mock Implementation for Testing
 *
 * This file provides a mock GPIO hardware policy that follows the same
 * CRTP pattern as embedded platforms, enabling host-based unit testing.
 *
 * Key features:
 * - Follows embedded hardware policy pattern exactly
 * - Mock register access for testing
 * - State tracking for verification
 * - Thread-safe operations
 * - Zero runtime overhead when optimized
 *
 * @note Part of MicroCore HAL Platform Abstraction Layer
 */

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>

#include "core/result.hpp"
#include "core/error_code.hpp"

namespace ucore::hal::host {

using namespace ucore::core;

/**
 * @brief Mock GPIO Register Structure
 *
 * Simulates a typical GPIO peripheral register layout.
 * Thread-safe using std::atomic for concurrent testing.
 */
struct MockGpioRegisters {
    std::atomic<uint32_t> MODER;     ///< Mode register (2 bits per pin)
    std::atomic<uint32_t> OTYPER;    ///< Output type register
    std::atomic<uint32_t> OSPEEDR;   ///< Output speed register
    std::atomic<uint32_t> PUPDR;     ///< Pull-up/pull-down register
    std::atomic<uint32_t> IDR;       ///< Input data register
    std::atomic<uint32_t> ODR;       ///< Output data register
    std::atomic<uint32_t> BSRR;      ///< Bit set/reset register (write-only)
    std::atomic<uint32_t> LCKR;      ///< Configuration lock register
    std::atomic<uint32_t> AFRL;      ///< Alternate function low register
    std::atomic<uint32_t> AFRH;      ///< Alternate function high register

    MockGpioRegisters()
        : MODER(0), OTYPER(0), OSPEEDR(0), PUPDR(0),
          IDR(0), ODR(0), BSRR(0), LCKR(0), AFRL(0), AFRH(0) {}
};

/**
 * @brief GPIO Hardware Policy for Host Platform
 *
 * Mock implementation following the same CRTP pattern as embedded platforms.
 * Enables unit testing of application logic without hardware.
 *
 * Template parameters:
 * @tparam Port GPIO port letter (0='A', 1='B', etc.)
 * @tparam Pin Pin number (0-15)
 *
 * Usage (identical to embedded):
 * @code
 * using LedPolicy = HostGpioHardwarePolicy<0, 5>;  // Port A, Pin 5
 * LedPolicy::configure_output();
 * LedPolicy::set_high();
 * @endcode
 */
template <uint8_t Port, uint8_t Pin>
class HostGpioHardwarePolicy {
public:
    static constexpr uint8_t port = Port;
    static constexpr uint8_t pin = Pin;
    static constexpr uint32_t pin_mask = (1u << Pin);

    /**
     * @brief Configure pin as output
     *
     * Sets MODER register bits for this pin to output mode (01).
     * Thread-safe operation.
     */
    static void configure_output() {
        auto& regs = get_registers();

        // Clear mode bits (2 bits per pin)
        uint32_t current = regs.MODER.load();
        current &= ~(0b11 << (Pin * 2));
        // Set to output mode (01)
        current |= (0b01 << (Pin * 2));
        regs.MODER.store(current);

        if constexpr (enable_debug_output) {
            std::lock_guard<std::mutex> lock(get_debug_mutex());
            std::cout << "[GPIO Mock] Port " << static_cast<char>('A' + Port)
                      << " Pin " << static_cast<int>(Pin)
                      << " configured as OUTPUT\n";
        }
    }

    /**
     * @brief Configure pin as input
     *
     * Sets MODER register bits for this pin to input mode (00).
     */
    static void configure_input() {
        auto& regs = get_registers();

        // Clear mode bits to set input mode (00)
        uint32_t current = regs.MODER.load();
        current &= ~(0b11 << (Pin * 2));
        regs.MODER.store(current);

        if constexpr (enable_debug_output) {
            std::lock_guard<std::mutex> lock(get_debug_mutex());
            std::cout << "[GPIO Mock] Port " << static_cast<char>('A' + Port)
                      << " Pin " << static_cast<int>(Pin)
                      << " configured as INPUT\n";
        }
    }

    /**
     * @brief Set pin HIGH
     *
     * Uses BSRR (Bit Set/Reset Register) atomic operation.
     * Lower 16 bits set pins, upper 16 bits reset pins.
     */
    static void set_high() {
        auto& regs = get_registers();

        // Write to BSRR to set pin (atomic operation)
        regs.BSRR.store(pin_mask);

        // Update ODR to reflect state
        uint32_t current = regs.ODR.load();
        regs.ODR.store(current | pin_mask);

        if constexpr (enable_debug_output) {
            std::lock_guard<std::mutex> lock(get_debug_mutex());
            std::cout << "[GPIO Mock] Port " << static_cast<char>('A' + Port)
                      << " Pin " << static_cast<int>(Pin)
                      << " set HIGH\n";
        }
    }

    /**
     * @brief Set pin LOW
     *
     * Uses BSRR (Bit Set/Reset Register) atomic operation.
     * Writing to upper 16 bits resets the corresponding pin.
     */
    static void set_low() {
        auto& regs = get_registers();

        // Write to BSRR to reset pin (bit 16+Pin)
        regs.BSRR.store(pin_mask << 16);

        // Update ODR to reflect state
        uint32_t current = regs.ODR.load();
        regs.ODR.store(current & ~pin_mask);

        if constexpr (enable_debug_output) {
            std::lock_guard<std::mutex> lock(get_debug_mutex());
            std::cout << "[GPIO Mock] Port " << static_cast<char>('A' + Port)
                      << " Pin " << static_cast<int>(Pin)
                      << " set LOW\n";
        }
    }

    /**
     * @brief Toggle pin state
     *
     * Atomic read-modify-write of ODR register.
     */
    static void toggle() {
        auto& regs = get_registers();

        uint32_t current = regs.ODR.load();
        uint32_t new_value = current ^ pin_mask;
        regs.ODR.store(new_value);

        if constexpr (enable_debug_output) {
            std::lock_guard<std::mutex> lock(get_debug_mutex());
            std::cout << "[GPIO Mock] Port " << static_cast<char>('A' + Port)
                      << " Pin " << static_cast<int>(Pin)
                      << " toggled to " << ((new_value & pin_mask) ? "HIGH" : "LOW") << "\n";
        }
    }

    /**
     * @brief Read pin state
     *
     * Reads from IDR (Input Data Register) for input pins,
     * or ODR (Output Data Register) for output pins.
     *
     * @return true if pin is HIGH, false if LOW
     */
    static bool read() {
        auto& regs = get_registers();

        // Check if configured as input or output
        uint32_t mode = regs.MODER.load();
        uint32_t pin_mode = (mode >> (Pin * 2)) & 0b11;

        uint32_t data_reg = (pin_mode == 0b00) ? regs.IDR.load() : regs.ODR.load();
        return (data_reg & pin_mask) != 0;
    }

    /**
     * @brief Configure pull-up resistor
     *
     * Sets PUPDR register to enable pull-up (01).
     */
    static void configure_pull_up() {
        auto& regs = get_registers();

        uint32_t current = regs.PUPDR.load();
        current &= ~(0b11 << (Pin * 2));
        current |= (0b01 << (Pin * 2));
        regs.PUPDR.store(current);
    }

    /**
     * @brief Configure pull-down resistor
     *
     * Sets PUPDR register to enable pull-down (10).
     */
    static void configure_pull_down() {
        auto& regs = get_registers();

        uint32_t current = regs.PUPDR.load();
        current &= ~(0b11 << (Pin * 2));
        current |= (0b10 << (Pin * 2));
        regs.PUPDR.store(current);
    }

    // ========================================================================
    // Testing Utilities
    // ========================================================================

    /**
     * @brief Get mock register values for testing
     *
     * Allows tests to verify register state without debug output.
     */
    static MockGpioRegisters& get_registers_for_testing() {
        return get_registers();
    }

    /**
     * @brief Reset all registers to initial state
     *
     * Useful for test setup/teardown.
     */
    static void reset_registers() {
        auto& regs = get_registers();
        regs.MODER.store(0);
        regs.OTYPER.store(0);
        regs.OSPEEDR.store(0);
        regs.PUPDR.store(0);
        regs.IDR.store(0);
        regs.ODR.store(0);
        regs.BSRR.store(0);
        regs.LCKR.store(0);
        regs.AFRL.store(0);
        regs.AFRH.store(0);
    }

    /**
     * @brief Enable/disable debug output globally
     *
     * Allows silencing output during automated tests.
     */
    static void set_debug_output(bool enabled) {
        enable_debug_output = enabled;
    }

private:
    /**
     * @brief Get mock register bank for this port
     *
     * Static storage, one register bank per port (A, B, C, etc.)
     */
    static MockGpioRegisters& get_registers() {
        static std::array<MockGpioRegisters, 16> port_registers;
        return port_registers[Port];
    }

    /**
     * @brief Get debug output mutex
     *
     * Ensures thread-safe console output.
     */
    static std::mutex& get_debug_mutex() {
        static std::mutex debug_mutex;
        return debug_mutex;
    }

    static inline bool enable_debug_output = true;  ///< Global debug flag
};

} // namespace ucore::hal::host
