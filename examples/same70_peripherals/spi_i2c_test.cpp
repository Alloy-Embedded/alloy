/**
 * @file spi_i2c_test.cpp
 * @brief Test compilation of SAME70 SPI and I2C templates
 *
 * This file verifies that SPI and I2C templates compile correctly
 * and have the expected API.
 */

#include "hal/platform/same70/spi.hpp"
#include "hal/platform/same70/i2c.hpp"

using namespace alloy::hal::same70;

int main() {
    // Test SPI compilation
    {
        auto spi = Spi0{};

        // These calls will compile but not run (no hardware)
        [[maybe_unused]] auto open_result = spi.open();
        [[maybe_unused]] auto config_result = spi.configureChipSelect(
            SpiChipSelect::CS0,
            8,  // Clock divider
            SpiMode::Mode0
        );

        uint8_t tx_data[4] = {0x01, 0x02, 0x03, 0x04};
        uint8_t rx_data[4];

        [[maybe_unused]] auto transfer_result = spi.transfer(
            tx_data, rx_data, 4, SpiChipSelect::CS0
        );

        [[maybe_unused]] auto write_result = spi.write(
            tx_data, 4, SpiChipSelect::CS0
        );

        [[maybe_unused]] auto read_result = spi.read(
            rx_data, 4, SpiChipSelect::CS0
        );

        [[maybe_unused]] bool is_open = spi.isOpen();

        [[maybe_unused]] auto close_result = spi.close();
    }

    // Test I2C compilation
    {
        auto i2c = I2c0{};

        [[maybe_unused]] auto open_result = i2c.open();
        [[maybe_unused]] auto speed_result = i2c.setSpeed(I2cSpeed::Fast);

        uint8_t device_addr = 0x50;
        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};

        [[maybe_unused]] auto write_result = i2c.write(device_addr, data, 4);

        [[maybe_unused]] auto read_result = i2c.read(device_addr, data, 4);

        uint8_t reg_addr = 0x00;
        uint8_t value = 0x42;

        [[maybe_unused]] auto write_reg_result = i2c.writeRegister(
            device_addr, reg_addr, value
        );

        [[maybe_unused]] auto read_reg_result = i2c.readRegister(
            device_addr, reg_addr, &value
        );

        [[maybe_unused]] bool is_open = i2c.isOpen();

        [[maybe_unused]] auto close_result = i2c.close();
    }

    // Test type aliases
    static_assert(Spi0::base_address == 0x40008000, "SPI0 base address");
    static_assert(Spi1::base_address == 0x40058000, "SPI1 base address");

    static_assert(I2c0::base_address == 0x40018000, "I2C0 base address");
    static_assert(I2c1::base_address == 0x4001C000, "I2C1 base address");
    static_assert(I2c2::base_address == 0x40060000, "I2C2 base address");

    return 0;
}
