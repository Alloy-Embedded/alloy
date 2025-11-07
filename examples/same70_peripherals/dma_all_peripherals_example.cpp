/**
 * @file dma_all_peripherals_example.cpp
 * @brief Comprehensive DMA example for all SAME70 peripherals
 *
 * This example demonstrates DMA usage with:
 * - UART: DMA-based serial transmission/reception
 * - SPI: High-speed SPI flash operations with DMA
 * - I2C: DMA-based I2C EEPROM access
 * - ADC: High-speed continuous ADC sampling
 *
 * Shows how DMA eliminates CPU overhead and enables true zero-copy I/O.
 *
 * Hardware Setup:
 * - UART: TX/RX pins configured
 * - SPI: Flash chip on SPI0
 * - I2C: EEPROM on TWIHS0
 * - ADC: Analog signal on AD0
 *
 * @note This demonstrates the universal DMA capability - one DMA controller
 *       working seamlessly with ALL peripherals using hardware handshaking.
 */

#include "hal/platform/same70/dma.hpp"
#include "hal/platform/same70/adc.hpp"
#include "hal/platform/same70/spi.hpp"
#include "hal/platform/same70/i2c.hpp"

using namespace alloy::hal::same70;
using namespace alloy::hal;

// ============================================================================
// Example 1: UART with DMA - Serial Communication
// ============================================================================

/**
 * @brief UART transmit using DMA (zero CPU overhead)
 *
 * Use case: Sending large data logs, debug output, or protocol data
 * without blocking the CPU.
 */
class UartDmaTx {
public:
    auto init() -> alloy::core::Result<void> {
        // Note: In real implementation, you would also initialize UART here
        // For this example, we focus on DMA configuration

        auto dma_result = m_dma.open();
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        // Configure DMA for Memory -> UART TX
        DmaConfig config;
        config.direction = DmaDirection::MemoryToPeripheral;
        config.src_width = DmaWidth::Byte;
        config.dst_width = DmaWidth::Byte;
        config.peripheral = DmaPeripheralId::USART0_TX;
        config.src_increment = true;   // Read sequential memory
        config.dst_increment = false;  // Write to same UART register

        return m_dma.configure(config);
    }

    /**
     * @brief Send data via UART using DMA
     */
    auto send(const char* data, size_t length) -> alloy::core::Result<void> {
        // UART TX register address (example for USART0)
        volatile uint32_t* uart_thr = reinterpret_cast<volatile uint32_t*>(0x4001C01C);

        return m_dma.transfer(data, uart_thr, length);
    }

    auto waitComplete() -> alloy::core::Result<void> {
        return m_dma.waitComplete();
    }

    auto close() -> alloy::core::Result<void> {
        return m_dma.close();
    }

private:
    DmaUartTx m_dma;
};

/**
 * @brief UART receive using DMA
 *
 * Use case: Receiving data packets, GPS NMEA sentences, or protocol frames
 * without polling or interrupts.
 */
class UartDmaRx {
public:
    auto init() -> alloy::core::Result<void> {
        auto dma_result = m_dma.open();
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        // Configure DMA for UART RX -> Memory
        DmaConfig config;
        config.direction = DmaDirection::PeripheralToMemory;
        config.src_width = DmaWidth::Byte;
        config.dst_width = DmaWidth::Byte;
        config.peripheral = DmaPeripheralId::USART0_RX;
        config.src_increment = false;  // Read from same UART register
        config.dst_increment = true;   // Write to sequential memory

        return m_dma.configure(config);
    }

    /**
     * @brief Receive data via UART using DMA
     */
    auto receive(uint8_t* buffer, size_t length) -> alloy::core::Result<void> {
        // UART RX register address (example for USART0)
        volatile uint32_t* uart_rhr = reinterpret_cast<volatile uint32_t*>(0x4001C018);

        return m_dma.transfer(uart_rhr, buffer, length);
    }

    auto waitComplete() -> alloy::core::Result<void> {
        return m_dma.waitComplete();
    }

    auto close() -> alloy::core::Result<void> {
        return m_dma.close();
    }

private:
    DmaUartRx m_dma;
};

// ============================================================================
// Example 2: SPI with DMA - High-Speed Flash Operations
// ============================================================================

/**
 * @brief SPI Flash with DMA for maximum throughput
 *
 * Use case: Reading/writing large blocks of data to/from SPI flash
 * for data logging, firmware updates, or file systems.
 */
