/**
 * @file gpio_mock.hpp
 * @brief Mock GPIO/PIO registers for unit testing (without hardware)
 *
 * This file provides a mock implementation of PIO registers that can be
 * used for testing the GPIO template without requiring actual hardware.
 *
 * Key features:
 * - Simulates PIO register behavior
 * - Tracks register writes for verification
 * - Simulates pin state changes
 * - Zero hardware dependency
 */

#pragma once

#include <cstdint>
#include <bitset>

// Include actual PIO register definition for compatibility
#include "hal/vendors/atmel/same70/atsame70q21/registers/pioa_registers.hpp"

namespace alloy::hal::test {

// Import the actual register type
using PioRegisters = alloy::hal::atmel::same70::atsame70q21::pioa::PIOA_Registers;

/**
 * @brief Mock PIO registers for testing
 *
 * Simulates PIO register behavior for unit testing.
 * Tracks all register accesses and simulates hardware behavior.
 *
 * This class is layout-compatible with the actual PIOA_Registers structure
 * so it can be safely cast for use with the GPIO template.
 */
class MockGpioRegisters : public PioRegisters {
public:
    // Test state
    std::bitset<32> pin_output_state;    ///< Simulated output pin states
    std::bitset<32> pin_input_state;     ///< Simulated input pin states (for read)

    /**
     * @brief Reset mock to initial state
     */
    void reset() {
        // Reset all register values (from base class)
        this->PER = 0;
        this->PDR = 0;
        this->PSR = 0;
        this->OER = 0;
        this->ODR = 0;
        this->OSR = 0;
        this->IFER = 0;
        this->IFDR = 0;
        this->IFSR = 0;
        this->SODR = 0;
        this->CODR = 0;
        this->ODSR = 0;
        this->PDSR = 0;
        this->IER = 0;
        this->IDR = 0;
        this->IMR = 0;
        this->ISR = 0;
        this->MDER = 0;
        this->MDDR = 0;
        this->MDSR = 0;
        this->PUDR = 0;
        this->PUER = 0;
        this->PUSR = 0;
        this->ABCDSR[0][0] = 0;
        this->ABCDSR[0][1] = 0;
        this->ABCDSR[1][0] = 0;
        this->ABCDSR[1][1] = 0;
        this->IFSCDR = 0;
        this->IFSCER = 0;
        this->IFSCSR = 0;
        this->SCDR = 0;
        this->PPDDR = 0;
        this->PPDER = 0;
        this->PPDSR = 0;
        this->OWER = 0;
        this->OWDR = 0;
        this->OWSR = 0;
        this->AIMER = 0;
        this->AIMDR = 0;
        this->AIMMR = 0;
        this->ESR = 0;
        this->LSR = 0;
        this->ELSR = 0;
        this->FELLSR = 0;
        this->REHLSR = 0;
        this->FRLHSR = 0;
        this->LOCKSR = 0;
        this->WPMR = 0;
        this->WPSR = 0;
        this->SCHMITT = 0;
        this->DRIVER = 0;
        this->PCMR = 0;
        this->PCIER = 0;
        this->PCIDR = 0;
        this->PCIMR = 0;
        this->PCISR = 0;
        this->PCRHR = 0;

        // Reset test state
        pin_output_state.reset();
        pin_input_state.reset();
    }

    /**
     * @brief Simulate setting a pin HIGH
     *
     * Called when SODR is written
     */
    void sync_sodr() {
        if (this->SODR != 0) {
            // Set bits in output state
            for (int i = 0; i < 32; i++) {
                if (this->SODR & (1u << i)) {
                    pin_output_state.set(i);
                }
            }
            // Update ODSR to reflect new state
            this->ODSR |= this->SODR;
            // Update PDSR (pin data status reflects output when pin is output)
            this->PDSR = this->ODSR;
            // Clear SODR (write-only register)
            this->SODR = 0;
        }
    }

    /**
     * @brief Simulate clearing a pin LOW
     *
     * Called when CODR is written
     */
    void sync_codr() {
        if (this->CODR != 0) {
            // Clear bits in output state
            for (int i = 0; i < 32; i++) {
                if (this->CODR & (1u << i)) {
                    pin_output_state.reset(i);
                }
            }
            // Update ODSR to reflect new state
            this->ODSR &= ~this->CODR;
            // Update PDSR
            this->PDSR = this->ODSR;
            // Clear CODR (write-only register)
            this->CODR = 0;
        }
    }

