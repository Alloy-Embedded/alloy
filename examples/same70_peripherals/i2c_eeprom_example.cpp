/**
 * @file i2c_eeprom_example.cpp
 * @brief Example demonstrating I2C EEPROM operations on SAME70
 *
 * This example shows how to use the SAME70 I2C template to interface
 * with a common I2C EEPROM chip (e.g., AT24C series, 24LC series).
 *
 * Hardware Setup:
 * - Connect I2C EEPROM to TWIHS0 (I2C0):
 *   - SDA: PA3 (TWIHS0_TWD)
 *   - SCL: PA4 (TWIHS0_TWCK)
 *   - Connect A0, A1, A2 to GND (device address = 0x50)
 *   - Pull-up resistors (4.7k) on SDA and SCL
 *
 * Typical EEPROM specs:
 * - Page size: 32 or 64 bytes (depends on device)
 * - Write cycle time: 5-10 ms
 * - I2C speed: Up to 400 kHz (Fast mode)
 */

#include "hal/platform/same70/i2c.hpp"

using namespace alloy::hal::same70;
using namespace alloy::hal;

/**
 * @brief Simple I2C EEPROM driver using SAME70 I2C
 */
class I2cEeprom {
public:
    /**
     * @brief Initialize I2C EEPROM
     *
     * @param device_address 7-bit I2C device address (typically 0x50-0x57)
     * @param page_size Page size in bytes (typically 32 or 64)
     */
    constexpr I2cEeprom(uint8_t device_address = 0x50, size_t page_size = 32)
        : m_device_addr(device_address), m_page_size(page_size) {}

    /**
     * @brief Initialize I2C peripheral
     */
    auto init() -> alloy::core::Result<void> {
        // Open I2C peripheral
        auto result = m_i2c.open();
        if (!result.is_ok()) {
            return result;
        }

        // Set I2C speed to Fast mode (400 kHz)
        return m_i2c.setSpeed(I2cSpeed::Fast);
    }

    /**
     * @brief Write single byte to EEPROM
     *
     * @param address Memory address (16-bit for larger EEPROMs)
     * @param data Byte to write
     */
    auto writeByte(uint16_t address, uint8_t data) -> alloy::core::Result<void> {
        // EEPROM write: [Address_High, Address_Low, Data]
        uint8_t buffer[3] = {
            static_cast<uint8_t>((address >> 8) & 0xFF),
            static_cast<uint8_t>(address & 0xFF),
            data
        };

        auto result = m_i2c.write(m_device_addr, buffer, 3);
        if (!result.is_ok()) {
            return alloy::core::Result<void>::error(result.error());
        }

        // Wait for write cycle to complete (~5ms)
        // In real hardware, poll with read or use fixed delay
        return alloy::core::Result<void>::ok();
    }

    /**
     * @brief Read single byte from EEPROM
     *
     * @param address Memory address (16-bit)
     * @param data Pointer to store read byte
     */
    auto readByte(uint16_t address, uint8_t* data) -> alloy::core::Result<void> {
        // Write address (without STOP condition would require sequential write+read)
        // This simplified version uses two transactions:

        // 1. Write address pointer
        uint8_t addr_buffer[2] = {
            static_cast<uint8_t>((address >> 8) & 0xFF),
            static_cast<uint8_t>(address & 0xFF)
        };

        auto write_result = m_i2c.write(m_device_addr, addr_buffer, 2);
        if (!write_result.is_ok()) {
            return alloy::core::Result<void>::error(write_result.error());
        }

        // 2. Read data byte
        auto read_result = m_i2c.read(m_device_addr, data, 1);
        if (!read_result.is_ok()) {
            return alloy::core::Result<void>::error(read_result.error());
        }

        return alloy::core::Result<void>::ok();
    }

