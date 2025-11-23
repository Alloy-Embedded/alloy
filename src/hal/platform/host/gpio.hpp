/**
 * @file gpio.hpp
 * @brief Host/Mock GPIO implementation for testing
 *
 * This file provides a mock GPIO implementation that runs on the host (x86/ARM macOS/Linux).
 * It emulates hardware behavior using in-memory registers, allowing for unit testing
 * without physical hardware.
 *
 * Design Principles:
 * - Same API as embedded GPIO (pin-compatible)
 * - Mock registers stored in memory
 * - Zero-cost in release builds (can be optimized out)
 * - Inspectable state for testing
 * - Thread-safe with optional atomics
 *
 * Usage in Tests:
 * @code
 * using namespace ucore::hal::host;
 *
 * // Define a mock LED pin
 * using MockLed = GpioPin<0, 5>;  // Port 0, Pin 5
 * auto led = MockLed{};
 *
 * led.setDirection(PinDirection::Output);
 * led.set();
 *
 * // Verify state in tests
 * assert(led.read().value() == true);
 * @endcode
 *
 * @note Part of MicroCore HAL Platform Abstraction Layer
 * @note For host-based testing only - not for actual embedded targets
 */

#pragma once

#include "hal/types.hpp"

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

#include <array>
#include <atomic>
#include <cstdint>

namespace ucore::hal::host {

using namespace ucore::core;
using namespace ucore::hal;

// ============================================================================
// Mock Register Storage
// ============================================================================

/**
 * @brief Mock GPIO port registers (simulates hardware)
 *
 * Each port has registers similar to STM32 GPIO:
 * - MODER: Pin mode (input/output/alternate/analog)
 * - OTYPER: Output type (push-pull/open-drain)
 * - OSPEEDR: Output speed
 * - PUPDR: Pull-up/pull-down resistors
 * - IDR: Input data register
 * - ODR: Output data register
 * - BSRR: Bit set/reset register
 */
struct MockGpioRegisters {
    std::atomic<uint32_t> MODER{0};     ///< Mode register (2 bits per pin)
    std::atomic<uint32_t> OTYPER{0};    ///< Output type register
    std::atomic<uint32_t> OSPEEDR{0};   ///< Output speed register
    std::atomic<uint32_t> PUPDR{0};     ///< Pull-up/pull-down register
    std::atomic<uint32_t> IDR{0};       ///< Input data register
    std::atomic<uint32_t> ODR{0};       ///< Output data register
    std::atomic<uint32_t> BSRR{0};      ///< Bit set/reset register (write-only)
};

/**
 * @brief Global mock register storage
 *
 * Simulates 8 GPIO ports (A-H) with 32 pins each
 */
inline std::array<MockGpioRegisters, 8> g_mock_gpio_ports;

/**
 * @brief Reset all mock GPIO ports to default state
 *
 * Useful for test setup/teardown
 */
inline void reset_mock_gpio() {
    for (auto& port : g_mock_gpio_ports) {
        port.MODER.store(0);
        port.OTYPER.store(0);
        port.OSPEEDR.store(0);
        port.PUPDR.store(0);
        port.IDR.store(0);
        port.ODR.store(0);
        port.BSRR.store(0);
    }
}

// ============================================================================
// Host GPIO Pin Implementation
// ============================================================================

/**
 * @brief Mock GPIO pin for host-based testing
 *
 * Provides the same interface as embedded GPIO but uses mock registers
 * stored in memory instead of hardware registers.
 *
 * @tparam PORT_NUM Port number (0-7, corresponding to A-H)
 * @tparam PIN_NUM Pin number (0-31)
 */
template <uint32_t PORT_NUM, uint8_t PIN_NUM>
class GpioPin {
   public:
    // Compile-time constants
    static constexpr uint32_t port_number = PORT_NUM;
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);

    // Validate at compile-time
    static_assert(PORT_NUM < 8, "Port number must be 0-7 (A-H)");
    static_assert(PIN_NUM < 32, "Pin number must be 0-31");

    /**
     * @brief Get mock port registers
     */
    static inline MockGpioRegisters* get_port() {
        return &g_mock_gpio_ports[PORT_NUM];
    }

    /**
     * @brief Set pin HIGH (output = 1)
     *
     * Simulates atomic BSRR write like real hardware.
     *
     * @return Result<void, ErrorCode> Ok() on success
     */
    Result<void, ErrorCode> set() {
        auto* port = get_port();

        // Simulate BSRR: Setting bit in lower 16 bits sets the pin
        port->BSRR.store(pin_mask);
        port->ODR.fetch_or(pin_mask);

        return Ok();
    }