    /**
     * @brief Simulate enabling PIO control
     *
     * Called when PER is written
     */
    void sync_per() {
        if (this->PER != 0) {
            // Enable PIO control for specified pins
            this->PSR |= this->PER;
            // Clear PER (write-only register)
            this->PER = 0;
        }
    }

    /**
     * @brief Simulate enabling output
     *
     * Called when OER is written
     */
    void sync_oer() {
        if (this->OER != 0) {
            // Enable output for specified pins
            this->OSR |= this->OER;
            // Clear OER (write-only register)
            this->OER = 0;
        }
    }

    /**
     * @brief Simulate disabling output
     *
     * Called when ODR is written
     */
    void sync_odr() {
        if (this->ODR != 0) {
            // Disable output for specified pins
            this->OSR &= ~this->ODR;
            // Clear ODR (write-only register)
            this->ODR = 0;
        }
    }

    /**
     * @brief Simulate enabling multi-driver (open-drain)
     *
     * Called when MDER is written
     */
    void sync_mder() {
        if (this->MDER != 0) {
            // Enable multi-driver for specified pins
            this->MDSR |= this->MDER;
            // Clear MDER (write-only register)
            this->MDER = 0;
        }
    }

    /**
     * @brief Simulate disabling multi-driver
     *
     * Called when MDDR is written
     */
    void sync_mddr() {
        if (this->MDDR != 0) {
            // Disable multi-driver for specified pins
            this->MDSR &= ~this->MDDR;
            // Clear MDDR (write-only register)
            this->MDDR = 0;
        }
    }

    /**
     * @brief Simulate enabling pull-up
     *
     * Called when PUER is written
     */
    void sync_puer() {
        if (this->PUER != 0) {
            // Enable pull-up for specified pins
            this->PUSR |= this->PUER;
            // Clear PUER (write-only register)
            this->PUER = 0;
        }
    }

    /**
     * @brief Simulate disabling pull-up
     *
     * Called when PUDR is written
     */
    void sync_pudr() {
        if (this->PUDR != 0) {
            // Disable pull-up for specified pins
            this->PUSR &= ~this->PUDR;
            // Clear PUDR (write-only register)
            this->PUDR = 0;
        }
    }

    /**
     * @brief Simulate enabling input filter
     *
     * Called when IFER is written
     */
    void sync_ifer() {
        if (this->IFER != 0) {
            // Enable filter for specified pins
            this->IFSR |= this->IFER;
            // Clear IFER (write-only register)
            this->IFER = 0;
        }
    }

    /**
     * @brief Simulate disabling input filter
     *
     * Called when IFDR is written
     */
    void sync_ifdr() {
        if (this->IFDR != 0) {
            // Disable filter for specified pins
            this->IFSR &= ~this->IFDR;
            // Clear IFDR (write-only register)
            this->IFDR = 0;
        }
    }

    /**
     * @brief Set simulated input pin state
     *
     * Used by tests to simulate external signals
     */
    void set_input_pin(uint8_t pin, bool value) {
        if (value) {
            pin_input_state.set(pin);
        } else {
            pin_input_state.reset(pin);
        }

        // Update PDSR for input pins
        if (!(this->OSR & (1u << pin))) {
            // Pin is input - update PDSR
            if (value) {
                this->PDSR |= (1u << pin);
            } else {
                this->PDSR &= ~(1u << pin);
            }
        }
    }

    /**
     * @brief Get output pin state
     */
    bool get_output_pin(uint8_t pin) const {
        return pin_output_state.test(pin);
    }

    /**
     * @brief Check if pin is configured as output
     */
    bool is_output(uint8_t pin) const {
        return (this->OSR & (1u << pin)) != 0;
    }

    /**
     * @brief Check if PIO control is enabled for pin
     */
    bool is_pio_enabled(uint8_t pin) const {
        return (this->PSR & (1u << pin)) != 0;
    }
};

/**
 * @brief Global mock instance (replaceable for testing)
 */
inline MockGpioRegisters* g_mock_gpio = nullptr;

/**
 * @brief RAII helper to set up and tear down GPIO mocks
 */
class MockGpioFixture {
public:
    MockGpioFixture() {
        gpio_mock.reset();
        g_mock_gpio = &gpio_mock;
    }

    ~MockGpioFixture() {
        g_mock_gpio = nullptr;
    }

    MockGpioRegisters& gpio() { return gpio_mock; }

private:
    MockGpioRegisters gpio_mock;
};

} // namespace alloy::hal::test
