/**
 * @file spi_multi_level_api_demo.cpp
 * @brief Comprehensive demonstration of SPI multi-level API
 *
 * This example showcases all three levels of the SPI API:
 * - Level 1 (Simple): One-liner setup with defaults
 * - Level 2 (Fluent): Builder pattern with method chaining
 * - Level 3 (Expert): Full configuration control with validation
 *
 * Use Cases Demonstrated:
 * 1. Simple SPI setup for common devices (Level 1)
 * 2. Customizable SPI with fluent API (Level 2)
 * 3. Advanced SPI with all parameters (Level 3)
 * 4. SPI with DMA integration
 * 5. TX-only SPI for output devices
 *
 * Hardware Setup:
 * - SPI0: Full-duplex communication (e.g., SD card, EEPROM)
 * - SPI1: TX-only for display (e.g., LCD, OLED)
 * - CS pins: GPIO controlled (not hardware NSS)
 *
 * @note Part of Phase 6.2: SPI Implementation
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
 */

#include <span>

#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/spi.hpp"
#include "hal/signals.hpp"
#include "hal/spi_dma.hpp"
#include "hal/spi_expert.hpp"
#include "hal/spi_fluent.hpp"
#include "hal/spi_simple.hpp"

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::signals;

// ============================================================================
// Mock GPIO Pin Definitions (replace with actual board pins)
// ============================================================================

// Mock Pin types for demonstration
template <PinId pin_id>
struct MockPin {
    static constexpr PinId get_pin_id() { return pin_id; }
};

// SPI0 Pins (for full-duplex communication)
using Spi0_MOSI = MockPin<PinId::PA7>;
using Spi0_MISO = MockPin<PinId::PA6>;
using Spi0_SCK = MockPin<PinId::PA5>;

// SPI1 Pins (for TX-only communication)
using Spi1_MOSI = MockPin<PinId::PB15>;
using Spi1_SCK = MockPin<PinId::PB13>;

// ============================================================================
// Mock SPI Device (for demonstration)
// ============================================================================

/**
 * @brief Mock SPI device for testing
 *
 * In a real implementation, this would interact with hardware registers.
 */
template <PeripheralId PeriphId>
class MockSpiDevice {
public:
    Result<void, ErrorCode> transfer(std::span<const u8> tx_buffer,
                                      std::span<u8> rx_buffer) {
        // Mock implementation - just copy data
        usize transfer_size = tx_buffer.size() < rx_buffer.size() ? tx_buffer.size()
                                                                    : rx_buffer.size();

        for (usize i = 0; i < transfer_size; i++) {
            rx_buffer[i] = tx_buffer[i] ^ 0xAA;  // Mock response
        }

        return Ok();
    }

    Result<void, ErrorCode> transmit(std::span<const u8> tx_buffer) {
        // Mock implementation - discard data
        (void)tx_buffer;
        return Ok();
    }

    Result<void, ErrorCode> receive(std::span<u8> rx_buffer) {
        // Mock implementation - fill with dummy data
        for (auto& byte : rx_buffer) {
            byte = 0x55;
        }
        return Ok();
    }

    Result<void, ErrorCode> configure(const SpiConfig& config) {
        (void)config;
        // Mock implementation - just store config
        return Ok();
    }

    bool is_busy() const { return false; }
};

// ============================================================================
// Example 1: Simple API - One-liner setup
// ============================================================================

void example_simple_api() {
    // Simplest SPI setup - just specify pins and speed
    constexpr auto spi_config =
        Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI, Spi0_MISO, Spi0_SCK>(
            2000000  // 2 MHz
        );

    // Validation happens at compile-time via static_assert in quick_setup

    // Create device and transfer data
    MockSpiDevice<PeripheralId::SPI0> spi_device;

    u8 tx_data[] = {0x01, 0x02, 0x03, 0x04};
    u8 rx_data[4] = {0};

    auto result = spi_device.transfer(std::span(tx_data), std::span(rx_data));

    if (result.is_ok()) {
        // Data transferred successfully
    }
}

