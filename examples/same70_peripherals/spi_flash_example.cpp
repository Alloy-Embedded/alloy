/**
 * @file spi_flash_example.cpp
 * @brief Example demonstrating SPI flash memory operations on SAME70
 *
 * This example shows how to use the SAME70 SPI template to interface
 * with a common SPI flash memory chip (e.g., W25Q series, MX25L series).
 *
 * Hardware Setup:
 * - Connect SPI flash to SPI0:
 *   - MISO: PA12 (SPI0_MISO)
 *   - MOSI: PA13 (SPI0_MOSI)
 *   - SCK:  PA14 (SPI0_SPCK)
 *   - CS0:  PA11 (SPI0_NPCS0)
 *
 * Flash Commands (example for W25Q series):
 * - 0x9F: Read JEDEC ID
 * - 0x06: Write Enable
 * - 0x04: Write Disable
 * - 0x05: Read Status Register
 * - 0x02: Page Program
 * - 0x03: Read Data
 * - 0x20: Sector Erase
 */

#include "hal/platform/same70/spi.hpp"

using namespace alloy::hal::same70;

// Flash command definitions
namespace FlashCmd {
    constexpr uint8_t READ_JEDEC_ID = 0x9F;
    constexpr uint8_t WRITE_ENABLE = 0x06;
    constexpr uint8_t WRITE_DISABLE = 0x04;
    constexpr uint8_t READ_STATUS = 0x05;
    constexpr uint8_t PAGE_PROGRAM = 0x02;
    constexpr uint8_t READ_DATA = 0x03;
    constexpr uint8_t SECTOR_ERASE = 0x20;
}

// Flash status register bits
namespace FlashStatus {
    constexpr uint8_t BUSY = (1 << 0);
    constexpr uint8_t WEL = (1 << 1);  // Write Enable Latch
}

/**
 * @brief Simple SPI Flash driver using SAME70 SPI
 */
class SpiFlash {
public:
    /**
     * @brief Initialize SPI flash
     */
    auto init() -> alloy::core::Result<void> {
        // Open SPI peripheral
        auto result = m_spi.open();
        if (!result.is_ok()) {
            return result;
        }

        // Configure CS0 for flash (8 MHz SPI clock, Mode 0)
        // Clock divider = MCK / SPI_CLK = 150MHz / 8MHz = ~19
        return m_spi.configureChipSelect(
            SpiChipSelect::CS0,
            19,  // Clock divider for ~8 MHz
            alloy::hal::SpiMode::Mode0
        );
    }

    /**
     * @brief Read flash JEDEC ID
     *
     * Returns manufacturer ID, device type, and capacity.
     * Example: W25Q128 returns 0xEF, 0x40, 0x18
     */
    auto readJedecId(uint8_t* manufacturer, uint8_t* device_type, uint8_t* capacity)
        -> alloy::core::Result<void> {

        uint8_t tx_data[4] = {FlashCmd::READ_JEDEC_ID, 0xFF, 0xFF, 0xFF};
        uint8_t rx_data[4];

        auto result = m_spi.transfer(tx_data, rx_data, 4, SpiChipSelect::CS0);
        if (!result.is_ok()) {
            return alloy::core::Result<void>::error(result.error());
        }

        *manufacturer = rx_data[1];
        *device_type = rx_data[2];
        *capacity = rx_data[3];

        return alloy::core::Result<void>::ok();
    }

    /**
     * @brief Read flash status register
     */
    auto readStatus(uint8_t* status) -> alloy::core::Result<void> {
        uint8_t tx_data[2] = {FlashCmd::READ_STATUS, 0xFF};
        uint8_t rx_data[2];

        auto result = m_spi.transfer(tx_data, rx_data, 2, SpiChipSelect::CS0);
        if (!result.is_ok()) {
            return alloy::core::Result<void>::error(result.error());
        }

        *status = rx_data[1];
        return alloy::core::Result<void>::ok();
    }

    /**
     * @brief Wait for flash to become ready (not busy)
     */
    auto waitReady() -> alloy::core::Result<void> {
        uint8_t status;
        for (int i = 0; i < 10000; ++i) {
            auto result = readStatus(&status);
            if (!result.is_ok()) {
                return result;
            }

            if ((status & FlashStatus::BUSY) == 0) {
                return alloy::core::Result<void>::ok();
            }
        }

        return alloy::core::Result<void>::error(alloy::core::ErrorCode::Timeout);
    }

    /**
     * @brief Enable writes (required before program/erase)
     */
    auto writeEnable() -> alloy::core::Result<void> {
        uint8_t cmd = FlashCmd::WRITE_ENABLE;
        auto result = m_spi.write(&cmd, 1, SpiChipSelect::CS0);
        if (!result.is_ok()) {
            return alloy::core::Result<void>::error(result.error());
        }
        return alloy::core::Result<void>::ok();
    }

    /**
     * @brief Read data from flash
     */
    auto read(uint32_t address, uint8_t* buffer, size_t size)
        -> alloy::core::Result<size_t> {

        // Wait for ready
        auto wait_result = waitReady();
        if (!wait_result.is_ok()) {
            return alloy::core::Result<size_t>::error(wait_result.error());
        }

        // Send read command + 24-bit address
        uint8_t cmd[4] = {
            FlashCmd::READ_DATA,
            static_cast<uint8_t>((address >> 16) & 0xFF),
            static_cast<uint8_t>((address >> 8) & 0xFF),
            static_cast<uint8_t>(address & 0xFF)
        };

        auto cmd_result = m_spi.write(cmd, 4, SpiChipSelect::CS0);
        if (!cmd_result.is_ok()) {
            return alloy::core::Result<size_t>::error(cmd_result.error());
        }

        // After write command, we need to continue same transaction for read

        // Read data
        return m_spi.read(buffer, size, SpiChipSelect::CS0);
    }

    /**
     * @brief Close SPI peripheral
     */
    auto close() -> alloy::core::Result<void> {
        return m_spi.close();
    }

private:
    Spi0 m_spi;
};

/**
 * @brief Example usage
 */
int main() {
    SpiFlash flash;

    // Initialize flash
    [[maybe_unused]] auto init_result = flash.init();

    // Read JEDEC ID to verify flash is connected
    uint8_t manufacturer, device_type, capacity;
    [[maybe_unused]] auto id_result = flash.readJedecId(&manufacturer, &device_type, &capacity);

    // Read flash status
    uint8_t status;
    [[maybe_unused]] auto status_result = flash.readStatus(&status);

    // Read 256 bytes from address 0x000000
    uint8_t read_buffer[256];
    [[maybe_unused]] auto read_result = flash.read(0x000000, read_buffer, 256);

    // Close when done
    [[maybe_unused]] auto close_result = flash.close();

    return 0;
}
