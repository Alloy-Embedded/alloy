/**
 * @file board.hpp
 * @brief Board Support Package for Atmel SAME70 Xplained Ultra
 *
 * This file provides a complete board support package including:
 * - Clock configuration with multiple frequency options
 * - All available peripherals (UART, I2C, SPI, ADC, PWM, Timer, DMA)
 * - LEDs and buttons mapped to board-specific pins
 * - Easy-to-use initialization functions
 *
 * SAME70 Xplained Ultra Specifications:
 * - MCU: ATSAME70Q21B (ARM Cortex-M7 @ 300MHz max)
 * - Flash: 2MB
 * - RAM: 384KB
 * - LEDs: 2x user LEDs (LED0: PC8, LED1: PC9)
 * - Buttons: 2x user buttons (SW0: PA11, SW1: PC2)
 * - USB: Full-speed USB device + host
 * - Ethernet: 10/100 Mbps
 * - Camera interface, SD card, etc.
 *
 * Usage Example:
 * @code
 * #include "boards/same70_xplained/board.hpp"
 *
 * using namespace alloy::boards::same70_xplained;
 *
 * int main() {
 *     // Initialize board with 300MHz clock
 *     board::init(ClockConfig::Clock300MHz);
 *
 *     // Use LEDs
 *     board::led0.set();     // Turn on LED0
 *     board::led1.toggle();  // Toggle LED1
 *
 *     // Use buttons
 *     if (board::sw0.read()) {
 *         // Button pressed
 *     }
 *
 *     // Use peripherals
 *     board::uart0.write("Hello, World!\n");
 *     uint16_t adc_value = board::adc0.read_channel(0);
 *
 *     while (true) {
 *         board::led0.toggle();
 *         board::delay_ms(500);
 *     }
 * }
 * @endcode
 *
 * @note Part of Alloy Framework Board Support
 */

#pragma once

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// Platform-specific HAL implementations
#include "hal/platform/same70/adc.hpp"
#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/dma.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/i2c.hpp"
#include "hal/platform/same70/pwm.hpp"
#include "hal/platform/same70/spi.hpp"
#include "hal/platform/same70/timer.hpp"

// Vendor-specific register definitions
#include "hal/vendors/atmel/same70/atsame70q21/peripherals.hpp"
#include "hal/vendors/atmel/same70/atsame70q21/register_map.hpp"