class SpiFlashDma {
public:
    auto init() -> alloy::core::Result<void> {
        // Initialize SPI
        auto spi_result = m_spi.open();
        if (!spi_result.is_ok()) {
            return spi_result;
        }

        auto config_result = m_spi.configureChipSelect(
            SpiChipSelect::CS0,
            8,  // Clock divider
            SpiMode::Mode0
        );
        if (!config_result.is_ok()) {
            return config_result;
        }

        // Initialize DMA TX channel
        auto tx_result = m_dma_tx.open();
        if (!tx_result.is_ok()) {
            return tx_result;
        }

        DmaConfig tx_config;
        tx_config.direction = DmaDirection::MemoryToPeripheral;
        tx_config.src_width = DmaWidth::Byte;
        tx_config.dst_width = DmaWidth::Byte;
        tx_config.peripheral = DmaPeripheralId::SPI0_TX;
        tx_config.src_increment = true;
        tx_config.dst_increment = false;

        auto tx_cfg_result = m_dma_tx.configure(tx_config);
        if (!tx_cfg_result.is_ok()) {
            return tx_cfg_result;
        }

        // Initialize DMA RX channel
        auto rx_result = m_dma_rx.open();
        if (!rx_result.is_ok()) {
            return rx_result;
        }

        DmaConfig rx_config;
        rx_config.direction = DmaDirection::PeripheralToMemory;
        rx_config.src_width = DmaWidth::Byte;
        rx_config.dst_width = DmaWidth::Byte;
        rx_config.peripheral = DmaPeripheralId::SPI0_RX;
        rx_config.src_increment = false;
        rx_config.dst_increment = true;

        return m_dma_rx.configure(rx_config);
    }

    /**
     * @brief Read large block from SPI flash using DMA
     *
     * This is MUCH faster than CPU-based SPI transfer for large blocks.
     */
    auto readBlock(uint32_t address, uint8_t* buffer, size_t size)
        -> alloy::core::Result<void> {

        // Send read command + address (CPU does this part)
        uint8_t cmd[4] = {
            0x03,  // READ command
            static_cast<uint8_t>((address >> 16) & 0xFF),
            static_cast<uint8_t>((address >> 8) & 0xFF),
            static_cast<uint8_t>(address & 0xFF)
        };

        auto cmd_result = m_spi.write(cmd, 4, SpiChipSelect::CS0);
        if (!cmd_result.is_ok()) {
            return alloy::core::Result<void>::error(cmd_result.error());
        }

        // Use DMA for bulk data read
        // SPI0 RDR address: 0x40008008
        volatile uint32_t* spi_rdr = reinterpret_cast<volatile uint32_t*>(0x40008008);

        auto dma_result = m_dma_rx.transfer(spi_rdr, buffer, size);
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        return m_dma_rx.waitComplete();
    }

    /**
     * @brief Write large block to SPI flash using DMA
     */
    auto writeBlock(uint32_t address, const uint8_t* data, size_t size)
        -> alloy::core::Result<void> {

        // Enable write (required for flash)
        uint8_t wren_cmd = 0x06;
        auto wren_result = m_spi.write(&wren_cmd, 1, SpiChipSelect::CS0);
        if (!wren_result.is_ok()) {
            return alloy::core::Result<void>::error(wren_result.error());
        }

        // Send page program command + address
        uint8_t cmd[4] = {
            0x02,  // PAGE PROGRAM command
            static_cast<uint8_t>((address >> 16) & 0xFF),
            static_cast<uint8_t>((address >> 8) & 0xFF),
            static_cast<uint8_t>(address & 0xFF)
        };

        auto cmd_result = m_spi.write(cmd, 4, SpiChipSelect::CS0);
        if (!cmd_result.is_ok()) {
            return alloy::core::Result<void>::error(cmd_result.error());
        }

        // Use DMA for bulk data write
        // SPI0 TDR address: 0x4000800C
        volatile uint32_t* spi_tdr = reinterpret_cast<volatile uint32_t*>(0x4000800C);

        auto dma_result = m_dma_tx.transfer(data, spi_tdr, size);
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        return m_dma_tx.waitComplete();
    }

    auto close() -> alloy::core::Result<void> {
        m_spi.close();
        m_dma_tx.close();
        return m_dma_rx.close();
    }

private:
    Spi0 m_spi;
    DmaSpiTx m_dma_tx;
    DmaSpiRx m_dma_rx;
};

// ============================================================================
// Example 3: I2C with DMA - EEPROM Operations
// ============================================================================

