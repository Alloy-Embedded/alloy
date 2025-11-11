/**
 * @file board_config.hpp
 * @brief SAME70 Xplained Ultra Board Configuration
 *
 * This file contains the board-specific implementation of the standard
 * board interface for the SAME70 Xplained Ultra evaluation board.
 *
 * Hardware Specifications:
 * - MCU: ATSAME70Q21B (ARM Cortex-M7 @ 300MHz max)
 * - Flash: 2MB
 * - RAM: 384KB
 * - LEDs: 2x user LEDs (LED0: PC8, LED1: PC9) - active-low
 * - Buttons: 2x user buttons (SW0: PA11, SW1: PC2) - active-low
 * - USB: Full-speed USB device + host
 * - Ethernet: 10/100 Mbps
 * - Debug: EDBG with virtual COM port
 *
 * @note Part of Alloy Framework Board Support
 */

#pragma once

#include "../common/board_interface.hpp"

// Platform-specific HAL implementations
#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/systick.hpp"
#include "hal/platform/same70/systick_delay.hpp"
#include "hal/platform/same70/uart.hpp"
#include "hal/platform/same70/spi.hpp"
#include "hal/platform/same70/i2c.hpp"
#include "hal/platform/same70/timer.hpp"
#include "hal/platform/same70/pwm.hpp"
#include "hal/platform/same70/adc.hpp"
#include "hal/platform/same70/dma.hpp"