    /**
     * @brief Write page to EEPROM
     *
     * Writes up to one page of data. The address must be page-aligned,
     * and size must not exceed page size.
     *
     * @param address Start address (should be page-aligned)
     * @param data Data buffer to write
     * @param size Number of bytes (must be <= page_size)
     */
    auto writePage(uint16_t address, const uint8_t* data, size_t size)
        -> alloy::core::Result<size_t> {

        if (size > m_page_size) {
            return alloy::core::Result<size_t>::error(
                alloy::core::ErrorCode::InvalidParameter
            );
        }

        // Build write buffer: [Addr_High, Addr_Low, Data...]
        uint8_t buffer[2 + m_page_size];
        buffer[0] = static_cast<uint8_t>((address >> 8) & 0xFF);
        buffer[1] = static_cast<uint8_t>(address & 0xFF);

        for (size_t i = 0; i < size; ++i) {
            buffer[2 + i] = data[i];
        }

        auto result = m_i2c.write(m_device_addr, buffer, 2 + size);
        if (!result.is_ok()) {
            return alloy::core::Result<size_t>::error(result.error());
        }

        // Wait for write cycle (~5ms)
        return alloy::core::Result<size_t>::ok(size);
    }

    /**
     * @brief Read multiple bytes from EEPROM
     *
     * Performs sequential read starting at the given address.
     *
     * @param address Start address (16-bit)
     * @param data Buffer to store read data
     * @param size Number of bytes to read
     */
    auto readSequential(uint16_t address, uint8_t* data, size_t size)
        -> alloy::core::Result<size_t> {

        // Write address pointer
        uint8_t addr_buffer[2] = {
            static_cast<uint8_t>((address >> 8) & 0xFF),
            static_cast<uint8_t>(address & 0xFF)
        };

        auto write_result = m_i2c.write(m_device_addr, addr_buffer, 2);
        if (!write_result.is_ok()) {
            return alloy::core::Result<size_t>::error(write_result.error());
        }

        // Read sequential data
        return m_i2c.read(m_device_addr, data, size);
    }

    /**
     * @brief Alternative: Use register read/write methods
     *
     * For single-byte operations at 8-bit addresses, you can use
     * the convenience methods writeRegister() and readRegister().
     */
    auto writeRegisterExample(uint8_t reg_addr, uint8_t value)
        -> alloy::core::Result<void> {
        return m_i2c.writeRegister(m_device_addr, reg_addr, value);
    }

    auto readRegisterExample(uint8_t reg_addr, uint8_t* value)
        -> alloy::core::Result<void> {
        return m_i2c.readRegister(m_device_addr, reg_addr, value);
    }

    /**
     * @brief Close I2C peripheral
     */
    auto close() -> alloy::core::Result<void> {
        return m_i2c.close();
    }

private:
    I2c0 m_i2c;
    uint8_t m_device_addr;
    size_t m_page_size;
};

/**
 * @brief Example usage
 */
int main() {
    // Create EEPROM driver (AT24C256: address 0x50, page size 64 bytes)
    I2cEeprom eeprom(0x50, 64);

    // Initialize I2C
    [[maybe_unused]] auto init_result = eeprom.init();

    // Write single byte
    [[maybe_unused]] auto write_result = eeprom.writeByte(0x0100, 0x42);

    // Read single byte
    uint8_t read_byte;
    [[maybe_unused]] auto read_result = eeprom.readByte(0x0100, &read_byte);

    // Write page (up to 64 bytes)
    uint8_t write_buffer[32];
    for (size_t i = 0; i < 32; ++i) {
        write_buffer[i] = static_cast<uint8_t>(i);
    }
    [[maybe_unused]] auto page_write_result = eeprom.writePage(0x0200, write_buffer, 32);

    // Read sequential data
    uint8_t read_buffer[32];
    [[maybe_unused]] auto seq_read_result = eeprom.readSequential(0x0200, read_buffer, 32);

    // Alternative: Use register methods for 8-bit addresses
    [[maybe_unused]] auto reg_write = eeprom.writeRegisterExample(0x10, 0xAA);
    uint8_t reg_value;
    [[maybe_unused]] auto reg_read = eeprom.readRegisterExample(0x10, &reg_value);

    // Close when done
    [[maybe_unused]] auto close_result = eeprom.close();

    return 0;
}