/**
 * @brief I2C EEPROM with DMA
 *
 * Use case: Reading/writing configuration data, calibration tables,
 * or logs to I2C EEPROM without CPU intervention.
 */
class I2cEepromDma {
public:
    constexpr I2cEepromDma(uint8_t device_addr = 0x50)
        : m_device_addr(device_addr) {}

    auto init() -> alloy::core::Result<void> {
        // Initialize I2C
        auto i2c_result = m_i2c.open();
        if (!i2c_result.is_ok()) {
            return i2c_result;
        }

        auto speed_result = m_i2c.setSpeed(I2cSpeed::Fast);
        if (!speed_result.is_ok()) {
            return speed_result;
        }

        // Initialize DMA TX
        auto tx_result = m_dma_tx.open();
        if (!tx_result.is_ok()) {
            return tx_result;
        }

        DmaConfig tx_config;
        tx_config.direction = DmaDirection::MemoryToPeripheral;
        tx_config.src_width = DmaWidth::Byte;
        tx_config.dst_width = DmaWidth::Byte;
        tx_config.peripheral = DmaPeripheralId::TWIHS0_TX;
        tx_config.src_increment = true;
        tx_config.dst_increment = false;

        auto tx_cfg_result = m_dma_tx.configure(tx_config);
        if (!tx_cfg_result.is_ok()) {
            return tx_cfg_result;
        }

        // Initialize DMA RX
        auto rx_result = m_dma_rx.open();
        if (!rx_result.is_ok()) {
            return rx_result;
        }

        DmaConfig rx_config;
        rx_config.direction = DmaDirection::PeripheralToMemory;
        rx_config.src_width = DmaWidth::Byte;
        rx_config.dst_width = DmaWidth::Byte;
        rx_config.peripheral = DmaPeripheralId::TWIHS0_RX;
        rx_config.src_increment = false;
        rx_config.dst_increment = true;

        return m_dma_rx.configure(rx_config);
    }

    /**
     * @brief Read block from EEPROM using DMA
     */
    auto readBlock(uint16_t address, uint8_t* buffer, size_t size)
        -> alloy::core::Result<void> {

        // Write address (CPU does this)
        uint8_t addr_buf[2] = {
            static_cast<uint8_t>((address >> 8) & 0xFF),
            static_cast<uint8_t>(address & 0xFF)
        };

        auto write_result = m_i2c.write(m_device_addr, addr_buf, 2);
        if (!write_result.is_ok()) {
            return alloy::core::Result<void>::error(write_result.error());
        }

        // Use DMA for bulk read
        // TWIHS0 RHR address: 0x40018030
        volatile uint32_t* twihs_rhr = reinterpret_cast<volatile uint32_t*>(0x40018030);

        auto dma_result = m_dma_rx.transfer(twihs_rhr, buffer, size);
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        return m_dma_rx.waitComplete();
    }

    /**
     * @brief Write block to EEPROM using DMA
     */
    auto writeBlock(uint16_t address, const uint8_t* data, size_t size)
        -> alloy::core::Result<void> {

        // Prepare address + data buffer
        constexpr size_t max_size = 256;
        uint8_t write_buf[max_size + 2];

        if (size > max_size) {
            return alloy::core::Result<void>::error(
                alloy::core::ErrorCode::InvalidParameter
            );
        }

        write_buf[0] = static_cast<uint8_t>((address >> 8) & 0xFF);
        write_buf[1] = static_cast<uint8_t>(address & 0xFF);

        for (size_t i = 0; i < size; ++i) {
            write_buf[2 + i] = data[i];
        }

        // Use DMA for transfer
        // TWIHS0 THR address: 0x40018034
        volatile uint32_t* twihs_thr = reinterpret_cast<volatile uint32_t*>(0x40018034);

        auto dma_result = m_dma_tx.transfer(write_buf, twihs_thr, size + 2);
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        return m_dma_tx.waitComplete();
    }

    auto close() -> alloy::core::Result<void> {
        m_i2c.close();
        m_dma_tx.close();
        return m_dma_rx.close();
    }

private:
    I2c0 m_i2c;
    DmaI2cTx m_dma_tx;
    DmaI2cRx m_dma_rx;
    uint8_t m_device_addr;
};

// ============================================================================
// Example 4: ADC with DMA - High-Speed Data Acquisition
// ============================================================================

/**
 * @brief ADC with DMA for continuous high-speed sampling
 *
 * Use case: Audio recording, oscilloscope, vibration monitoring,
 * spectrum analysis - any application requiring continuous high-speed sampling.
 */