    /**
     * @brief Set pin LOW (output = 0)
     *
     * @return Result<void, ErrorCode> Ok() on success
     */
    Result<void, ErrorCode> clear() {
        auto* port = get_port();

        // Simulate BSRR: Setting bit in upper 16 bits clears the pin
        port->BSRR.store(pin_mask << 16);
        port->ODR.fetch_and(~pin_mask);

        return Ok();
    }

    /**
     * @brief Toggle pin state
     *
     * @return Result<void, ErrorCode> Ok() on success
     */
    Result<void, ErrorCode> toggle() {
        auto* port = get_port();

        uint32_t current = port->ODR.load();
        if (current & pin_mask) {
            return clear();
        } else {
            return set();
        }
    }

    /**
     * @brief Write pin value
     *
     * @param value true for HIGH, false for LOW
     * @return Result<void, ErrorCode> Ok() on success
     */
    Result<void, ErrorCode> write(bool value) {
        return value ? set() : clear();
    }

    /**
     * @brief Read pin input value
     *
     * For host platform, reads from ODR (output state) by default.
     * In real testing, you can inject input values into IDR.
     *
     * @return Result<bool, ErrorCode> Pin state
     */
    Result<bool, ErrorCode> read() const {
        auto* port = get_port();

        // Read from ODR for now (simulates reading output pin)
        // In tests, you can manually set IDR to simulate external signals
        uint32_t value = port->ODR.load();

        return Ok(bool((value & pin_mask) != 0));
    }

    /**
     * @brief Set GPIO pin direction
     *
     * @param direction Input or Output
     * @return Result<void, ErrorCode> Ok() on success
     */
    Result<void, ErrorCode> setDirection(PinDirection direction) {
        auto* port = get_port();

        uint32_t moder = port->MODER.load();
        moder &= ~(0x3u << (PIN_NUM * 2));  // Clear 2 bits
        moder |= (direction == PinDirection::Input ? 0x0u : 0x1u) << (PIN_NUM * 2);
        port->MODER.store(moder);

        return Ok();
    }

    /**
     * @brief Set GPIO pin drive mode
     *
     * @param drive PushPull or OpenDrain
     * @return Result<void, ErrorCode> Ok() on success
     */
    Result<void, ErrorCode> setDrive(PinDrive drive) {
        auto* port = get_port();

        uint32_t otyper = port->OTYPER.load();
        if (drive == PinDrive::OpenDrain) {
            otyper |= pin_mask;
        } else {
            otyper &= ~pin_mask;
        }
        port->OTYPER.store(otyper);

        return Ok();
    }

    /**
     * @brief Configure pull resistor
     *
     * @param pull None, PullUp, or PullDown
     * @return Result<void, ErrorCode> Ok() on success
     */
    Result<void, ErrorCode> setPull(PinPull pull) {
        auto* port = get_port();

        uint32_t pupdr = port->PUPDR.load();
        pupdr &= ~(0x3u << (PIN_NUM * 2));  // Clear 2 bits

        uint32_t value = 0;
        switch (pull) {
            case PinPull::None:
                value = 0x0;
                break;
            case PinPull::PullUp:
                value = 0x1;
                break;
            case PinPull::PullDown:
                value = 0x2;
                break;
        }

        pupdr |= (value << (PIN_NUM * 2));
        port->PUPDR.store(pupdr);

        return Ok();
    }

    /**
     * @brief Check if pin is configured as output
     *
     * @return Result<bool, ErrorCode> True if output mode
     */
    Result<bool, ErrorCode> isOutput() const {
        auto* port = get_port();

        uint32_t moder = port->MODER.load();
        bool is_output = ((moder >> (PIN_NUM * 2)) & 0x3) == 0x1;

        return Ok(bool(is_output));
    }

    /**
     * @brief Get mock registers for testing/inspection
     *
     * Allows tests to verify register state
     *
     * @return const MockGpioRegisters* Pointer to mock registers
     */
    static const MockGpioRegisters* get_mock_registers() {
        return get_port();
    }

    /**
     * @brief Inject input value for testing
     *
     * Allows tests to simulate external signals
     *
     * @param value Input value to inject
     */
    void inject_input(bool value) {
        auto* port = get_port();

        uint32_t idr = port->IDR.load();
        if (value) {
            idr |= pin_mask;
        } else {
            idr &= ~pin_mask;
        }
        port->IDR.store(idr);
    }
};

// ============================================================================
// Port Base Definitions (for compatibility)
// ============================================================================

constexpr uint32_t GPIOA_PORT = 0;
constexpr uint32_t GPIOB_PORT = 1;
constexpr uint32_t GPIOC_PORT = 2;
constexpr uint32_t GPIOD_PORT = 3;
constexpr uint32_t GPIOE_PORT = 4;
constexpr uint32_t GPIOF_PORT = 5;
constexpr uint32_t GPIOG_PORT = 6;
constexpr uint32_t GPIOH_PORT = 7;

}  // namespace ucore::hal::host
