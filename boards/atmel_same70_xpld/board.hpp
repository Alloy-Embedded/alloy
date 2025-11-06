/// ATSAME70 Xplained Board Definition
///
/// Modern C++20 board support for Atmel SAME70 Xplained evaluation board.
/// This file defines board-specific pins and peripherals using generated
/// hardware abstractions (gpio.hpp, pins.hpp, pio_hal.hpp).
///
/// Board: ATSAME70-XPLD
/// MCU:   ATSAME70Q21B (Cortex-M7 @ 300MHz)
/// Features:
///   - 2MB Flash, 384KB SRAM
///   - FPU (double precision)
///   - I-Cache and D-Cache
///   - DSP instructions

#ifndef ALLOY_BOARD_ATSAME70_XPLD_HPP
#define ALLOY_BOARD_ATSAME70_XPLD_HPP

#include "../../src/hal/vendors/atmel/same70/atsame70q21b/gpio.hpp"
#include "../../src/hal/vendors/atmel/same70/atsame70q21b/pins.hpp"
#include "../../src/hal/vendors/atmel/same70/atsame70q21b/register_map.hpp"
#include <stdint.h>

namespace Board {

/// Board identification
inline constexpr const char* name = "ATSAME70 Xplained";
inline constexpr const char* mcu = "ATSAME70Q21B";

/// Clock configuration
inline constexpr uint32_t system_clock_hz = 300'000'000;  // 300 MHz (max)
inline constexpr uint32_t xtal_frequency_hz = 12'000'000; // 12 MHz crystal

//
// Pre-defined GPIO Pins
//
// Pin naming follows SAME70 conventions:
// - PA0-PA31 (Port A)
// - PB0-PB31 (Port B)
// - PC0-PC31 (Port C)
// - PD0-PD31 (Port D)
//

namespace detail {
    /// Calculate pin number: Port * 32 + Pin
    /// PA0  = 0*32 + 0  = 0
    /// PB12 = 1*32 + 12 = 44
    /// PC8  = 2*32 + 8  = 72

    // LED (User LED - active LOW)
    // The SAME70 Xplained has one user LED on PC8
    inline constexpr uint8_t LED_USER_PIN = 72;  // PC8 (active LOW)

    // Button (User button SW0 - active LOW)
    inline constexpr uint8_t BUTTON_SW0_PIN = 9;  // PA9 (active LOW)

    // Debug UART (UART0 - connected to EDBG)
    inline constexpr uint8_t UART0_TX_PIN = 10;  // PA10 (URXD0)
    inline constexpr uint8_t UART0_RX_PIN = 9;   // PA9  (UTXD0)

    // I2C0 pins (TWIHS0)
    inline constexpr uint8_t I2C0_SCL_PIN = 4;   // PA4 (TWCK0)
    inline constexpr uint8_t I2C0_SDA_PIN = 3;   // PA3 (TWD0)

    // SPI0 pins
    inline constexpr uint8_t SPI0_MISO_PIN = 12; // PA12 (MISO)
    inline constexpr uint8_t SPI0_MOSI_PIN = 13; // PA13 (MOSI)
    inline constexpr uint8_t SPI0_SCK_PIN = 14;  // PA14 (SPCK)
    inline constexpr uint8_t SPI0_CS0_PIN = 11;  // PA11 (NPCS0)

    // Ethernet (EMAC) pins
    inline constexpr uint8_t ETH_MDIO_PIN = 18;  // PA18 (EMDIO)
    inline constexpr uint8_t ETH_MDC_PIN = 19;   // PA19 (EMDC)
    inline constexpr uint8_t ETH_RXDV_PIN = 15;  // PA15 (ERXDV)
    inline constexpr uint8_t ETH_RXD0_PIN = 16;  // PA16 (ERX0)
    inline constexpr uint8_t ETH_RXD1_PIN = 17;  // PA17 (ERX1)
    inline constexpr uint8_t ETH_TXEN_PIN = 20;  // PA20 (ETXEN)
    inline constexpr uint8_t ETH_TXD0_PIN = 21;  // PA21 (ETX0)
    inline constexpr uint8_t ETH_TXD1_PIN = 22;  // PA22 (ETX1)
}

//
// LED Control
//
// Note: The user LED on SAME70 Xplained is active LOW
//

namespace Led {
    using namespace alloy::hal::atmel::same70::atsame70q21b;

    // User LED on PC8 (active LOW)
    using LedPin = GPIOPin<pins::PC8>;

    /// Initialize LED pin as output
    inline void init() {
        // Note: PIOC clock is enabled in Board::initialize()
        // Configure PC8 as output using generated GPIO abstraction
        LedPin::configureOutput();

        // LED is active LOW, so set HIGH to turn off initially
        LedPin::set();
    }