// ============================================================================
// Example 2: Simple API - TX-only for output devices
// ============================================================================

void example_simple_tx_only() {
    // TX-only SPI for displays, DACs, etc.
    constexpr auto spi_config =
        Spi<PeripheralId::SPI1>::quick_setup_master_tx<Spi1_MOSI, Spi1_SCK>(
            8000000,         // 8 MHz for fast display updates
            SpiMode::Mode0   // Mode 0 for most displays
        );

    // Validation happens at compile-time via static_assert in quick_setup_master_tx

    // Send data to display
    MockSpiDevice<PeripheralId::SPI1> spi_device;

    u8 display_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    auto result = spi_device.transmit(std::span(display_data));
}

// ============================================================================
// Example 3: Fluent API - Builder pattern
// ============================================================================

void example_fluent_api() {
    // Build SPI configuration step by step
    auto spi_result = SpiBuilder<PeripheralId::SPI0>()
                          .with_mosi<Spi0_MOSI>()
                          .with_miso<Spi0_MISO>()
                          .with_sck<Spi0_SCK>()
                          .clock_speed(4000000)  // 4 MHz
                          .mode(SpiMode::Mode3)  // CPOL=1, CPHA=1
                          .msb_first()
                          .data_8bit()
                          .initialize();

    if (spi_result.is_ok()) {
        auto config = spi_result.unwrap();
        // Use configuration
        config.apply();
    }
}

// ============================================================================
// Example 4: Fluent API - Preset configurations
// ============================================================================

void example_fluent_presets() {
    // Use preset Mode 0 configuration
    auto spi_mode0 = SpiBuilder<PeripheralId::SPI0>()
                         .with_mosi<Spi0_MOSI>()
                         .with_miso<Spi0_MISO>()
                         .with_sck<Spi0_SCK>()
                         .clock_speed(2000000)
                         .standard_mode0()  // Sets Mode0, MSB first, 8-bit
                         .initialize();

    // Use preset Mode 3 configuration
    auto spi_mode3 = SpiBuilder<PeripheralId::SPI0>()
                         .with_mosi<Spi0_MOSI>()
                         .with_miso<Spi0_MISO>()
                         .with_sck<Spi0_SCK>()
                         .clock_speed(2000000)
                         .standard_mode3()  // Sets Mode3, MSB first, 8-bit
                         .initialize();
}

// ============================================================================
// Example 5: Expert API - Full configuration control
// ============================================================================

void example_expert_api() {
    // Create expert configuration with all parameters
    constexpr SpiExpertConfig config = {
        .peripheral = PeripheralId::SPI0,
        .mosi_pin = PinId::PA7,
        .miso_pin = PinId::PA6,
        .sck_pin = PinId::PA5,
        .nss_pin = PinId::PA4,  // Hardware NSS (optional)
        .mode = SpiMode::Mode0,
        .clock_speed = 10000000,  // 10 MHz
        .bit_order = SpiBitOrder::MsbFirst,
        .data_size = SpiDataSize::Bits8,
        .enable_mosi = true,
        .enable_miso = true,
        .enable_nss = false,  // Use software CS instead
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = false,
        .enable_motorola = true};

    // Validate at compile-time
    static_assert(config.is_valid(), "Invalid SPI config");

    // Apply configuration
    auto result = expert::configure(config);
}

// ============================================================================
// Example 6: Expert API - Preset configurations
// ============================================================================

void example_expert_presets() {
    // Standard Mode 0 configuration
    constexpr auto mode0_config = SpiExpertConfig::standard_mode0_2mhz(
        PeripheralId::SPI0, PinId::PA7, PinId::PA6, PinId::PA5);

    // TX-only configuration for output devices
    constexpr auto tx_only_config =
        SpiExpertConfig::tx_only_config(PeripheralId::SPI1, PinId::PB15, PinId::PB13,
                                        8000000);

    // High-speed DMA configuration
    constexpr auto dma_config = SpiExpertConfig::dma_config(
        PeripheralId::SPI0, PinId::PA7, PinId::PA6, PinId::PA5, 10000000);

    // Validate all configurations
    static_assert(mode0_config.is_valid(), "Invalid mode0 config");
    static_assert(tx_only_config.is_valid(), "Invalid TX-only config");
    static_assert(dma_config.is_valid(), "Invalid DMA config");
}

