#pragma once

#include <stdint.h>

// Include auto-generated register definitions
#include "registers/pioa_registers.hpp"
#include "registers/piob_registers.hpp"
#include "registers/pioc_registers.hpp"
#include "registers/piod_registers.hpp"

#include "hardware.hpp"
#include "pins.hpp"
#include "pin_functions.hpp"

namespace alloy::hal::atmel::samv71::atsamv71n21b {

// ============================================================================
// Port Register Mapping (Compile-Time)
//
// Maps port index to actual PIO register instance from generated files
// This is fully resolved at compile time with zero runtime overhead
// ============================================================================

template<uint8_t Port>
constexpr auto getPortRegister() {
    if constexpr (Port == 0) return pioa::PIOA;
    if constexpr (Port == 1) return piob::PIOB;
    if constexpr (Port == 2) return pioc::PIOC;
    if constexpr (Port == 3) return piod::PIOD;
    else static_assert(Port < 4, "Invalid port index");
}

// ============================================================================
// Type-Safe GPIO Pin Abstraction
//
// Template-based GPIO abstraction using auto-generated register types.
// All operations are constexpr and inline, resulting in assembly identical
// to direct register manipulation.
//
// Features:
// - Type-safe at compile time
// - Zero runtime overhead
// - Uses auto-generated register definitions
// - Clean, high-level API
//
// Usage:
//   using Led = GPIOPin<pins::PC8>;
//   Led::configureOutput();
//   Led::set();
//   Led::toggle();
// ============================================================================

template<uint8_t GlobalPin>
class GPIOPin {
public:
    // Compile-time port/pin calculation
    static constexpr uint8_t PORT = GlobalPin / 32;
    static constexpr uint8_t PIN = GlobalPin % 32;
    static constexpr uint32_t MASK = (1U << PIN);

    // Get port register at compile-time
    using PortRegType = decltype(getPortRegister<PORT>());
    static constexpr PortRegType PORT_REG = getPortRegister<PORT>();

    // ========================================================================
    // Pin Mode Configuration
    // ========================================================================

    /// Configure pin as GPIO output
    static inline void configureOutput() {
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->OER = MASK;    // Enable output
        PORT_REG->PUDR = MASK;   // Disable pull-up
        PORT_REG->PPDDR = MASK;  // Disable pull-down
    }

    /// Configure pin as GPIO input
    static inline void configureInput() {
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->ODR = MASK;    // Disable output (input mode)
        PORT_REG->PUDR = MASK;   // Disable pull-up
        PORT_REG->PPDDR = MASK;  // Disable pull-down
    }

    /// Configure pin as input with pull-up
    static inline void configureInputPullUp() {
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->ODR = MASK;    // Disable output
        PORT_REG->PUER = MASK;   // Enable pull-up
        PORT_REG->PPDDR = MASK;  // Disable pull-down
    }

    /// Configure pin as input with pull-down
    static inline void configureInputPullDown() {
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->ODR = MASK;    // Disable output
        PORT_REG->PUDR = MASK;   // Disable pull-up
        PORT_REG->PPDER = MASK;  // Enable pull-down
    }

    /// Configure pin as input with glitch filter
    static inline void configureInputFiltered() {
        configureInput();
        PORT_REG->IFER = MASK;   // Enable glitch filter
    }

    // ========================================================================
    // Digital I/O Operations
    // ========================================================================

    /// Set pin high
    static inline void set() {
        PORT_REG->SODR = MASK;   // Set Output Data Register
    }

    /// Set pin low
    static inline void clear() {
        PORT_REG->CODR = MASK;   // Clear Output Data Register
    }

    /// Toggle pin state
    static inline void toggle() {
        if (PORT_REG->ODSR & MASK) {
            clear();
        } else {
            set();
        }
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
        return (PORT_REG->PDSR & MASK) != 0;
    }

    // ========================================================================
    // Peripheral Function Configuration
    // ========================================================================

    enum class PeripheralFunction : uint8_t {
        PIO = 0,  // GPIO mode
        A = 1,    // Peripheral A
        B = 2,    // Peripheral B
        C = 3,    // Peripheral C
        D = 4,    // Peripheral D
    };

    /// Configure pin for peripheral function (A, B, C, D)
    static inline void configurePeripheral(PeripheralFunction func) {
        if (func == PeripheralFunction::PIO) {
            PORT_REG->PER = MASK;  // Enable PIO control
            return;
        }

        // Disable PIO control (enable peripheral)
        PORT_REG->PDR = MASK;

        // Set peripheral function using ABCDSR registers
        // ABCDSR[0] = bit 0, ABCDSR[1] = bit 1
        // 00 = A, 01 = B, 10 = C, 11 = D
        uint8_t periph_val = static_cast<uint8_t>(func) - 1;

        if (periph_val & 0x01) {
            PORT_REG->ABCDSR[0] |= MASK;
        } else {
            PORT_REG->ABCDSR[0] &= ~MASK;
        }

        if (periph_val & 0x02) {
            PORT_REG->ABCDSR[1] |= MASK;
        } else {
            PORT_REG->ABCDSR[1] &= ~MASK;
        }
    }

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /// Enable multi-driver (open-drain) mode
    static inline void enableMultiDriver() {
        PORT_REG->MDER = MASK;
    }

    /// Disable multi-driver mode
    static inline void disableMultiDriver() {
        PORT_REG->MDDR = MASK;
    }

    /// Enable Schmitt trigger
    static inline void enableSchmitt() {
        PORT_REG->SCHMITT |= MASK;
    }

    /// Disable Schmitt trigger
    static inline void disableSchmitt() {
        PORT_REG->SCHMITT &= ~MASK;
    }

    // ========================================================================
    // Interrupt Configuration
    // ========================================================================

    enum class InterruptMode : uint8_t {
        Disabled = 0,
        RisingEdge = 1,
        FallingEdge = 2,
        BothEdges = 3,
        LowLevel = 4,
        HighLevel = 5,
    };

    /// Configure interrupt mode
    static inline void configureInterrupt(InterruptMode mode) {
        if (mode == InterruptMode::Disabled) {
            PORT_REG->IDR = MASK;
            return;
        }

        PORT_REG->AIMER = MASK;  // Enable additional interrupt modes

        switch (mode) {
            case InterruptMode::RisingEdge:
                PORT_REG->ESR = MASK;      // Edge select
                PORT_REG->REHLSR = MASK;   // Rising edge
                break;

            case InterruptMode::FallingEdge:
                PORT_REG->ESR = MASK;      // Edge select
                PORT_REG->FELLSR = MASK;   // Falling edge
                break;

            case InterruptMode::BothEdges:
                PORT_REG->ESR = MASK;      // Edge select
                break;

            case InterruptMode::LowLevel:
                PORT_REG->LSR = MASK;      // Level select
                PORT_REG->FELLSR = MASK;   // Low level
                break;

            case InterruptMode::HighLevel:
                PORT_REG->LSR = MASK;      // Level select
                PORT_REG->REHLSR = MASK;   // High level
                break;

            default:
                break;
        }

        PORT_REG->IER = MASK;  // Enable interrupt
    }

    /// Check if interrupt is pending
    [[nodiscard]] static inline bool isInterruptPending() {
        return (PORT_REG->ISR & MASK) != 0;
    }
};

// Re-export from sub-namespaces for convenience
using namespace pins;
using namespace pin_functions;

}  // namespace alloy::hal::atmel::samv71::atsamv71n21b
