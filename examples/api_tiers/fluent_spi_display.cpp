/**
 * @file fluent_spi_display.cpp
 * @brief Level 2 Fluent API - SPI Display Driver Example
 *
 * Demonstrates the Fluent tier SPI API with builder pattern for custom configuration.
 * Perfect for devices requiring specific SPI modes and timing.
 *
 * This example shows:
 * - Builder pattern for custom SPI configuration
 * - SPI Mode3 (CPOL=1, CPHA=1) for displays
 * - Higher clock speed (8 MHz) for fast updates
 * - TX-only optimization for write-heavy operations
 *
 * Hardware Setup:
 * - SPI MOSI -> Display SDI/MOSI
 * - SPI SCK  -> Display SCK
 * - SPI CS   -> Display CS (manual control)
 * - DC pin   -> Display D/C (Data/Command select)
 * - RST pin  -> Display Reset (optional)
 *
 * Expected Behavior:
 * - Initializes SPI display (e.g., ST7735, ILI9341)
 * - Sends initialization commands
 * - Fills screen with color pattern
 * - Demonstrates fast pixel writes
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
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;   // D13 (MISO not needed for display)
using SpiCs = Pin<PeripheralId::GPIO, PinId::PB6>;    // D10
using DisplayDc = Pin<PeripheralId::GPIO, PinId::PC7>; // D9 (Data/Command)

#elif defined(PLATFORM_STM32F722ZE)
#include "boards/nucleo_f722ze/board.hpp"
using namespace board::nucleo_f722ze;

// SPI1 pins (Arduino header)
using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;   // D11
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;    // D13
using SpiCs = Pin<PeripheralId::GPIO, PinId::PD14>;    // D10
using DisplayDc = Pin<PeripheralId::GPIO, PinId::PD15>; // D9

#elif defined(PLATFORM_STM32G071RB)
#include "boards/nucleo_g071rb/board.hpp"
using namespace board::nucleo_g071rb;

// SPI1 pins (Arduino header)
using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;  // D11
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;   // D13
using SpiCs = Pin<PeripheralId::GPIO, PinId::PB1>;    // D10
using DisplayDc = Pin<PeripheralId::GPIO, PinId::PA9>; // D9

#elif defined(PLATFORM_STM32G0B1RE)
#include "boards/nucleo_g0b1re/board.hpp"
using namespace board::nucleo_g0b1re;

// SPI1 pins (Arduino header)
using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;  // D11
using SpiSck = Pin<PeripheralId::SPI1, PinId::PA5>;   // D13
using SpiCs = Pin<PeripheralId::GPIO, PinId::PB1>;    // D10
using DisplayDc = Pin<PeripheralId::GPIO, PinId::PA9>; // D9

#elif defined(PLATFORM_SAME70)
#include "boards/same70_xplained/board.hpp"
using namespace board::same70_xplained;

// SPI0 pins
using SpiMosi = Pin<PeripheralId::SPI0, PinId::PD21>;  // MOSI
using SpiSck = Pin<PeripheralId::SPI0, PinId::PD22>;   // SCK
using SpiCs = Pin<PeripheralId::GPIO, PinId::PD25>;    // CS
using DisplayDc = Pin<PeripheralId::GPIO, PinId::PD26>; // D/C

#else
#error "Unsupported platform. Please define one of the supported platforms."
#endif

#include "hal/gpio.hpp"

using namespace ucore;
using namespace ucore::hal;

// ============================================================================
// Display Helper Functions
// ============================================================================

/**
 * @brief Send command to display
 *
 * @param cs Chip select pin
 * @param dc Data/Command pin
 * @param spi SPI instance
 * @param cmd Command byte
 */
template <typename CsPin, typename DcPin, typename Spi>
void send_command(CsPin& cs, DcPin& dc, Spi& spi, uint8_t cmd) {
    dc.clear();  // Command mode
    cs.clear();  // Select display
    spi.transmit_byte(cmd);
    cs.set();    // Deselect display
}

/**
 * @brief Send data to display
 *
 * @param cs Chip select pin
 * @param dc Data/Command pin
 * @param spi SPI instance
 * @param data Data byte
 */
template <typename CsPin, typename DcPin, typename Spi>
void send_data(CsPin& cs, DcPin& dc, Spi& spi, uint8_t data) {
    dc.set();    // Data mode
    cs.clear();  // Select display
    spi.transmit_byte(data);
    cs.set();    // Deselect display
}

/**
 * @brief Send data buffer to display (fast bulk write)
 *
 * @param cs Chip select pin
 * @param dc Data/Command pin
 * @param spi SPI instance
 * @param buffer Data buffer
 * @param length Buffer length
 */
template <typename CsPin, typename DcPin, typename Spi>
void send_data_buffer(CsPin& cs, DcPin& dc, Spi& spi, const uint8_t* buffer, size_t length) {
    dc.set();    // Data mode
    cs.clear();  // Select display
    spi.transmit(buffer, length);  // Bulk write for performance
    cs.set();    // Deselect display
}

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Fluent SPI display driver example
 *
 * Demonstrates builder pattern for custom SPI configuration.
 */
