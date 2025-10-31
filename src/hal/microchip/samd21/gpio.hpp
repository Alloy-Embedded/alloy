/// ATSAMD21 GPIO Implementation
///
/// SAMD21 uses PORT module for GPIO
/// Pin numbering: PA00=0, PA01=1, ..., PA31=31, PB00=32, PB01=33, etc.

#ifndef ALLOY_HAL_MICROCHIP_SAMD21_GPIO_HPP
#define ALLOY_HAL_MICROCHIP_SAMD21_GPIO_HPP

#include "hal/interface/gpio.hpp"
#include "generated/microchip_technology_inc/atsamd21j18a/atsamd21j18a/peripherals.hpp"
#include <cstdint>

namespace alloy::hal::microchip::samd21 {

// Import generated peripherals
using namespace alloy::generated::atsamd21j18a;

/// GPIO port identifiers
enum class Port : uint8_t {
    A = 0,
    B = 1
};

/// Extract port and pin from pin number
/// SAMD21 uses format: Port * 32 + Pin
/// PA00=0, PA01=1, ..., PA31=31, PB00=32, PB01=33, etc.
constexpr inline Port get_port(uint8_t pin) {
    return static_cast<Port>(pin / 32);
}

constexpr inline uint8_t get_pin_number(uint8_t pin) {
    return pin % 32;
}

/// ATSAMD21 GPIO pin implementation
/// Pin numbering: PA00=0, ..., PA17=17 (LED on Arduino Zero), etc.
template<uint8_t PIN>
class GpioPin {
public:
    static constexpr uint8_t pin_number = PIN;
    static constexpr Port port = get_port(PIN);
    static constexpr uint8_t pin_bit = get_pin_number(PIN);

    // SAMD21 has 2 ports: A and B (64 pins max)
    static_assert(PIN < 64, "GPIO pin must be less than 64 on SAMD21");

    /// Constructor
    GpioPin() = default;

    /// Configure pin mode
    void configure(hal::PinMode mode) {
        // Enable port clock (if not already enabled)
        // PM->APBBMASK |= PM_APBBMASK_PORT

        if (mode == hal::PinMode::Output) {
            set_mode_output();
        } else if (mode == hal::PinMode::Input) {
            set_mode_input();
        }
    }

    /// Set pin to HIGH
    void set_high() {
        // Use OUTSET register - atomic set
        auto port_regs = get_port_registers();
        port_regs->OUTSET = (1U << pin_bit);
    }

    /// Set pin to LOW
    void set_low() {
        // Use OUTCLR register - atomic clear
        auto port_regs = get_port_registers();
        port_regs->OUTCLR = (1U << pin_bit);
    }

    /// Toggle pin
    void toggle() {
        // Use OUTTGL register - atomic toggle
        auto port_regs = get_port_registers();
        port_regs->OUTTGL = (1U << pin_bit);
    }

    /// Read pin state
    bool read() const {
        auto port_regs = get_port_registers();
        return (port_regs->IN & (1U << pin_bit)) != 0;
    }

private:
    /// Set pin mode to output
    static void set_mode_output() {
        auto port_regs = get_port_registers();

        // Set pin direction to output in DIR register
        port_regs->DIRSET = (1U << pin_bit);

        // Disable input buffer (PINCFG)
        // For minimal implementation, skip PINCFG configuration
    }

    /// Set pin mode to input
    static void set_mode_input() {
        auto port_regs = get_port_registers();

        // Set pin direction to input in DIR register
        port_regs->DIRCLR = (1U << pin_bit);

        // Enable input buffer would require PINCFG configuration
    }

    /// Get pointer to PORT registers for this pin
    static auto get_port_registers() {
        // SAMD21 PORT module structure
        // PORT->Group[0] = Port A (0x41004400)
        // PORT->Group[1] = Port B (0x41004480)
        constexpr uint32_t PORT_BASE = 0x41004400;
        constexpr uint32_t PORT_GROUP_SIZE = 0x80;

        uint32_t group_base = PORT_BASE + (static_cast<uint32_t>(port) * PORT_GROUP_SIZE);

        // PORT Group registers structure
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

        return reinterpret_cast<PortGroup*>(group_base);
    }
};

// Static assertion to verify concept compliance
static_assert(alloy::hal::GpioPin<GpioPin<0>>,
              "SAMD21 GpioPin must satisfy GpioPin concept");

} // namespace alloy::hal::microchip::samd21

#endif // ALLOY_HAL_MICROCHIP_SAMD21_GPIO_HPP
