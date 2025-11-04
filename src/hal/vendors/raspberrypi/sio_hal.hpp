// This file will be copied to src/hal/vendors/raspberrypi/sio_hal.hpp
#pragma once

#include <cstdint>
#include "rp2040/hardware.hpp"

namespace alloy::hal::raspberrypi::rp2040 {

// ============================================================================
// RP2040 SIO (Single-cycle I/O) HAL
//
// Architecture: Raspberry Pi RP2040 uses SIO for fast GPIO access
// - Direct register access for GPIO read/write (single-cycle)
// - Function selection via IO_BANK0
// - Pad configuration via PADS_BANK0
// ============================================================================

template<uint8_t Pin>
class SIOPin {
public:
    static_assert(Pin < 30, "Pin number must be 0-29");

    // Pin mask for register operations
    static constexpr uint32_t PIN_MASK = (1U << Pin);

    // ========================================================================
    // Pin Mode Configuration
    // ========================================================================

    // Configure pin as GPIO output
    static void configureOutput() {
        // Set function to SIO (GPIO)
        setFunction(5);  // FUNCSEL_SIO

        // Enable output
        hardware::SIO->GPIO_OE_SET = PIN_MASK;

        // Configure pad: enable output, disable input to save power
        configurePad(hardware::PADS_IE);
    }

    // Configure pin as GPIO input
    static void configureInput() {
        // Set function to SIO (GPIO)
        setFunction(5);  // FUNCSEL_SIO

        // Disable output
        hardware::SIO->GPIO_OE_CLR = PIN_MASK;

        // Configure pad: enable input, no pulls
        configurePad(hardware::PADS_IE);
    }

    // Configure pin as input with pull-up
    static void configureInputPullUp() {
        // Set function to SIO (GPIO)
        setFunction(5);  // FUNCSEL_SIO

        // Disable output
        hardware::SIO->GPIO_OE_CLR = PIN_MASK;

        // Configure pad: enable input and pull-up
        configurePad(hardware::PADS_IE | hardware::PADS_PUE);
    }

    // Configure pin as input with pull-down
    static void configureInputPullDown() {
        // Set function to SIO (GPIO)
        setFunction(5);  // FUNCSEL_SIO

        // Disable output
        hardware::SIO->GPIO_OE_CLR = PIN_MASK;

        // Configure pad: enable input and pull-down
        configurePad(hardware::PADS_IE | hardware::PADS_PDE);
    }

    // ========================================================================
    // Peripheral Function Configuration
    // ========================================================================

    enum class PeripheralFunction : uint8_t {
        GPIO  = 5,   // SIO (GPIO mode)
        SPI   = 1,
        UART  = 2,
        I2C   = 3,
        PWM   = 4,
        PIO0  = 6,
        PIO1  = 7,
        CLOCK = 8,
        USB   = 9,
        NULL_FUNC = 31,
    };

    // Configure pin for peripheral function
    static void configurePeripheral(PeripheralFunction func) {
        setFunction(static_cast<uint8_t>(func));

        // For most peripherals, enable input in pad
        if (func != PeripheralFunction::GPIO) {
            configurePad(hardware::PADS_IE);
        }
    }

    // ========================================================================
    // Digital I/O Operations (Single-cycle)
    // ========================================================================

    // Set pin high
    static void set() {
        hardware::SIO->GPIO_OUT_SET = PIN_MASK;
    }

    // Set pin low
    static void clear() {
        hardware::SIO->GPIO_OUT_CLR = PIN_MASK;
    }

    // Toggle pin
    static void toggle() {
        hardware::SIO->GPIO_OUT_XOR = PIN_MASK;
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
        return (hardware::SIO->GPIO_IN & PIN_MASK) != 0;
    }

    // ========================================================================
    // Advanced Features
    // ========================================================================

    // Set drive strength
    enum class DriveStrength : uint8_t {
        Drive_2mA  = 0,
        Drive_4mA  = 1,
        Drive_8mA  = 2,
        Drive_12mA = 3,
    };

    static void setDriveStrength(DriveStrength strength) {
        uint32_t pad = hardware::PADS_BANK0->GPIO[Pin];
        pad &= ~(3 << 4);  // Clear drive bits
        pad |= (static_cast<uint32_t>(strength) << 4);
        hardware::PADS_BANK0->GPIO[Pin] = pad;
    }

    // Enable/disable slew rate limiting
    static void setSlewFast(bool fast) {
        if (fast) {
            hardware::PADS_BANK0->GPIO[Pin] |= hardware::PADS_SLEWFAST;
        } else {
            hardware::PADS_BANK0->GPIO[Pin] &= ~hardware::PADS_SLEWFAST;
        }
    }

    // Enable/disable Schmitt trigger
    static void setSchmitt(bool enable) {
        if (enable) {
            hardware::PADS_BANK0->GPIO[Pin] |= hardware::PADS_SCHMITT;
        } else {
            hardware::PADS_BANK0->GPIO[Pin] &= ~hardware::PADS_SCHMITT;
        }
    }

private:
    // Set GPIO function via IO_BANK0
    static void setFunction(uint8_t funcsel) {
        hardware::IO_BANK0->GPIO[Pin].CTRL = funcsel & 0x1F;
    }

    // Configure pad settings
    static void configurePad(uint32_t config) {
        // Set pad configuration (input enable, pulls, drive, etc.)
        hardware::PADS_BANK0->GPIO[Pin] = config;
    }
};

}  // namespace alloy::hal::raspberrypi::rp2040