int main() {
    // ========================================================================
    // Setup with Fluent Builder Pattern
    // ========================================================================

    // Fluent API: Configure SPI with custom settings for display
    auto spi_builder = SpiBuilderInstance()
        .with_pins<SpiMosi, SpiSck>()        // TX-only (no MISO needed)
        .clock_speed(8000000)                 // 8 MHz for fast updates
        .mode(SpiMode::Mode3)                 // CPOL=1, CPHA=1 (common for displays)
        .bit_order(SpiBitOrder::MsbFirst)     // MSB first (standard)
        .data_size(SpiDataSize::Bits8);       // 8-bit transfers

    // Initialize SPI with configuration
    auto spi = spi_builder.initialize();

    // Setup control pins
    auto cs_pin = Gpio<SpiCs>::output();
    auto dc_pin = Gpio<DisplayDc>::output();
    auto led = LedPin::output();

    cs_pin.set();  // CS high (inactive)
    dc_pin.set();  // Data mode by default

    // ========================================================================
    // Display Initialization Sequence (example for ST7735)
    // ========================================================================

    // Software reset
    send_command(cs_pin, dc_pin, spi, 0x01);
    for (volatile uint32_t i = 0; i < 1000000; ++i);  // Delay

    // Sleep out
    send_command(cs_pin, dc_pin, spi, 0x11);
    for (volatile uint32_t i = 0; i < 1000000; ++i);  // Delay

    // Color mode: 16-bit color (RGB565)
    send_command(cs_pin, dc_pin, spi, 0x3A);
    send_data(cs_pin, dc_pin, spi, 0x05);  // 16-bit/pixel

    // Display on
    send_command(cs_pin, dc_pin, spi, 0x29);

    led.toggle();  // Initialization complete

    // ========================================================================
    // Fill Screen with Color Pattern
    // ========================================================================

    // Set column address (0 to 127 for 128px width)
    send_command(cs_pin, dc_pin, spi, 0x2A);
    send_data(cs_pin, dc_pin, spi, 0x00);
    send_data(cs_pin, dc_pin, spi, 0x00);
    send_data(cs_pin, dc_pin, spi, 0x00);
    send_data(cs_pin, dc_pin, spi, 0x7F);

    // Set row address (0 to 159 for 160px height)
    send_command(cs_pin, dc_pin, spi, 0x2B);
    send_data(cs_pin, dc_pin, spi, 0x00);
    send_data(cs_pin, dc_pin, spi, 0x00);
    send_data(cs_pin, dc_pin, spi, 0x00);
    send_data(cs_pin, dc_pin, spi, 0x9F);

    // Write to RAM
    send_command(cs_pin, dc_pin, spi, 0x2C);

    // Fill screen with red (RGB565 = 0xF800)
    uint8_t pixel_buffer[256];
    for (size_t i = 0; i < 256; i += 2) {
        pixel_buffer[i] = 0xF8;    // High byte (red)
        pixel_buffer[i + 1] = 0x00; // Low byte
    }

    // Send 128x160 pixels = 20,480 pixels = 40,960 bytes
    // Using 256-byte buffer chunks for efficiency
    for (uint32_t i = 0; i < 160; ++i) {
        send_data_buffer(cs_pin, dc_pin, spi, pixel_buffer, 256);
    }

    led.toggle();  // Screen filled

    // ========================================================================
    // Continuous Update Loop
    // ========================================================================

    while (true) {
        // Cycle through colors (Red -> Green -> Blue)
        uint16_t colors[] = {0xF800, 0x07E0, 0x001F};  // RGB565

        for (uint16_t color : colors) {
            // Prepare pixel buffer with current color
            for (size_t i = 0; i < 256; i += 2) {
                pixel_buffer[i] = (color >> 8) & 0xFF;      // High byte
                pixel_buffer[i + 1] = color & 0xFF;         // Low byte
            }

            // Set write position (top-left corner)
            send_command(cs_pin, dc_pin, spi, 0x2C);

            // Fill screen
            for (uint32_t i = 0; i < 160; ++i) {
                send_data_buffer(cs_pin, dc_pin, spi, pixel_buffer, 256);
            }

            led.toggle();

            // Delay
            for (volatile uint32_t i = 0; i < 5000000; ++i);
        }
    }

    return 0;
}

/**
 * Key Points:
 *
 * 1. Fluent builder: Method chaining for readable configuration
 * 2. Custom SPI mode: Mode3 for display compatibility
 * 3. Higher clock speed: 8 MHz for fast pixel updates
 * 4. TX-only mode: No MISO needed for write-only displays
 * 5. Bulk transfers: Buffer writes for performance
 *
 * Why Mode3 for Displays?
 * - Many LCD controllers (ST7735, ILI9341) use Mode3
 * - CPOL=1: Clock idles high
 * - CPHA=1: Sample on falling edge
 * - Check your display's datasheet!
 *
 * Performance Optimization:
 * - 8 MHz clock: 8 million bits/sec = 1 MB/sec
 * - 128x160 RGB565 display = 40,960 bytes
 * - Theoretical fill time: ~41ms @ 8 MHz
 * - Actual: ~50-60ms (with overhead)
 * - Buffer writes: 10-20x faster than byte-by-byte
 *
 * Common Display Commands:
 * - 0x01: Software reset
 * - 0x11: Sleep out
 * - 0x29: Display on
 * - 0x2A: Set column address
 * - 0x2B: Set row address
 * - 0x2C: Write to RAM
 * - 0x3A: Color mode (RGB565, RGB666, etc.)
 */
