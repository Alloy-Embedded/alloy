#pragma once

#include <stdint.h>
#include "hardware.hpp"
#include "pins.hpp"

namespace alloy::hal::atmel::samd21::atsamd21e18a {

// ============================================================================
// Type-Safe GPIO Pin Abstraction
//
// Template-based GPIO abstraction using hardware PORT registers.
// All operations are constexpr and inline, resulting in assembly identical
// to direct register manipulation.
//
// SAMD21 Architecture:
// - PORT controller with groups: PORT->GROUP[0] = PORTA, GROUP[1] = PORTB
// - Each port has 32 pins (0-31)
// - 8 peripheral functions (A-H) via PMUX register
//
// Usage:
//   using Led = GPIOPin<pins::PA17>;
//   Led::configureOutput();
//   Led::set();
//   Led::toggle();
// ============================================================================

template<uint8_t GlobalPin>
class GPIOPin {
public:
    // Compile-time port/pin calculation
    static constexpr uint8_t PORT_IDX = GlobalPin / 32;  // 0=A, 1=B
    static constexpr uint8_t PIN = GlobalPin % 32;
    static constexpr uint32_t MASK = (1U << PIN);

    static_assert(PORT_IDX < 1, "Invalid port index for this MCU variant");

    // Get PORT register group at compile-time
    static inline hardware::PORT_Group* getPortGroup() {
        return &hardware::PORT->GROUP[PORT_IDX];
    }

    // ========================================================================
    // Pin Mode Configuration
    // ========================================================================

    /// Configure pin as GPIO output
    static inline void configureOutput() {
        auto* port = getPortGroup();
        port->PINCFG[PIN] = 0;                     // Disable PMUX (GPIO mode)
        port->DIRSET = MASK;                       // Set direction to output
    }

    /// Configure pin as GPIO input
    static inline void configureInput() {
        auto* port = getPortGroup();
        port->PINCFG[PIN] = hardware::PORT_PINCFG_INEN;  // Enable input buffer
        port->DIRCLR = MASK;                              // Clear direction (input)
    }

    /// Configure pin as input with pull-up
    static inline void configureInputPullUp() {
        auto* port = getPortGroup();
        port->PINCFG[PIN] = hardware::PORT_PINCFG_INEN | hardware::PORT_PINCFG_PULLEN;
        port->DIRCLR = MASK;                       // Clear direction (input)
        port->OUTSET = MASK;                       // Pull-up (OUT selects up/down)
    }

    /// Configure pin as input with pull-down
    static inline void configureInputPullDown() {
        auto* port = getPortGroup();
        port->PINCFG[PIN] = hardware::PORT_PINCFG_INEN | hardware::PORT_PINCFG_PULLEN;
        port->DIRCLR = MASK;                       // Clear direction (input)
        port->OUTCLR = MASK;                       // Pull-down (OUT selects up/down)
    }

    // ========================================================================
    // Digital I/O Operations
    // ========================================================================

    /// Set pin high
    static inline void set() {
        getPortGroup()->OUTSET = MASK;
    }

    /// Set pin low
    static inline void clear() {
        getPortGroup()->OUTCLR = MASK;
    }

    /// Toggle pin state (hardware toggle on SAMD21)
    static inline void toggle() {
        getPortGroup()->OUTTGL = MASK;
    }

    /// Write boolean value to pin
    static inline void write(bool value) {
        if (value) {
            set();
        } else {
            clear();
        }
    }

    /// Read pin state
    [[nodiscard]] static inline bool read() {
        return (getPortGroup()->IN & MASK) != 0;
    }

    // ========================================================================
    // Peripheral Function Configuration (A-H)
    // ========================================================================

    enum class PeripheralFunction : uint8_t {
        GPIO = 0xFF,  // GPIO mode (PMUX disabled)
        A = 0,        // Peripheral A
        B = 1,        // Peripheral B
        C = 2,        // Peripheral C
        D = 3,        // Peripheral D
        E = 4,        // Peripheral E
        F = 5,        // Peripheral F
        G = 6,        // Peripheral G
        H = 7,        // Peripheral H
    };

    /// Configure pin for peripheral function (A-H)
    static inline void configurePeripheral(PeripheralFunction func) {
        auto* port = getPortGroup();

        if (func == PeripheralFunction::GPIO) {
            port->PINCFG[PIN] &= ~hardware::PORT_PINCFG_PMUXEN;  // Disable PMUX
            return;
        }

        // Configure PMUX register (each register controls 2 pins)
        uint8_t pmux_idx = PIN / 2;
        uint8_t pmux_shift = (PIN & 1) ? 4 : 0;  // Odd pin = upper nibble, even = lower
        uint8_t func_val = static_cast<uint8_t>(func);

        uint8_t pmux_val = port->PMUX[pmux_idx];
        pmux_val &= ~(0xF << pmux_shift);            // Clear old value
        pmux_val |= (func_val << pmux_shift);        // Set new value
        port->PMUX[pmux_idx] = pmux_val;

        port->PINCFG[PIN] |= hardware::PORT_PINCFG_PMUXEN;  // Enable PMUX
    }

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /// Enable strong drive strength
    static inline void enableStrongDrive() {
        getPortGroup()->PINCFG[PIN] |= hardware::PORT_PINCFG_DRVSTR;
    }

    /// Disable strong drive (normal strength)
    static inline void disableStrongDrive() {
        getPortGroup()->PINCFG[PIN] &= ~hardware::PORT_PINCFG_DRVSTR;
    }

    /// Enable input buffer
    static inline void enableInput() {
        getPortGroup()->PINCFG[PIN] |= hardware::PORT_PINCFG_INEN;
    }

    /// Disable input buffer
    static inline void disableInput() {
        getPortGroup()->PINCFG[PIN] &= ~hardware::PORT_PINCFG_INEN;
    }
};

// Re-export from sub-namespaces for convenience
using namespace pins;

}  // namespace alloy::hal::atmel::samd21::atsamd21e18a