// ============================================================================
// Example 7: Expert API - Compile-time validation
// ============================================================================

void example_expert_validation() {
    // This configuration is invalid (no MOSI or MISO enabled)
    constexpr SpiExpertConfig invalid_config = {
        .peripheral = PeripheralId::SPI0,
        .mosi_pin = PinId::PA7,
        .miso_pin = PinId::PA6,
        .sck_pin = PinId::PA5,
        .nss_pin = PinId::PA4,
        .mode = SpiMode::Mode0,
        .clock_speed = 2000000,
        .bit_order = SpiBitOrder::MsbFirst,
        .data_size = SpiDataSize::Bits8,
        .enable_mosi = false,  // Neither enabled!
        .enable_miso = false,
        .enable_nss = false,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_crc = false,
        .crc_polynomial = 0,
        .enable_ti_mode = false,
        .enable_motorola = true};

    // This will fail at compile-time with descriptive error
    // static_assert(invalid_config.is_valid(), invalid_config.error_message());

    // Check validation at runtime
    if (!invalid_config.is_valid()) {
        // Error: Must enable at least MOSI or MISO
    }
}

// ============================================================================
// Example 8: DMA Integration - Type-safe DMA channels
// ============================================================================

void example_spi_dma() {
    // Define DMA connections (platform-specific)
    // These would be actual DMA channels on real hardware
    using Spi0TxDma =
        DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;
    using Spi0RxDma =
        DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_RX, DmaStream::Stream2>;

    // Create SPI DMA configuration
    constexpr auto config = SpiDmaConfig<Spi0TxDma, Spi0RxDma>::create(
        PinId::PA7,   // MOSI
        PinId::PA6,   // MISO
        PinId::PA5,   // SCK
        10000000,     // 10 MHz
        SpiMode::Mode0);

    // Validate at compile-time
    static_assert(config.is_valid(), "Invalid SPI config");

    // Transfer data with DMA
    u8 tx_buffer[256] = {0};
    u8 rx_buffer[256] = {0};

    auto result = spi_dma_transfer<Spi0TxDma, Spi0RxDma>(tx_buffer, rx_buffer, 256);
}

// ============================================================================
// Example 9: DMA Integration - Preset configurations
// ============================================================================

void example_spi_dma_presets() {
    using Spi0TxDma =
        DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;
    using Spi0RxDma =
        DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_RX, DmaStream::Stream2>;

    // Full-duplex SPI with DMA
    constexpr auto full_duplex = create_spi_full_duplex_dma<Spi0TxDma, Spi0RxDma>(
        PinId::PA7,   // MOSI
        PinId::PA6,   // MISO
        PinId::PA5,   // SCK
        8000000       // 8 MHz
    );

    // TX-only SPI with DMA (for displays)
    constexpr auto tx_only = create_spi_tx_only_dma<Spi0TxDma>(
        PinId::PB15,  // MOSI
        PinId::PB13,  // SCK
        8000000       // 8 MHz
    );

    // High-speed SPI with DMA
    constexpr auto high_speed = create_spi_high_speed_dma<Spi0TxDma, Spi0RxDma>(
        PinId::PA7,   // MOSI
        PinId::PA6,   // MISO
        PinId::PA5,   // SCK
        20000000      // 20 MHz
    );

    static_assert(full_duplex.is_valid(), "Full-duplex DMA config invalid");
    static_assert(tx_only.is_valid(), "TX-only DMA config invalid");
    static_assert(high_speed.is_valid(), "High-speed DMA config invalid");
}

// ============================================================================
// Example 10: Real-world use case - SD Card communication
// ============================================================================

