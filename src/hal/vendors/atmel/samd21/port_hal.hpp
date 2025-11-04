// This file will be copied to src/hal/vendors/atmel/samd21/port_hal.hpp
#pragma once

#include <cstdint>

namespace alloy::hal::atmel::samd21 {

// ============================================================================
// SAMD21 PORT (Parallel I/O) HAL
//
// Architecture: Atmel/Microchip SAMD21 uses PORT controller
// - Each pin can be GPIO or assigned to peripheral function A-H
// - Controlled via PORT GROUP registers
// - 8 peripheral functions per pin (vs 4 on SAME70)
// ============================================================================

template<typename Hardware, uint8_t PortIndex, uint8_t Pin>
class PORTPin {
public:
    static_assert(Pin < 32, "Pin number must be 0-31");
    static_assert(PortIndex < 2, "Port index must be 0 (A) or 1 (B)");

    // Pin mask for register operations
    static constexpr uint32_t PIN_MASK = (1U << Pin);
    
    // PMUX index (2 pins per PMUX register)
    static constexpr uint8_t PMUX_INDEX = Pin / 2;
    static constexpr uint8_t PMUX_SHIFT = (Pin & 1) ? 4 : 0;
    static constexpr uint8_t PMUX_MASK = 0xF << PMUX_SHIFT;

    // ========================================================================
    // Pin Mode Configuration
    // ========================================================================

    // Configure pin as GPIO output
    static void configureOutput() {
        auto* port = getPort();
        
        // Disable peripheral multiplexing
        port->PINCFG[Pin] = 0;
        
        // Set as output
        port->DIRSET = PIN_MASK;
    }

    // Configure pin as GPIO input
    static void configureInput() {
        auto* port = getPort();
        
        // Disable peripheral multiplexing, enable input
        port->PINCFG[Pin] = 0x02;  // INEN
        
        // Set as input
        port->DIRCLR = PIN_MASK;
    }

    // Configure pin as GPIO input with pull-up
    static void configureInputPullUp() {
        auto* port = getPort();
        
        // Enable input and pull
        port->PINCFG[Pin] = 0x06;  // INEN | PULLEN
        
        // Set as input
        port->DIRCLR = PIN_MASK;
        
        // Enable pull-up by setting output high
        port->OUTSET = PIN_MASK;
    }

    // Configure pin as GPIO input with pull-down
    static void configureInputPullDown() {
        auto* port = getPort();
        
        // Enable input and pull
        port->PINCFG[Pin] = 0x06;  // INEN | PULLEN
        
        // Set as input
        port->DIRCLR = PIN_MASK;
        
        // Enable pull-down by setting output low
        port->OUTCLR = PIN_MASK;
    }

    // ========================================================================
    // Peripheral Function Configuration
    // ========================================================================

    enum class PeripheralFunction : uint8_t {
        GPIO = 0xFF,  // GPIO mode (no peripheral)
        A = 0,        // Peripheral A
        B = 1,        // Peripheral B
        C = 2,        // Peripheral C
        D = 3,        // Peripheral D
        E = 4,        // Peripheral E
        F = 5,        // Peripheral F
        G = 6,        // Peripheral G
        H = 7,        // Peripheral H
    };

    // Configure pin for peripheral function
    static void configurePeripheral(PeripheralFunction func) {
        auto* port = getPort();
        
        if (func == PeripheralFunction::GPIO) {
            // Disable peripheral multiplexing
            port->PINCFG[Pin] &= ~0x01;  // Clear PMUXEN
            return;
        }

        // Set peripheral function in PMUX register
        uint8_t pmux_val = port->PMUX[PMUX_INDEX];
        pmux_val &= ~PMUX_MASK;  // Clear old value
        pmux_val |= (static_cast<uint8_t>(func) << PMUX_SHIFT);
        port->PMUX[PMUX_INDEX] = pmux_val;

        // Enable peripheral multiplexing
        port->PINCFG[Pin] |= 0x01;  // Set PMUXEN
    }

    // ========================================================================
    // Digital I/O Operations
    // ========================================================================

    // Set pin high
    static void set() {
        getPort()->OUTSET = PIN_MASK;
    }

    // Set pin low
    static void clear() {
        getPort()->OUTCLR = PIN_MASK;
    }

    // Toggle pin
    static void toggle() {
        getPort()->OUTTGL = PIN_MASK;
    }

    // Write value to pin
    static void write(bool value) {
        if (value) {
            set();
        } else {
            clear();
        }
    }

    // Read pin state
    static bool read() {
        return (getPort()->IN & PIN_MASK) != 0;
    }

    // ========================================================================
    // Advanced Features
    // ========================================================================

    // Enable strong drive strength
    static void enableStrongDrive() {
        getPort()->PINCFG[Pin] |= 0x40;  // Set DRVSTR
    }

    // Disable strong drive strength (default)
    static void disableStrongDrive() {
        getPort()->PINCFG[Pin] &= ~0x40;  // Clear DRVSTR
    }

    // Enable input buffer
    static void enableInput() {
        getPort()->PINCFG[Pin] |= 0x02;  // Set INEN
    }

    // Disable input buffer
    static void disableInput() {
        getPort()->PINCFG[Pin] &= ~0x02;  // Clear INEN
    }

private:
    // Get the appropriate PORT group based on port index
    static auto* getPort() {
        return &Hardware::PORT->GROUP[PortIndex];
    }
};

}  // namespace alloy::hal::atmel::samd21
