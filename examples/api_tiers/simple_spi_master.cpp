/**
 * @file simple_spi_master.cpp
 * @brief Level 1 Simple API - SPI Master Example
 *
 * Demonstrates the Simple tier SPI API with a one-liner setup.
 * Perfect for quick prototyping and straightforward SPI communication.
 *
 * This example shows:
 * - One-liner SPI setup with defaults (Mode0, 1 MHz)
 * - Full-duplex SPI transfer
 * - Reading from an SPI device (e.g., accelerometer, sensor)
 *
 * Hardware Setup:
 * - SPI MOSI -> Connect to device's SDI/MOSI
 * - SPI MISO -> Connect to device's SDO/MISO
 * - SPI SCK  -> Connect to device's SCK
 * - SPI CS   -> Connect to device's CS/NSS (manual control)
 *
 * Expected Behavior:
 * - Reads WHO_AM_I register from device
 * - Prints device ID via UART (if available)
 * - Blinks LED on successful read
 *
 * @note Part of Phase 3.3: SPI Implementation
 * @see docs/API_TIERS.md
 */

// ============================================================================
// Board-Specific Configuration
// ============================================================================

#if defined(PLATFORM_STM32F401RE)
#include "boards/nucleo_f401re/board.hpp"
using namespace board::nucleo_f401re;

// SPI1 pins (Arduino header)
using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;  // D11
using SpiMiso = Pin<PeripheralId::SPI1, PinId::PA6>;  // D12
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;   // D13
using SpiCs = Pin<PeripheralId::GPIO, PinId::PB6>;    // D10 (manual CS)

#elif defined(PLATFORM_STM32F722ZE)
#include "boards/nucleo_f722ze/board.hpp"
using namespace board::nucleo_f722ze;

// SPI1 pins (Arduino header)
using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;  // D11
using SpiMiso = Pin<PeripheralId::SPI1, PinId::PA6>;  // D12
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;   // D13
using SpiCs = Pin<PeripheralId::GPIO, PinId::PD14>;   // D10 (manual CS)

#elif defined(PLATFORM_STM32G071RB)
#include "boards/nucleo_g071rb/board.hpp"
using namespace board::nucleo_g071rb;

// SPI1 pins (Arduino header)
using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;  // D11
using SpiMiso = Pin<PeripheralId::SPI1, PinId::PA6>;  // D12
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;   // D13
using SpiCs = Pin<PeripheralId::GPIO, PinId::PB1>;    // D10 (manual CS)

#elif defined(PLATFORM_STM32G0B1RE)
#include "boards/nucleo_g0b1re/board.hpp"
using namespace board::nucleo_g0b1re;

// SPI1 pins (Arduino header)
using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;  // D11
using SpiMiso = Pin<PeripheralId::SPI1, PinId::PA6>;  // D12
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;   // D13
using SpiCs = Pin<PeripheralId::GPIO, PinId::PB1>;    // D10 (manual CS)

#elif defined(PLATFORM_SAME70)
#include "boards/same70_xplained/board.hpp"
using namespace board::same70_xplained;

// SPI0 pins
using SpiMosi = Pin<PeripheralId::SPI0, PinId::PD21>;  // MOSI
using SpiMiso = Pin<PeripheralId::SPI0, PinId::PD20>;  // MISO
using SpiSck = Pin<PeripheralId::SPI0, PinId::PD22>;   // SCK
using SpiCs = Pin<PeripheralId::GPIO, PinId::PD25>;    // CS (manual)

#else
#error "Unsupported platform. Please define one of the supported platforms."
#endif

#include "hal/gpio.hpp"

using namespace ucore;
using namespace ucore::hal;

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Simple SPI master example
 *
 * Demonstrates one-liner SPI setup and basic communication.
 */
int main() {
    // ========================================================================
    // Setup
    // ========================================================================

    // Simple API: One-liner setup with defaults (Mode0, 1 MHz, 8-bit, MSB first)
    auto spi = SpiInstance::quick_setup<SpiMosi, SpiMiso, SpiSck>(
        1000000  // 1 MHz clock speed (safe default for most devices)
    );

    // Initialize SPI peripheral
    spi.initialize();

    // Setup CS pin (manual control - active low)
    auto cs_pin = Gpio<SpiCs>::output();
    cs_pin.set();  // CS high (inactive)

    // Setup LED for visual feedback
    auto led = LedPin::output();

    // ========================================================================
    // Read Device ID Example (e.g., WHO_AM_I register)
    // ========================================================================

    // Example: Read WHO_AM_I register from device (0x75)
    // This is a common pattern for SPI sensors and accelerometers

    uint8_t who_am_i_reg = 0x75;  // WHO_AM_I register address (read bit set)
    uint8_t device_id = 0x00;     // Buffer for device ID

    // CS low (select device)
    cs_pin.clear();

    // Send register address (with read bit)
    spi.transmit_byte(who_am_i_reg | 0x80);  // 0x80 sets read bit

    // Read device ID byte
    device_id = spi.receive_byte();

    // CS high (deselect device)
    cs_pin.set();

    // ========================================================================
    // Full-Duplex Transfer Example
    // ========================================================================

    // Buffer for sending and receiving
    uint8_t tx_buffer[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_buffer[4] = {0x00, 0x00, 0x00, 0x00};

    // CS low (select device)
    cs_pin.clear();

    // Full-duplex transfer (send and receive simultaneously)
    auto result = spi.transfer(tx_buffer, rx_buffer, 4);

    // CS high (deselect device)
    cs_pin.set();

    // Check transfer result
    if (result.is_ok()) {
        // Success! Blink LED
        led.toggle();
    }

    // ========================================================================
    // Continuous Operation
    // ========================================================================

    while (true) {
        // CS low
        cs_pin.clear();

        // Read single byte
        uint8_t data = spi.receive_byte();

        // CS high
        cs_pin.set();

        // Blink LED
        led.toggle();

        // Small delay (busy wait)
        for (volatile uint32_t i = 0; i < 1000000; ++i)
            ;
    }

    return 0;
}

/**
 * Key Points:
 *
 * 1. One-liner setup: quick_setup() handles all configuration
 * 2. Default settings: Mode0, 1 MHz, 8-bit, MSB first
 * 3. Manual CS control: More flexible for multi-device buses
 * 4. Full-duplex capable: Transfer sends and receives simultaneously
 * 5. Error handling: Result<T, ErrorCode> pattern for robustness
 *
 * Common SPI Modes:
 * - Mode0 (CPOL=0, CPHA=0): Most common, clock idles low, sample on rising edge
 * - Mode1 (CPOL=0, CPHA=1): Clock idles low, sample on falling edge
 * - Mode2 (CPOL=1, CPHA=0): Clock idles high, sample on falling edge
 * - Mode3 (CPOL=1, CPHA=1): Clock idles high, sample on rising edge
 *
 * Typical Clock Speeds:
 * - 1 MHz:   Safe default for most devices
 * - 2-4 MHz: Common for SD cards, displays
 * - 8-10 MHz: High-speed sensors, accelerometers
 * - 16+ MHz: Fast memory, high-performance devices
 */
