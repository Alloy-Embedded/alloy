// This file will be copied to src/hal/vendors/atmel/same70/pio_hal.hpp
#pragma once

#include <cstdint>

namespace alloy::hal::atmel::same70 {

// ============================================================================
// SAME70 PIO (Parallel I/O) HAL
//
// Architecture: Atmel/Microchip SAME70 uses PIO controller instead of standard GPIO
// - Each pin can be PIO (GPIO) or assigned to peripheral A, B, C, or D
// - Controlled via PIO_ABCDSR1 and PIO_ABCDSR2 registers
// ============================================================================

template<typename Hardware, uint8_t Pin>
class PIOPin {
public:
    static_assert(Pin < 32, "Pin number must be 0-31");

    // Pin mask for register operations
    static constexpr uint32_t PIN_MASK = (1U << Pin);

    // ========================================================================
    // Pin Mode Configuration
    // ========================================================================

    // Configure pin as GPIO output
    static void configureOutput() {
        // Enable PIO control (disable peripheral)
        getPort()->PER = PIN_MASK;

        // Enable output
        getPort()->OER = PIN_MASK;

        // Disable pull-up and pull-down
        getPort()->PUDR = PIN_MASK;
        getPort()->PPDDR = PIN_MASK;
    }

    // Configure pin as GPIO input
    static void configureInput() {
        // Enable PIO control
        getPort()->PER = PIN_MASK;

        // Disable output (input mode)
        getPort()->ODR = PIN_MASK;

        // Disable pull-up and pull-down by default
        getPort()->PUDR = PIN_MASK;
        getPort()->PPDDR = PIN_MASK;
    }

    // Configure pin as GPIO input with pull-up
    static void configureInputPullUp() {
        configureInput();
        getPort()->PUER = PIN_MASK;  // Enable pull-up
    }

    // Configure pin as GPIO input with pull-down
    static void configureInputPullDown() {
        configureInput();
        getPort()->PPDER = PIN_MASK;  // Enable pull-down
    }

    // Configure pin as GPIO input with glitch filter
    static void configureInputFiltered() {
        configureInput();
        getPort()->IFER = PIN_MASK;  // Enable glitch filter
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

    // Configure pin for peripheral function
    static void configurePeripheral(PeripheralFunction func) {
        if (func == PeripheralFunction::PIO) {
            // Enable PIO control
            getPort()->PER = PIN_MASK;
            return;
        }

        // Disable PIO control (enable peripheral)
        getPort()->PDR = PIN_MASK;

        // Set peripheral function using ABCDSR registers
        // ABCDSR[0] = bit 0, ABCDSR[1] = bit 1
        // 00 = A, 01 = B, 10 = C, 11 = D

        uint8_t periph_val = static_cast<uint8_t>(func) - 1;  // A=0, B=1, C=2, D=3

        if (periph_val & 0x01) {
            getPort()->ABCDSR[0] |= PIN_MASK;  // Set bit 0
        } else {
            getPort()->ABCDSR[0] &= ~PIN_MASK;  // Clear bit 0
        }

        if (periph_val & 0x02) {
            getPort()->ABCDSR[1] |= PIN_MASK;  // Set bit 1
        } else {
            getPort()->ABCDSR[1] &= ~PIN_MASK;  // Clear bit 1
        }
    }

    // ========================================================================
    // Digital I/O Operations
    // ========================================================================

    // Set pin high
    static void set() {
        getPort()->SODR = PIN_MASK;  // Set Output Data Register
    }

    // Set pin low
    static void clear() {
        getPort()->CODR = PIN_MASK;  // Clear Output Data Register
    }

    // Toggle pin
    static void toggle() {
        if (getPort()->ODSR & PIN_MASK) {
            clear();
        } else {
            set();
        }
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
        return (getPort()->PDSR & PIN_MASK) != 0;  // Pin Data Status Register
    }

    // ========================================================================
    // Advanced Features
    // ========================================================================

    // Enable multi-driver (open-drain) mode
    static void enableMultiDriver() {
        getPort()->MDER = PIN_MASK;
    }

    // Disable multi-driver mode
    static void disableMultiDriver() {
        getPort()->MDDR = PIN_MASK;
    }

    // Configure Schmitt trigger
    static void enableSchmitt() {
        getPort()->SCHMITT |= PIN_MASK;
    }

    static void disableSchmitt() {
        getPort()->SCHMITT &= ~PIN_MASK;
    }

    // Configure drive strength (requires checking DRIVER register layout)
    enum class DriveStrength : uint8_t {
        Low = 0,
        High = 1,
    };

    static void setDriveStrength(DriveStrength strength) {
        // DRIVER register: 2 bits per pin (bits [2*Pin+1:2*Pin])
        // This is a simplified version - actual implementation may vary
        uint32_t shift = Pin * 2;
        uint32_t mask = 0x3U << shift;
        uint32_t value = static_cast<uint32_t>(strength) << shift;

        getPort()->DRIVER = (getPort()->DRIVER & ~mask) | value;
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

    static void configureInterrupt(InterruptMode mode) {
        if (mode == InterruptMode::Disabled) {
            getPort()->IDR = PIN_MASK;  // Disable interrupt
            return;
        }

        // Enable additional interrupt modes
        getPort()->AIMER = PIN_MASK;

        // Configure interrupt type
        switch (mode) {
            case InterruptMode::RisingEdge:
                getPort()->ESR = PIN_MASK;      // Edge select
                getPort()->REHLSR = PIN_MASK;   // Rising edge
                break;

            case InterruptMode::FallingEdge:
                getPort()->ESR = PIN_MASK;      // Edge select
                getPort()->FELLSR = PIN_MASK;   // Falling edge
                break;

            case InterruptMode::BothEdges:
                getPort()->ESR = PIN_MASK;      // Edge select
                // Both edges mode (implementation varies)
                break;

            case InterruptMode::LowLevel:
                getPort()->LSR = PIN_MASK;      // Level select
                getPort()->FELLSR = PIN_MASK;   // Low level
                break;

            case InterruptMode::HighLevel:
                getPort()->LSR = PIN_MASK;      // Level select
                getPort()->REHLSR = PIN_MASK;   // High level
                break;

            default:
                break;
        }

        // Enable interrupt
        getPort()->IER = PIN_MASK;
    }

    // Check interrupt status
    static bool isInterruptPending() {
        return (getPort()->ISR & PIN_MASK) != 0;
    }

private:
    // Get the appropriate PIO port based on pin characteristics
    // This will be specialized per port in actual usage
    static Hardware* getPort();
};

}  // namespace alloy::hal::atmel::same70