void example_sd_card_spi() {
    // SD card typically uses Mode 0, 8-bit, MSB first
    // Start slow for initialization, then speed up
    constexpr auto slow_init = Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI,
                                                                     Spi0_MISO, Spi0_SCK>(
        400000,  // 400 kHz for initialization
        SpiMode::Mode0);

    constexpr auto fast_transfer =
        Spi<PeripheralId::SPI0>::quick_setup<Spi0_MOSI, Spi0_MISO, Spi0_SCK>(
            25000000,  // 25 MHz for data transfer
            SpiMode::Mode0);

    MockSpiDevice<PeripheralId::SPI0> spi_device;

    // Initialize SD card at low speed
    u8 init_cmd[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};  // CMD0
    u8 response[1];
    spi_device.transfer(std::span(init_cmd), std::span(response));

    // After init, switch to high speed for data transfers
    u8 data_block[512];
    spi_device.receive(std::span(data_block));
}

// ============================================================================
// Example 11: Real-world use case - SPI Flash memory
// ============================================================================

void example_spi_flash() {
    // SPI Flash typically uses high-speed SPI with DMA for bulk transfers
    using FlashTxDma =
        DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_TX, DmaStream::Stream3>;
    using FlashRxDma =
        DmaConnection<PeripheralId::SPI0, DmaRequest::SPI0_RX, DmaStream::Stream2>;

    constexpr auto flash_config = create_spi_high_speed_dma<FlashTxDma, FlashRxDma>(
        PinId::PA7,   // MOSI
        PinId::PA6,   // MISO
        PinId::PA5,   // SCK
        50000000      // 50 MHz for fast flash
    );

    // Read flash data with DMA
    u8 read_cmd[] = {0x03, 0x00, 0x00, 0x00};  // READ command
    u8 flash_data[4096];

    // First send command
    spi_dma_transmit<FlashTxDma>(read_cmd, sizeof(read_cmd));

    // Then receive data
    spi_dma_receive<FlashRxDma>(flash_data, sizeof(flash_data));
}

// ============================================================================
// Example 12: Helper functions - Using SPI with RAII chip select
// ============================================================================

// NOTE: This example requires GPIO implementation
// Uncomment when GPIO is available on the target platform

// template <typename CsPin>
// void example_with_chip_select() {
//     MockSpiDevice<PeripheralId::SPI0> spi_device;
//
//     u8 tx_data[] = {0x01, 0x02, 0x03};
//     u8 rx_data[3];
//
//     // RAII chip select - automatically manages CS pin
//     {
//         SpiChipSelect<CsPin> cs(/* cs_pin */);  // CS goes low
//
//         // Transfer data while CS is active
//         spi_device.transfer(std::span(tx_data), std::span(rx_data));
//
//         // CS goes high automatically when scope exits
//     }
// }

// ============================================================================
// Example 13: Helper functions - Single byte transfer
// ============================================================================

void example_byte_transfers() {
    MockSpiDevice<PeripheralId::SPI0> spi_device;

    // Transfer single byte (full-duplex)
    auto tx_result = spi_transfer_byte(spi_device, 0xAA);
    if (tx_result.is_ok()) {
        u8 received = tx_result.unwrap();
        // Process received byte
    }

    // Write single byte (TX-only)
    auto write_result = spi_write_byte(spi_device, 0xBB);

    // Read single byte (RX-only)
    auto read_result = spi_read_byte(spi_device);
    if (read_result.is_ok()) {
        u8 received = read_result.unwrap();
    }
}

// ============================================================================
// Main function
// ============================================================================

int main() {
    // Run all examples
    example_simple_api();
    example_simple_tx_only();
    example_fluent_api();
    example_fluent_presets();
    example_expert_api();
    example_expert_presets();
    example_expert_validation();
    example_spi_dma();
    example_spi_dma_presets();
    example_sd_card_spi();
    example_spi_flash();
    example_byte_transfers();

    return 0;
}