// Vendor-specific register definitions
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::boards::same70_xplained {

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::same70;

// ============================================================================
// Peripheral Type Aliases (OpenSpec REQ-TP-008)
// ============================================================================

/**
 * @brief Board-level peripheral type aliases
 *
 * These aliases provide convenient access to SAME70 peripherals without
 * needing to specify template parameters. They follow the OpenSpec
 * template-based peripheral pattern for zero-overhead abstraction.
 *
 * All peripherals use compile-time base addresses and IRQ IDs from the
 * generated peripheral definitions.
 *
 * Example usage:
 * @code
 * #include "boards/same70_xplained/board.hpp"
 * using namespace alloy::boards::same70_xplained;
 *
 * int main() {
 *     board::init();
 *
 *     // UART
 *     Uart0 uart;
 *     uart.initialize(uart_config);
 *     uart.write('H');
 *
 *     // SPI
 *     Spi0 spi;
 *     spi.open();
 *     spi.transfer(tx_data, rx_data, 3, SpiChipSelect::CS0);
 *
 *     // I2C
 *     I2c0 i2c;
 *     i2c.open();
 *     i2c.write(0x50, data, 4);
 * }
 * @endcode
 */

// Import vendor peripheral addresses and IDs
using namespace alloy::generated::atsame70q21b;

// UART peripherals (USART)
using Uart0 = same70::Uart<peripherals::USART0, id::USART0>;
using Uart1 = same70::Uart<peripherals::USART1, id::USART1>;
using Uart2 = same70::Uart<peripherals::USART2, id::USART2>;

// SPI peripherals
using Spi0 = same70::Spi<peripherals::SPI0, id::SPI0>;
using Spi1 = same70::Spi<peripherals::SPI1, id::SPI1>;

// I2C peripherals (TWIHS - Two-Wire Interface High Speed)
using I2c0 = same70::I2c<peripherals::TWIHS0, id::TWIHS0>;
using I2c1 = same70::I2c<peripherals::TWIHS1, id::TWIHS1>;
using I2c2 = same70::I2c<peripherals::TWIHS2, id::TWIHS2>;

// Timer Counter peripherals
using Timer0 = same70::Timer<peripherals::TC0, id::TC0>;
using Timer1 = same70::Timer<peripherals::TC1, id::TC1>;
using Timer2 = same70::Timer<peripherals::TC2, id::TC2>;
using Timer3 = same70::Timer<peripherals::TC3, id::TC3>;

// PWM peripherals
using Pwm0 = same70::Pwm<peripherals::PWM0, id::PWM0>;
using Pwm1 = same70::Pwm<peripherals::PWM1, id::PWM1>;

// ADC peripherals (AFEC - Analog Front-End Controller)
using Adc0 = same70::Adc<peripherals::AFEC0, id::AFEC0>;
using Adc1 = same70::Adc<peripherals::AFEC1, id::AFEC1>;

// DMA peripheral (XDMAC - eXtensible DMA Controller)
using Dma = same70::Dma<peripherals::XDMAC, id::XDMAC>;

// GPIO ports (already defined in detail namespace, but exposed here for consistency)
using GpioA = peripherals::PIOA;
using GpioB = peripherals::PIOB;
using GpioC = peripherals::PIOC;
using GpioD = peripherals::PIOD;
using GpioE = peripherals::PIOE;

// Convenience GPIO pin aliases for common board features
namespace pins {
    // LEDs (active-low)
    using Led0 = GpioPin<peripherals::PIOC, 8>;   // PC8
    using Led1 = GpioPin<peripherals::PIOC, 9>;   // PC9

    // Buttons (active-low with pull-up)
    using Button0 = GpioPin<peripherals::PIOA, 11>;  // PA11
    using Button1 = GpioPin<peripherals::PIOC, 2>;   // PC2

    // UART0 pins (commonly used on Arduino headers)
    using Uart0_Tx = GpioPin<peripherals::PIOA, 10>;  // PA10 - URXD0
    using Uart0_Rx = GpioPin<peripherals::PIOA, 9>;   // PA9 - UTXD0

    // SPI0 pins
    using Spi0_Miso = GpioPin<peripherals::PIOD, 20>;  // PD20 - MISO
    using Spi0_Mosi = GpioPin<peripherals::PIOD, 21>;  // PD21 - MOSI
    using Spi0_Sck  = GpioPin<peripherals::PIOD, 22>;  // PD22 - SCK
    using Spi0_Cs0  = GpioPin<peripherals::PIOB, 2>;   // PB2 - NPCS0

    // I2C0 pins (TWIHS0)
    using I2c0_Sda = GpioPin<peripherals::PIOA, 3>;   // PA3 - TWD0
    using I2c0_Scl = GpioPin<peripherals::PIOA, 4>;   // PA4 - TWCK0
}

// ============================================================================
// Board Identification
// ============================================================================

namespace board {

inline constexpr const char* name() {
    return "SAME70 Xplained Ultra";
}

inline constexpr const char* mcu() {
    return "ATSAME70Q21B";
}

inline constexpr uint32_t clock_frequency_hz() {
    return 300'000'000;  // 300 MHz when using default Clock300MHz preset
}

// ============================================================================
// Clock Configuration Presets
// ============================================================================

/**
 * @brief Clock configuration presets for SAME70
 *
 * Choose based on your application needs:
 * - Clock300MHz: Maximum performance (300 MHz core, 150 MHz bus)
 * - Clock150MHz: Balanced performance/power (150 MHz core, 75 MHz bus)
 * - Clock120MHz: USB-compatible (120 MHz core, 60 MHz bus)
 * - Clock48MHz:  Low power operation (48 MHz core, 24 MHz bus)
 * - Clock12MHz:  Ultra-low power (12 MHz RC oscillator)
 */
enum class ClockPreset {
    Clock300MHz,  ///< 300 MHz - Maximum performance
    Clock150MHz,  ///< 150 MHz - Balanced
    Clock120MHz,  ///< 120 MHz - USB compatible
    Clock48MHz,   ///< 48 MHz  - Low power
    Clock12MHz    ///< 12 MHz  - Ultra-low power (RC oscillator)
};

// ============================================================================
// Internal Definitions
// ============================================================================

namespace detail {
    using namespace alloy::generated::atsame70q21b;

    // LED pins (active-low: clear()=ON, set()=OFF)
    using Led0Pin = GpioPin<PIOC_BASE, 8>;
    using Led1Pin = GpioPin<PIOC_BASE, 9>;

    // Button pins (active-low with internal pull-up)
    using Button0Pin = GpioPin<PIOA_BASE, 11>;
    using Button1Pin = GpioPin<PIOC_BASE, 2>;

    // Global instances (initialized in init())
    inline Led0Pin led0_instance;
    inline Led1Pin led1_instance;
    inline Button0Pin button0_instance;
    inline Button1Pin button1_instance;

    // Initialization flag (defined in board_config.cpp)
    extern bool initialized;
}

// ============================================================================
// Board Initialization
// ============================================================================

/**
 * @brief Initialize the SAME70 Xplained Ultra board
 *
 * This function:
 * 1. Configures system clock based on preset
 * 2. Initializes SysTick timer for delays
 * 3. Enables peripheral clocks for GPIO ports
 * 4. Configures LEDs as outputs (OFF by default)
 * 5. Configures buttons as inputs with pull-ups
 *
 * @param preset Clock configuration preset (default: 300MHz)
 *
 * After calling this function, you can use:
 * - board::delay_ms() / board::delay_us() - Precise delays
 * - board::micros() - Microsecond timestamp
 * - board::led::on/off/toggle() - LED control
 * - board::button::read() - Button input
 *
 * Example:
 * @code
 * board::init();                           // Use default 300MHz
 * board::init(ClockPreset::Clock12MHz);    // Use 12MHz RC oscillator
 *
 * while (true) {
 *     board::led::toggle();
 *     board::delay_ms(500);  // Precise 500ms delay
 * }
 * @endcode
 *
 * @note Implementation is in board_config.cpp
 */
void init(ClockPreset preset = ClockPreset::Clock300MHz);

// ============================================================================
// Delay Functions
// ============================================================================

inline void delay_ms(uint32_t ms) {
    alloy::hal::same70::delay_ms(ms);
}

inline void delay_us(uint32_t us) {
    alloy::hal::same70::delay_us(us);
}

/**
 * @brief Get microsecond timestamp since boot
 * @return Microseconds elapsed since system initialization
 */
inline uint32_t micros() {
    return alloy::hal::same70::SystemTick::micros();
}

// ============================================================================
// LED Interface
// ============================================================================

namespace led {
    /**
     * @brief Turn LED0 on
     * @note LED0 is active-low, so we clear the pin
     */
    inline void on() {
        detail::led0_instance.clear();
    }

    /**
     * @brief Turn LED0 off
     * @note LED0 is active-low, so we set the pin
     */
    inline void off() {
        detail::led0_instance.set();
    }

    /**
     * @brief Toggle LED0 state
     */
    inline void toggle() {
        detail::led0_instance.toggle();
    }

    /**
     * @brief Set LED0 to specific state
     * @param state true = ON, false = OFF
     */
    inline void set(bool state) {
        if (state) {
            on();
        } else {
            off();
        }
    }

    /**
     * @namespace led1
     * @brief Second LED control (LED1 / PC9)
     */
    namespace led1 {
        inline void on() { detail::led1_instance.clear(); }
        inline void off() { detail::led1_instance.set(); }
        inline void toggle() { detail::led1_instance.toggle(); }
        inline void set(bool state) {
            if (state) {
                on();
            } else {
                off();
            }
        }
    }
}

// ============================================================================
// Button Interface
// ============================================================================

namespace button {
    /**
     * @brief Read button SW0 state
     * @return true if button is pressed, false otherwise
     * @note Button is active-low, so we invert the reading
     */
    inline bool read() {
        return !detail::button0_instance.read();  // active-low
    }

    /**
     * @brief Wait for button SW0 press
     * @note Blocking function with debouncing
     */
    inline void wait_press() {
        while (!read()) {}  // Wait for press
        delay_ms(20);       // Debounce
    }

    /**
     * @brief Wait for button SW0 release
     * @note Blocking function with debouncing
     */
    inline void wait_release() {
        while (read()) {}   // Wait for release
        delay_ms(20);       // Debounce
    }

    /**
     * @namespace button1
     * @brief Second button control (SW1 / PC2)
     */
    namespace button1 {
        inline bool read() {
            return !detail::button1_instance.read();  // active-low
        }

        inline void wait_press() {
            while (!read()) {}
            delay_ms(20);
        }

        inline void wait_release() {
            while (read()) {}
            delay_ms(20);
        }
    }
}

} // namespace board

// Define feature flags
#define BOARD_HAS_LED
#define BOARD_HAS_BUTTON

} // namespace alloy::boards::same70_xplained

// Note: SysTick_Handler is implemented in board_config.cpp