class AdcDmaStreaming {
public:
    auto init() -> alloy::core::Result<void> {
        // Initialize ADC
        auto adc_result = m_adc.open();
        if (!adc_result.is_ok()) {
            return adc_result;
        }

        same70::AdcConfig adc_config;
        adc_config.resolution = same70::AdcResolution::Bits12;
        adc_config.trigger = same70::AdcTrigger::Continuous;
        adc_config.sample_rate = 1000000;  // 1 MSPS
        adc_config.use_dma = true;

        auto config_result = m_adc.configure(adc_config);
        if (!config_result.is_ok()) {
            return config_result;
        }

        auto dma_enable = m_adc.enableDma();
        if (!dma_enable.is_ok()) {
            return dma_enable;
        }

        // Initialize DMA
        auto dma_result = m_dma.open();
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        DmaConfig dma_config;
        dma_config.direction = DmaDirection::PeripheralToMemory;
        dma_config.src_width = DmaWidth::Word;
        dma_config.dst_width = DmaWidth::HalfWord;
        dma_config.peripheral = DmaPeripheralId::AFEC0;
        dma_config.src_increment = false;
        dma_config.dst_increment = true;

        return m_dma.configure(dma_config);
    }

    /**
     * @brief Start streaming ADC data to buffer
     */
    auto startStreaming(uint16_t* buffer, size_t size) -> alloy::core::Result<void> {
        auto enable_result = m_adc.enableChannel(AdcChannel::CH0);
        if (!enable_result.is_ok()) {
            return enable_result;
        }

        auto dma_result = m_dma.transfer(
            m_adc.getDmaSourceAddress(),
            buffer,
            size
        );

        if (!dma_result.is_ok()) {
            return dma_result;
        }

        return m_adc.startConversion();
    }

    bool isComplete() const {
        return m_dma.isComplete();
    }

    auto close() -> alloy::core::Result<void> {
        m_adc.close();
        return m_dma.close();
    }

private:
    Adc0 m_adc;
    DmaAdcChannel0 m_dma;
};

// ============================================================================
// Example 5: Memory-to-Memory DMA - Fast memcpy
// ============================================================================

/**
 * @brief Memory-to-memory DMA for fast data copying
 *
 * Use case: Copying large buffers, image processing, buffer management
 */
class DmaMemcpy {
public:
    auto init() -> alloy::core::Result<void> {
        auto dma_result = m_dma.open();
        if (!dma_result.is_ok()) {
            return dma_result;
        }

        DmaConfig config;
        config.direction = DmaDirection::MemoryToMemory;
        config.src_width = DmaWidth::Word;  // 32-bit for speed
        config.dst_width = DmaWidth::Word;
        config.peripheral = DmaPeripheralId::MEM;
        config.src_increment = true;
        config.dst_increment = true;

        return m_dma.configure(config);
    }

    /**
     * @brief Copy memory using DMA (faster than CPU memcpy for large blocks)
     */
    auto copy(const void* src, void* dst, size_t size_bytes)
        -> alloy::core::Result<void> {

        // Convert byte size to word count
        size_t word_count = size_bytes / 4;

        auto transfer_result = m_dma.transfer(src, dst, word_count);
        if (!transfer_result.is_ok()) {
            return transfer_result;
        }

        return m_dma.waitComplete();
    }

    auto close() -> alloy::core::Result<void> {
        return m_dma.close();
    }

private:
    DmaChannel0 m_dma;
};

// ============================================================================
// Main: Comprehensive DMA demonstration
// ============================================================================

