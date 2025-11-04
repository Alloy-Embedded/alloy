/// ATSAMD21 GPIO Implementation
///
/// Modern C++20 GPIO implementation for SAMD21 microcontroller.
/// SAMD21 uses PORT module for GPIO with atomic set/clear/toggle operations.
///
/// Pin numbering: PA00=0, PA01=1, ..., PA31=31, PB00=32, PB01=33, etc.
///
/// Features:
/// - Compile-time pin validation using concepts
/// - Zero-cost abstractions (everything inline/constexpr)
/// - Atomic operations using DIRSET/DIRCLR/OUTSET/OUTCLR/OUTTGL
/// - Uses generated peripheral registers from SVD
///
/// Example usage:
/// \code
/// // Create LED pin (PA17 on Arduino Zero = pin 17)
/// GpioPin<17> led;
/// led.configure(PinMode::Output);
/// led.set_high();
/// led.toggle();
/// \endcode

#ifndef ALLOY_HAL_MICROCHIP_SAMD21_GPIO_HPP
#define ALLOY_HAL_MICROCHIP_SAMD21_GPIO_HPP

#include "../../interface/gpio.hpp"
#include <peripherals.hpp>
#include <cstdint>

namespace alloy::hal::samd21 {

/// Default to the configured MCU namespace (set by CMake)
/// This can be overridden by boards that need a different variant
namespace mcu = alloy::generated::atsamd21j18a;

/// GPIO port identifiers
enum class Port : uint8_t {
    A = 0,
    B = 1
};

/// Extract port from pin number
/// SAMD21 uses format: Port * 32 + Pin
/// PA00=0, PA01=1, ..., PA31=31, PB00=32, PB01=33, etc.
constexpr inline Port get_port_from_pin(uint8_t pin) {
    return static_cast<Port>(pin / 32);
}

/// Extract pin bit (0-31) from pin number
constexpr inline uint8_t get_pin_bit(uint8_t pin) {
    return pin % 32;
}

/// PORT Group register structure for SAMD21
/// Each port (A, B) has its own group of registers
struct PortGroup {
    volatile uint32_t DIR;        // 0x00: Direction
    volatile uint32_t DIRCLR;     // 0x04: Direction Clear
    volatile uint32_t DIRSET;     // 0x08: Direction Set
    volatile uint32_t DIRTGL;     // 0x0C: Direction Toggle
    volatile uint32_t OUT;        // 0x10: Output Value
    volatile uint32_t OUTCLR;     // 0x14: Output Value Clear
    volatile uint32_t OUTSET;     // 0x18: Output Value Set
    volatile uint32_t OUTTGL;     // 0x1C: Output Value Toggle
    volatile uint32_t IN;         // 0x20: Input Value
    volatile uint32_t CTRL;       // 0x24: Control
    volatile uint32_t WRCONFIG;   // 0x28: Write Configuration
    uint32_t reserved[1];
    volatile uint8_t  PMUX[16];   // 0x30: Peripheral Multiplexing
    volatile uint8_t  PINCFG[32]; // 0x40: Pin Configuration
};

/// Get PORT group registers for a specific port
inline PortGroup* get_port_group(Port port) {
    // SAMD21 PORT module structure
    // PORT->Group[0] = Port A (0x41004400)
    // PORT->Group[1] = Port B (0x41004480)
    constexpr uint32_t PORT_BASE = 0x41004400;
    constexpr uint32_t PORT_GROUP_SIZE = 0x80;

    uint32_t group_base = PORT_BASE + (static_cast<uint32_t>(port) * PORT_GROUP_SIZE);
    return reinterpret_cast<PortGroup*>(group_base);
}

/// Enable clock for PORT peripheral (SAMD21 has single PORT clock for all ports)
inline void enable_gpio_clock() {
    // Enable PORT clock in PM peripheral (APB Bus Mask)
    // PM->APBBMASK |= PM_APBBMASK_PORT (bit 3)
    constexpr uint32_t PM_BASE = 0x40000400;
    constexpr uint32_t PM_APBBMASK_OFFSET = 0x1C;
    constexpr uint32_t PM_APBBMASK_PORT = (1U << 3);

    volatile uint32_t* apbbmask = reinterpret_cast<volatile uint32_t*>(PM_BASE + PM_APBBMASK_OFFSET);
    *apbbmask |= PM_APBBMASK_PORT;
}

/// Modern C++20 GPIO Pin Implementation for SAMD21
///
/// This class template provides compile-time GPIO pin configuration
/// using the SAMD21 PORT peripheral with atomic operations.
///
/// Features:
/// - Compile-time pin validation using concepts
/// - Zero-cost abstractions (everything inline/constexpr)
/// - Atomic set/clear/toggle operations
/// - Type-safe API
///
/// Template Parameters:
/// - PIN: Pin number (0-based: PA00=0, PA01=1, ..., PA17=17, PB00=32, etc.)
///
/// Example usage:
/// \code
/// // Create LED pin (PA17 on Arduino Zero = pin 17)
/// GpioPin<17> led;
/// led.configure(PinMode::Output);
/// led.set_high();
/// led.toggle();
/// \endcode
template<uint8_t PIN>
class GpioPin {
public:
    // Compile-time constants
    static constexpr uint8_t pin_number = PIN;
    static constexpr Port port = get_port_from_pin(PIN);
    static constexpr uint8_t pin_bit = get_pin_bit(PIN);