    /// Turn LED on (active LOW - set pin LOW)
    inline void on() {
        LedPin::clear();
    }

    /// Turn LED off (active LOW - set pin HIGH)
    inline void off() {
        LedPin::set();
    }

    /// Toggle LED state
    inline void toggle() {
        LedPin::toggle();
    }
}

//
// Button Control
//

namespace Button {
    using namespace alloy::hal::atmel::same70::atsame70q21b;

    // User button SW0 on PA9 (active LOW)
    using ButtonPin = GPIOPin<pins::PA9>;

    /// Initialize button pin as input with pull-up
    inline void init() {
        // Note: PIOA clock is enabled in Board::initialize()
        // Configure PA9 as input with pull-up using generated GPIO abstraction
        ButtonPin::configureInputPullUp();
    }

    /// Read button state (returns true if pressed)
    /// Note: SW0 is active LOW, so pressed = LOW
    inline bool is_pressed() {
        return !ButtonPin::read();  // Pressed when LOW
    }
}

//
// Helper Functions
//

/// Initialize board (clocks, essential peripherals)
inline void initialize() {
    // Enable peripheral clocks for GPIO ports via PMC
    // PMC (Power Management Controller) controls peripheral clocks
    volatile uint32_t* PMC_PCER0 = reinterpret_cast<volatile uint32_t*>(0x400E0610);

    // Enable clocks for all GPIO ports (PIOA, PIOB, PIOC, PIOD, PIOE)
    constexpr uint32_t ID_PIOA = 10;
    constexpr uint32_t ID_PIOB = 11;
    constexpr uint32_t ID_PIOC = 12;
    constexpr uint32_t ID_PIOD = 16;
    constexpr uint32_t ID_PIOE = 17;

    *PMC_PCER0 = (1U << ID_PIOA) | (1U << ID_PIOB) | (1U << ID_PIOC) |
                 (1U << ID_PIOD) | (1U << ID_PIOE);

    // Small delay to let clocks stabilize
    for (volatile int i = 0; i < 10000; i++) {
        __asm__ volatile("nop");
    }

    // Enable ODSR (Output Data Status Register) access for all ports
    // This allows direct read/write to ODSR register
    // OWER offset = 0x00A0 from PIO base
    volatile uint32_t* PIOA_OWER = reinterpret_cast<volatile uint32_t*>(0x400E0EA0);
    volatile uint32_t* PIOB_OWER = reinterpret_cast<volatile uint32_t*>(0x400E10A0);
    volatile uint32_t* PIOC_OWER = reinterpret_cast<volatile uint32_t*>(0x400E12A0);
    volatile uint32_t* PIOD_OWER = reinterpret_cast<volatile uint32_t*>(0x400E14A0);
    volatile uint32_t* PIOE_OWER = reinterpret_cast<volatile uint32_t*>(0x400E16A0);

    *PIOA_OWER = 0xFFFFFFFF;
    *PIOB_OWER = 0xFFFFFFFF;
    *PIOC_OWER = 0xFFFFFFFF;
    *PIOD_OWER = 0xFFFFFFFF;
    *PIOE_OWER = 0xFFFFFFFF;

    // TODO: Configure system clock to 300MHz using PLL
    // TODO: Configure flash wait states
}

/// Simple delay (busy wait - not accurate, for basic use only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(uint32_t ms) {
    // MCU starts with 12MHz RC oscillator (not 300MHz PLL!)
    // At 12MHz, approximately 12,000 cycles per millisecond
    // Rough approximation: 3 cycles per loop iteration
    // So: 12,000 / 3 = 4000 iterations per ms
    volatile uint32_t count = ms * 4000;
    while (count--) {
        __asm__ volatile("nop");
    }
}

/// More accurate microsecond delay
/// @param us Microseconds to delay
inline void delay_us(uint32_t us) {
    // At 12MHz, approximately 12 cycles per microsecond
    // Rough approximation: 3 cycles per loop iteration
    // So: 12 / 3 = 4 iterations per us
    volatile uint32_t count = us * 4;
    while (count--) {
        __asm__ volatile("nop");
    }
}

//
// Board Information
//

namespace Info {
    /// Get board name
    inline constexpr const char* get_name() { return name; }

    /// Get MCU name
    inline constexpr const char* get_mcu() { return mcu; }

    /// Get system clock frequency
    inline constexpr uint32_t get_clock_hz() { return system_clock_hz; }

    /// Get crystal frequency
    inline constexpr uint32_t get_xtal_hz() { return xtal_frequency_hz; }
}

} // namespace Board

#endif // ALLOY_BOARD_ATSAME70_XPLD_HPP