namespace alloy::boards::same70_xplained {

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::same70;

// ============================================================================
// Clock Configuration Options
// ============================================================================

/**
 * @brief Clock configuration presets for SAME70
 *
 * The SAME70 can run at multiple frequencies. Choose based on your needs:
 * - Clock300MHz: Maximum performance (300 MHz)
 * - Clock150MHz: Balanced performance/power (150 MHz)
 * - Clock120MHz: Compatible with USB (120 MHz)
 * - Clock48MHz: Low power operation (48 MHz)
 * - Clock12MHz: Ultra-low power (12 MHz from RC oscillator)
 */
enum class ClockConfig {
    Clock300MHz,  ///< 300 MHz - Maximum performance
    Clock150MHz,  ///< 150 MHz - Balanced
    Clock120MHz,  ///< 120 MHz - USB compatible
    Clock48MHz,   ///< 48 MHz  - Low power
    Clock12MHz    ///< 12 MHz  - Ultra-low power (RC oscillator)
};

// ============================================================================
// Board-Specific Pin Definitions
// ============================================================================

// Port base addresses (from vendor headers)
constexpr uint32_t PIOA_BASE = 0x400E0E00;
constexpr uint32_t PIOB_BASE = 0x400E1000;
constexpr uint32_t PIOC_BASE = 0x400E1200;
constexpr uint32_t PIOD_BASE = 0x400E1400;
constexpr uint32_t PIOE_BASE = 0x400E1600;

// -----------------------------------------------------------------------------
// LEDs (Active Low)
// -----------------------------------------------------------------------------

/// LED0 (Green) - PC8
using Led0Pin = GpioPin<PIOC_BASE, 8>;

/// LED1 (Green) - PC9
using Led1Pin = GpioPin<PIOC_BASE, 9>;

// -----------------------------------------------------------------------------
// User Buttons (Active Low - with pull-up)
// -----------------------------------------------------------------------------

/// SW0 (User button 0) - PA11
using Sw0Pin = GpioPin<PIOA_BASE, 11>;

/// SW1 (User button 1) - PC2
using Sw1Pin = GpioPin<PIOB_BASE, 9>;

// -----------------------------------------------------------------------------
// UART/USART Pins
// -----------------------------------------------------------------------------

// UART0 (Debug console via USB)
using Uart0TxPin = GpioPin<PIOA_BASE, 10>;  // URXD0
using Uart0RxPin = GpioPin<PIOA_BASE, 9>;   // UTXD0

// USART0
using Usart0TxPin = GpioPin<PIOB_BASE, 1>;  // TXD0
using Usart0RxPin = GpioPin<PIOB_BASE, 0>;  // RXD0

// USART1
using Usart1TxPin = GpioPin<PIOA_BASE, 22>; // TXD1
using Usart1RxPin = GpioPin<PIOA_BASE, 21>; // RXD1

// -----------------------------------------------------------------------------
// I2C Pins
// -----------------------------------------------------------------------------

// TWIHS0 (Two-Wire Interface High-Speed 0)
using I2c0SclPin = GpioPin<PIOA_BASE, 4>;   // TWCK0
using I2c0SdaPin = GpioPin<PIOA_BASE, 3>;   // TWD0

// TWIHS1
using I2c1SclPin = GpioPin<PIOB_BASE, 5>;   // TWCK1
using I2c1SdaPin = GpioPin<PIOB_BASE, 4>;   // TWD1

// TWIHS2
using I2c2SclPin = GpioPin<PIOD_BASE, 28>;  // TWCK2
using I2c2SdaPin = GpioPin<PIOD_BASE, 27>;  // TWD2

// -----------------------------------------------------------------------------
// SPI Pins
// -----------------------------------------------------------------------------

// SPI0
using Spi0MisoPin = GpioPin<PIOD_BASE, 20>; // MISO
using Spi0MosiPin = GpioPin<PIOD_BASE, 21>; // MOSI
using Spi0SckPin  = GpioPin<PIOD_BASE, 22>; // SPCK
using Spi0Cs0Pin  = GpioPin<PIOB_BASE, 2>;  // NPCS0

// SPI1
using Spi1MisoPin = GpioPin<PIOC_BASE, 26>; // MISO
using Spi1MosiPin = GpioPin<PIOC_BASE, 27>; // MOSI
using Spi1SckPin  = GpioPin<PIOC_BASE, 24>; // SPCK
using Spi1Cs0Pin  = GpioPin<PIOC_BASE, 25>; // NPCS0

// -----------------------------------------------------------------------------
// ADC Pins
// -----------------------------------------------------------------------------

// ADC Channel pins (some ADC channels)
using Adc0Pin = GpioPin<PIOD_BASE, 30>;  // AD0
using Adc1Pin = GpioPin<PIOA_BASE, 19>;  // AD1
using Adc5Pin = GpioPin<PIOC_BASE, 13>;  // AD5

// -----------------------------------------------------------------------------
// PWM Pins
// -----------------------------------------------------------------------------

using Pwm0Pin = GpioPin<PIOA_BASE, 0>;   // PWMC0_PWMH0
using Pwm1Pin = GpioPin<PIOA_BASE, 19>;  // PWMC0_PWMH1
using Pwm2Pin = GpioPin<PIOA_BASE, 13>;  // PWMC0_PWMH2
using Pwm3Pin = GpioPin<PIOC_BASE, 19>;  // PWMC0_PWMH3

// ============================================================================
// Board Namespace
// ============================================================================

namespace board {

// ============================================================================
// Global Board Resources (Pre-defined instances)
// ============================================================================

// -----------------------------------------------------------------------------
// LEDs (Active Low - clear() = ON, set() = OFF)
// -----------------------------------------------------------------------------

inline Led0Pin led0{};
inline Led1Pin led1{};

// Helper functions for intuitive LED control
inline void led0_on()  { led0.clear(); }
inline void led0_off() { led0.set(); }
inline void led1_on()  { led1.clear(); }
inline void led1_off() { led1.set(); }

// -----------------------------------------------------------------------------
// Buttons (Active Low - read() returns false when pressed)
// -----------------------------------------------------------------------------

inline Sw0Pin sw0{};
inline Sw1Pin sw1{};

// Helper functions for intuitive button reading (returns false if read fails)
inline bool sw0_pressed() {
    auto result = sw0.read();
    return result.is_ok() ? !result.unwrap() : false;
}
inline bool sw1_pressed() {
    auto result = sw1.read();
    return result.is_ok() ? !result.unwrap() : false;
}

// -----------------------------------------------------------------------------
// UART/USART Instances
// -----------------------------------------------------------------------------

// UART0 - Debug console (connected to EDBG USB-UART)
inline Uart<0> uart0{};

// USART instances
inline Usart<0> usart0{};
inline Usart<1> usart1{};
inline Usart<2> usart2{};

// -----------------------------------------------------------------------------
// I2C Instances
// -----------------------------------------------------------------------------

inline I2c<0> i2c0{};  // TWIHS0
inline I2c<1> i2c1{};  // TWIHS1
inline I2c<2> i2c2{};  // TWIHS2

// -----------------------------------------------------------------------------
// SPI Instances
// -----------------------------------------------------------------------------

inline Spi<0> spi0{};
inline Spi<1> spi1{};

// -----------------------------------------------------------------------------
// ADC Instances
// -----------------------------------------------------------------------------

inline Adc<0> adc0{};  // 12-bit ADC with 12 channels

// -----------------------------------------------------------------------------
// PWM Instances
// -----------------------------------------------------------------------------

inline Pwm<0> pwm0{};
inline Pwm<1> pwm1{};
inline Pwm<2> pwm2{};
inline Pwm<3> pwm3{};

// -----------------------------------------------------------------------------
// Timer Instances
// -----------------------------------------------------------------------------

inline Timer<0> timer0{};
inline Timer<1> timer1{};
inline Timer<2> timer2{};

// -----------------------------------------------------------------------------
// DMA Instances
// -----------------------------------------------------------------------------

inline Dma<0> dma0{};

// ============================================================================
// Board Initialization
// ============================================================================

/**
 * @brief Initialize board with specified clock configuration
 *
 * This function performs complete board initialization:
 * 1. Configures system clock to the specified frequency
 * 2. Initializes all LEDs as outputs (OFF by default)
 * 3. Initializes all buttons as inputs with pull-ups
 * 4. Enables peripheral clocks for all peripherals
 *
 * @param clock_config Desired clock configuration
 * @return Result indicating success or error code
 *
 * @note Call this function before using any board peripherals
 */
inline Result<void, ErrorCode> init(ClockConfig clock_config = ClockConfig::Clock300MHz) {
    // -------------------------------------------------------------------------
    // 1. Configure System Clock
    // -------------------------------------------------------------------------

    ClockFrequency target_freq;

    switch (clock_config) {
        case ClockConfig::Clock300MHz:
            target_freq = ClockFrequency::Freq300MHz;
            break;
        case ClockConfig::Clock150MHz:
            target_freq = ClockFrequency::Freq150MHz;
            break;
        case ClockConfig::Clock120MHz:
            target_freq = ClockFrequency::Freq120MHz;
            break;
        case ClockConfig::Clock48MHz:
            target_freq = ClockFrequency::Freq48MHz;
            break;
        case ClockConfig::Clock12MHz:
            target_freq = ClockFrequency::Freq12MHz;
            break;
        default:
            return Err(ErrorCode::InvalidParameter);
    }

    // Initialize system clock
    auto clock_result = Clock::init(target_freq);
    if (clock_result.is_error()) {
        return Err(clock_result.error());
    }

    // -------------------------------------------------------------------------
    // 2. Initialize LEDs (outputs, active low - OFF by default)
    // -------------------------------------------------------------------------

    // LED0 - PC8
    auto led0_result = led0.setDirection(PinDirection::Output);
    if (led0_result.is_error()) {
        return Err(led0_result.error());
    }
    led0.set();  // OFF (active low)

    // LED1 - PC9
    auto led1_result = led1.setDirection(PinDirection::Output);
    if (led1_result.is_error()) {
        return Err(led1_result.error());
    }
    led1.set();  // OFF (active low)

    // -------------------------------------------------------------------------
    // 3. Initialize Buttons (inputs with pull-ups)
    // -------------------------------------------------------------------------

    // SW0 - PA11 (with pull-up, active low)
    auto sw0_result = sw0.setDirection(PinDirection::Input);
    if (sw0_result.is_error()) {
        return Err(sw0_result.error());
    }
    sw0.setPull(PinPull::PullUp);

    // SW1 - PB9 (with pull-up, active low)
    auto sw1_result = sw1.setDirection(PinDirection::Input);
    if (sw1_result.is_error()) {
        return Err(sw1_result.error());
    }
    sw1.setPull(PinPull::PullUp);

    // -------------------------------------------------------------------------
    // 4. Enable Peripheral Clocks (done automatically by Clock::init)
    // -------------------------------------------------------------------------

    return Ok();
}

/**
 * @brief Simple delay in milliseconds
 *
 * @param ms Delay duration in milliseconds
 *
 * @note This is a blocking delay using a simple loop.
 *       For precise timing or non-blocking delays, use Timer peripherals.
 */
inline void delay_ms(uint32_t ms) {
    // Simple delay loop (assumes 300 MHz clock)
    // This is approximate and should be calibrated for production use
    constexpr uint32_t cycles_per_ms = 300000 / 4;  // Rough estimate
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t j = 0; j < cycles_per_ms; j++) {
            // Busy wait
        }
    }
}

/**
 * @brief Simple delay in microseconds
 *
 * @param us Delay duration in microseconds
 *
 * @note This is a blocking delay. For precise timing, use Timer peripherals.
 */
inline void delay_us(uint32_t us) {
    // Simple delay loop (assumes 300 MHz clock)
    constexpr uint32_t cycles_per_us = 300 / 4;  // Rough estimate
    for (uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < cycles_per_us; j++) {
            // Busy wait
        }
    }
}

/**
 * @brief Get current system clock frequency in Hz
 *
 * @return System clock frequency in Hz
 */
inline uint32_t get_system_clock_hz() {
    return Clock::getSystemClockFrequency();
}

/**
 * @brief Get board name string
 *
 * @return Board name as string
 */
inline const char* get_board_name() {
    return "Atmel SAME70 Xplained Ultra";
}

/**
 * @brief Get MCU name string
 *
 * @return MCU name as string
 */
inline const char* get_mcu_name() {
    return "ATSAME70Q21B";
}

} // namespace board

} // namespace alloy::boards::same70_xplained
