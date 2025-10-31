/// RP2040 GPIO Implementation
///
/// RP2040 uses SIO (Single-cycle IO) for fast GPIO operations
/// Pin numbering: GPIO0=0, GPIO1=1, ..., GPIO29=29 (30 GPIOs total)

#ifndef ALLOY_HAL_RASPBERRYPI_RP2040_GPIO_HPP
#define ALLOY_HAL_RASPBERRYPI_RP2040_GPIO_HPP

#include "hal/interface/gpio.hpp"
#include "generated/raspberrypi/rp2040/rp2040/peripherals.hpp"
#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040 {

// Import generated peripherals
using namespace alloy::generated::rp2040;

/// RP2040 GPIO pin implementation
/// Pin numbering: GPIO0=0, GPIO1=1, ..., GPIO25=25 (LED on Pico)
template<uint8_t PIN>
class GpioPin {
public:
    static constexpr uint8_t pin_number = PIN;

    // RP2040 has 30 GPIO pins (GPIO0-GPIO29)
    // GPIO25 is connected to LED on Raspberry Pi Pico
    static_assert(PIN < 30, "GPIO pin must be 0-29 on RP2040");

    /// Constructor
    GpioPin() = default;

    /// Configure pin mode
    void configure(hal::PinMode mode) {
        if (mode == hal::PinMode::Output) {
            set_mode_output();
        } else if (mode == hal::PinMode::Input) {
            set_mode_input();
        }
    }

    /// Set pin to HIGH
    void set_high() {
        // Use SIO GPIO_OUT_SET for atomic set (single-cycle)
        // SIO base: 0xD0000000
        constexpr uint32_t SIO_BASE = 0xD0000000;
        constexpr uint32_t GPIO_OUT_SET_OFFSET = 0x014;
        volatile uint32_t* gpio_out_set = reinterpret_cast<volatile uint32_t*>(SIO_BASE + GPIO_OUT_SET_OFFSET);
        *gpio_out_set = (1U << PIN);
    }

    /// Set pin to LOW
    void set_low() {
        // Use SIO GPIO_OUT_CLR for atomic clear (single-cycle)
        constexpr uint32_t SIO_BASE = 0xD0000000;
        constexpr uint32_t GPIO_OUT_CLR_OFFSET = 0x018;
        volatile uint32_t* gpio_out_clr = reinterpret_cast<volatile uint32_t*>(SIO_BASE + GPIO_OUT_CLR_OFFSET);
        *gpio_out_clr = (1U << PIN);
    }

    /// Toggle pin
    void toggle() {
        // Use SIO GPIO_OUT_XOR for atomic toggle
        constexpr uint32_t SIO_BASE = 0xD0000000;
        constexpr uint32_t GPIO_OUT_XOR_OFFSET = 0x01C;
        volatile uint32_t* gpio_out_xor = reinterpret_cast<volatile uint32_t*>(SIO_BASE + GPIO_OUT_XOR_OFFSET);
        *gpio_out_xor = (1U << PIN);
    }

    /// Read pin state
    bool read() const {
        // Read from SIO GPIO_IN
        constexpr uint32_t SIO_BASE = 0xD0000000;
        constexpr uint32_t GPIO_IN_OFFSET = 0x004;
        volatile uint32_t* gpio_in = reinterpret_cast<volatile uint32_t*>(SIO_BASE + GPIO_IN_OFFSET);
        return (*gpio_in & (1U << PIN)) != 0;
    }

private:
    /// Set pin mode to output
    static void set_mode_output() {
        // 1. Set function to SIO (GPIO function)
        //    IO_BANK0 CTRL register for each pin
        set_function_sio();

        // 2. Set output enable in SIO
        constexpr uint32_t SIO_BASE = 0xD0000000;
        constexpr uint32_t GPIO_OE_SET_OFFSET = 0x024;
        volatile uint32_t* gpio_oe_set = reinterpret_cast<volatile uint32_t*>(SIO_BASE + GPIO_OE_SET_OFFSET);
        *gpio_oe_set = (1U << PIN);
    }

    /// Set pin mode to input
    static void set_mode_input() {
        // 1. Set function to SIO
        set_function_sio();

        // 2. Clear output enable in SIO
        constexpr uint32_t SIO_BASE = 0xD0000000;
        constexpr uint32_t GPIO_OE_CLR_OFFSET = 0x028;
        volatile uint32_t* gpio_oe_clr = reinterpret_cast<volatile uint32_t*>(SIO_BASE + GPIO_OE_CLR_OFFSET);
        *gpio_oe_clr = (1U << PIN);
    }

    /// Set pin function to SIO (GPIO)
    static void set_function_sio() {
        // IO_BANK0 base: 0x40014000
        // Each GPIO has GPIO_CTRL register at offset (pin * 8) + 4
        constexpr uint32_t IO_BANK0_BASE = 0x40014000;
        constexpr uint32_t GPIO_CTRL_OFFSET = (PIN * 8) + 4;

        volatile uint32_t* gpio_ctrl = reinterpret_cast<volatile uint32_t*>(IO_BANK0_BASE + GPIO_CTRL_OFFSET);

        // Function 5 = SIO (GPIO)
        // FUNCSEL bits [4:0]
        *gpio_ctrl = (*gpio_ctrl & ~0x1F) | 5;
    }
};

// Static assertion to verify concept compliance
static_assert(alloy::hal::GpioPin<GpioPin<0>>,
              "RP2040 GpioPin must satisfy GpioPin concept");

} // namespace alloy::hal::raspberrypi::rp2040

#endif // ALLOY_HAL_RASPBERRYPI_RP2040_GPIO_HPP