    // Compile-time validation
    static_assert(PIN < 64, "Pin number must be < 64 (SAMD21 has 2 ports * 32 pins)");
    static_assert(static_cast<uint8_t>(port) < 2, "GPIO port must be A or B on SAMD21");

    /// Constructor - enables PORT clock automatically
    GpioPin() {
        enable_gpio_clock();
    }

    /// Configure pin mode
    /// @param mode Desired pin mode (Input, Output, Analog, etc.)
    inline void configure(PinMode mode) {
        auto* port_regs = get_port_group(port);
        if (!port_regs) return;

        switch (mode) {
            case PinMode::Input:
                // Set pin direction to input
                port_regs->DIRCLR = (1U << pin_bit);
                // Enable input buffer in PINCFG
                port_regs->PINCFG[pin_bit] |= (1U << 1);  // INEN bit
                break;

            case PinMode::InputPullUp:
                // Set pin direction to input
                port_regs->DIRCLR = (1U << pin_bit);
                // Enable input buffer and pull-up
                port_regs->PINCFG[pin_bit] |= (1U << 1) | (1U << 2);  // INEN | PULLEN
                port_regs->OUTSET = (1U << pin_bit);  // Set = pull-up
                break;

            case PinMode::InputPullDown:
                // Set pin direction to input
                port_regs->DIRCLR = (1U << pin_bit);
                // Enable input buffer and pull-down
                port_regs->PINCFG[pin_bit] |= (1U << 1) | (1U << 2);  // INEN | PULLEN
                port_regs->OUTCLR = (1U << pin_bit);  // Clear = pull-down
                break;

            case PinMode::Output:
                // Set pin direction to output (atomic operation)
                port_regs->DIRSET = (1U << pin_bit);
                // Disable input buffer to save power
                port_regs->PINCFG[pin_bit] &= ~(1U << 1);  // Clear INEN
                break;

            case PinMode::Alternate:
                // Enable peripheral multiplexing
                port_regs->PINCFG[pin_bit] |= (1U << 0);  // PMUXEN bit
                // Note: Caller needs to configure PMUX register separately
                break;

            case PinMode::Analog:
                // Set pin direction to input and disable digital input
                port_regs->DIRCLR = (1U << pin_bit);
                port_regs->PINCFG[pin_bit] &= ~(1U << 1);  // Clear INEN
                break;
        }
    }

    /// Set pin HIGH (using atomic OUTSET register)
    inline void set_high() {
        auto* port_regs = get_port_group(port);
        if (!port_regs) return;
        port_regs->OUTSET = (1U << pin_bit);  // Atomic bit set
    }

    /// Set pin LOW (using atomic OUTCLR register)
    inline void set_low() {
        auto* port_regs = get_port_group(port);
        if (!port_regs) return;
        port_regs->OUTCLR = (1U << pin_bit);  // Atomic bit clear
    }

    /// Toggle pin output state (using atomic OUTTGL register)
    inline void toggle() {
        auto* port_regs = get_port_group(port);
        if (!port_regs) return;
        port_regs->OUTTGL = (1U << pin_bit);  // Atomic bit toggle
    }

    /// Read pin input state
    /// @return true if pin is HIGH, false if LOW
    inline bool read() const {
        auto* port_regs = get_port_group(port);
        if (!port_regs) return false;
        return (port_regs->IN & (1U << pin_bit)) != 0;
    }
};

// Verify that our implementation satisfies the GPIO pin concept
static_assert(alloy::hal::GpioPin<GpioPin<0>>,
              "SAMD21 GpioPin must satisfy the GpioPin concept");

/// Type alias for output pins (more readable)
template<uint8_t PIN>
using OutputPin = GpioPin<PIN>;

/// Type alias for input pins (more readable)
template<uint8_t PIN>
using InputPin = GpioPin<PIN>;

} // namespace alloy::hal::samd21

#endif // ALLOY_HAL_MICROCHIP_SAMD21_GPIO_HPP