int main() {
    // ========================================================================
    // Example 1: UART with DMA
    // ========================================================================
    {
        UartDmaTx uart_tx;
        [[maybe_unused]] auto init_result = uart_tx.init();

        const char message[] = "Hello via DMA! This is transmitted with zero CPU overhead.\n";
        [[maybe_unused]] auto send_result = uart_tx.send(message, sizeof(message) - 1);
        [[maybe_unused]] auto wait_result = uart_tx.waitComplete();

        [[maybe_unused]] auto close_result = uart_tx.close();
    }

    {
        UartDmaRx uart_rx;
        [[maybe_unused]] auto init_result = uart_rx.init();

        uint8_t rx_buffer[128];
        [[maybe_unused]] auto receive_result = uart_rx.receive(rx_buffer, 128);
        [[maybe_unused]] auto wait_result = uart_rx.waitComplete();

        [[maybe_unused]] auto close_result = uart_rx.close();
    }

    // ========================================================================
    // Example 2: SPI Flash with DMA
    // ========================================================================
    {
        SpiFlashDma flash;
        [[maybe_unused]] auto init_result = flash.init();

        // Read 4KB block from flash
        uint8_t flash_buffer[4096];
        [[maybe_unused]] auto read_result = flash.readBlock(0x00000, flash_buffer, 4096);

        // Write 256 bytes (one page)
        uint8_t write_data[256];
        for (size_t i = 0; i < 256; ++i) {
            write_data[i] = static_cast<uint8_t>(i);
        }
        [[maybe_unused]] auto write_result = flash.writeBlock(0x01000, write_data, 256);

        [[maybe_unused]] auto close_result = flash.close();
    }

    // ========================================================================
    // Example 3: I2C EEPROM with DMA
    // ========================================================================
    {
        I2cEepromDma eeprom(0x50);
        [[maybe_unused]] auto init_result = eeprom.init();

        // Read configuration block
        uint8_t config_data[128];
        [[maybe_unused]] auto read_result = eeprom.readBlock(0x0000, config_data, 128);

        // Write calibration data
        uint8_t cal_data[64];
        for (size_t i = 0; i < 64; ++i) {
            cal_data[i] = static_cast<uint8_t>(i * 2);
        }
        [[maybe_unused]] auto write_result = eeprom.writeBlock(0x0100, cal_data, 64);

        [[maybe_unused]] auto close_result = eeprom.close();
    }

    // ========================================================================
    // Example 4: ADC High-Speed Streaming with DMA
    // ========================================================================
    {
        AdcDmaStreaming adc_stream;
        [[maybe_unused]] auto init_result = adc_stream.init();

        // Capture 10,000 samples at 1 MSPS (10ms of data)
        uint16_t adc_buffer[10000];
        [[maybe_unused]] auto start_result = adc_stream.startStreaming(adc_buffer, 10000);

        // Wait for capture complete
        while (!adc_stream.isComplete()) {
            // CPU is free to do other work!
            // This is the beauty of DMA - zero CPU overhead
        }

        // Process captured data
        uint32_t sum = 0;
        for (size_t i = 0; i < 10000; ++i) {
            sum += adc_buffer[i];
        }
        [[maybe_unused]] uint32_t average = sum / 10000;

        [[maybe_unused]] auto close_result = adc_stream.close();
    }

    // ========================================================================
    // Example 5: Memory-to-Memory DMA (Fast memcpy)
    // ========================================================================
    {
        DmaMemcpy dma_memcpy;
        [[maybe_unused]] auto init_result = dma_memcpy.init();

        // Copy large buffer (much faster than CPU memcpy)
        uint32_t source_buffer[1024];
        uint32_t dest_buffer[1024];

        // Initialize source
        for (size_t i = 0; i < 1024; ++i) {
            source_buffer[i] = i;
        }

        // DMA copy (hardware does the work)
        [[maybe_unused]] auto copy_result = dma_memcpy.copy(
            source_buffer,
            dest_buffer,
            sizeof(source_buffer)
        );

        [[maybe_unused]] auto close_result = dma_memcpy.close();
    }

    // ========================================================================
    // Example 6: Concurrent DMA operations
    // ========================================================================
    {
        // The XDMAC has 24 independent channels, so we can run
        // multiple DMA operations simultaneously!

        UartDmaTx uart;
        SpiFlashDma flash;
        AdcDmaStreaming adc;

        [[maybe_unused]] auto uart_init = uart.init();
        [[maybe_unused]] auto flash_init = flash.init();
        [[maybe_unused]] auto adc_init = adc.init();

        // Start all three DMA operations AT THE SAME TIME
        const char msg[] = "Concurrent DMA!\n";
        [[maybe_unused]] auto uart_start = uart.send(msg, sizeof(msg) - 1);

        uint8_t flash_buf[512];
        [[maybe_unused]] auto flash_start = flash.readBlock(0x0000, flash_buf, 512);

        uint16_t adc_buf[1000];
        [[maybe_unused]] auto adc_start = adc.startStreaming(adc_buf, 1000);

        // All three operations running in parallel via hardware!
        // CPU is completely free for other tasks

        // Wait for all to complete
        [[maybe_unused]] auto uart_wait = uart.waitComplete();
        // flash and adc complete checks would go here

        [[maybe_unused]] auto uart_close = uart.close();
        [[maybe_unused]] auto flash_close = flash.close();
        [[maybe_unused]] auto adc_close = adc.close();
    }

    return 0;
}
