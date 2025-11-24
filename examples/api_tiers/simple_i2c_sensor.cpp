/**
 * @file simple_i2c_sensor.cpp
 * @brief Level 1 Simple API - I2C Sensor Reading Example
 *
 * Demonstrates the Simple tier I2C API with a one-liner setup.
 * Perfect for quick sensor reading and basic I2C communication.
 *
 * This example shows:
 * - One-liner I2C setup with defaults (100 kHz, 7-bit addressing)
 * - Reading from I2C sensors (temperature, accelerometer, etc.)
 * - Write-then-read pattern for register access
 * - Device scanning to find I2C addresses
 *
 * Hardware Setup:
 * - I2C SDA -> Connect to sensor SDA with 4.7k pullup to 3.3V
 * - I2C SCL -> Connect to sensor SCL with 4.7k pullup to 3.3V
 *
 * Common I2C Sensors:
 * - BME280 (temp/humidity/pressure): address 0x76 or 0x77
 * - MPU6050 (accelerometer/gyro): address 0x68 or 0x69
 * - DS3231 (RTC): address 0x68
 * - AT24C32 (EEPROM): address 0x50
 *
 * Expected Behavior:
 * - Scans I2C bus for devices
 * - Reads WHO_AM_I register from sensor
 * - Continuously reads sensor data
 * - Blinks LED on successful reads
 *
 * @note Part of Phase 3.4: I2C Implementation
 * @see docs/API_TIERS.md
 */

// ============================================================================
// Board-Specific Configuration
// ============================================================================

#if defined(PLATFORM_STM32F401RE)
#include "boards/nucleo_f401re/board.hpp"
using namespace board::nucleo_f401re;

// I2C1 pins (Arduino header)
using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_STM32F722ZE)
#include "boards/nucleo_f722ze/board.hpp"
using namespace board::nucleo_f722ze;

// I2C1 pins (Arduino header)
using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_STM32G071RB)
#include "boards/nucleo_g071rb/board.hpp"
using namespace board::nucleo_g071rb;

// I2C1 pins (Arduino header)
using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_STM32G0B1RE)
#include "boards/nucleo_g0b1re/board.hpp"
using namespace board::nucleo_g0b1re;

// I2C1 pins (Arduino header)
using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_SAME70)
#include "boards/same70_xplained/board.hpp"
using namespace board::same70_xplained;

// TWI0 pins (I2C)
using I2cSda = Pin<PeripheralId::TWI0, PinId::PA3>;
using I2cScl = Pin<PeripheralId::TWI0, PinId::PA4>;

#else
#error "Unsupported platform. Please define one of the supported platforms."
#endif

#include "hal/gpio.hpp"

using namespace ucore;
using namespace ucore::hal;

// ============================================================================
// I2C Device Addresses (Common Sensors)
// ============================================================================

constexpr uint8_t MPU6050_ADDR = 0x68;      // Accelerometer/Gyro
constexpr uint8_t BME280_ADDR = 0x76;       // Temp/Humidity/Pressure
constexpr uint8_t DS3231_ADDR = 0x68;       // RTC
constexpr uint8_t AT24C32_ADDR = 0x50;      // EEPROM

// MPU6050 Registers
constexpr uint8_t MPU6050_WHO_AM_I = 0x75;  // Should return 0x68
constexpr uint8_t MPU6050_PWR_MGMT_1 = 0x6B;
constexpr uint8_t MPU6050_ACCEL_XOUT_H = 0x3B;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Scan I2C bus for devices
 *
 * @param i2c I2C instance
 * @param led LED for visual feedback
 */
template <typename I2c, typename Led>
void scan_i2c_bus(I2c& i2c, Led& led) {
    uint8_t devices_found = 0;

    // Scan addresses 0x08 to 0x77 (valid 7-bit range)
    for (uint8_t addr = 0x08; addr < 0x78; ++addr) {
        // Try to write 0 bytes (address-only transmission)
        auto result = i2c.write(addr, nullptr, 0);

        if (result.is_ok()) {
            // Device found!
            devices_found++;
            led.toggle();

            // Small delay for visual feedback
            for (volatile uint32_t i = 0; i < 100000; ++i);
        }
    }

    // Blink LED based on number of devices found
    for (uint8_t i = 0; i < devices_found; ++i) {
        led.toggle();
        for (volatile uint32_t j = 0; j < 500000; ++j);
        led.toggle();
        for (volatile uint32_t j = 0; j < 500000; ++j);
    }
}

/**
 * @brief Read register from I2C device
 *
 * @param i2c I2C instance
 * @param device_addr Device address
 * @param reg_addr Register address
 * @return Register value or 0xFF on error
 */
template <typename I2c>
uint8_t read_register(I2c& i2c, uint8_t device_addr, uint8_t reg_addr) {
    uint8_t data = 0xFF;

    // Write register address
    auto write_result = i2c.write(device_addr, &reg_addr, 1);
    if (!write_result.is_ok()) {
        return 0xFF;
    }

    // Read data
    auto read_result = i2c.read(device_addr, &data, 1);
    if (!read_result.is_ok()) {
        return 0xFF;
    }

    return data;
}

/**
 * @brief Write register to I2C device
 *
 * @param i2c I2C instance
 * @param device_addr Device address
 * @param reg_addr Register address
 * @param value Value to write
 * @return true on success
 */
template <typename I2c>
bool write_register(I2c& i2c, uint8_t device_addr, uint8_t reg_addr, uint8_t value) {
    uint8_t data[2] = {reg_addr, value};
    auto result = i2c.write(device_addr, data, 2);
    return result.is_ok();
}

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Simple I2C sensor reading example
 *
 * Demonstrates one-liner I2C setup and basic sensor communication.
 */
int main() {
    // ========================================================================
    // Setup
    // ========================================================================

    // Simple API: One-liner setup with defaults (100 kHz, 7-bit addressing)
    auto i2c = I2cInstance::quick_setup<I2cSda, I2cScl>(
        I2cSpeed::Standard_100kHz  // 100 kHz (safe default for most sensors)
    );

    // Initialize I2C peripheral
    i2c.initialize();

    // Setup LED for visual feedback
    auto led = LedPin::output();

    // ========================================================================
    // Scan I2C Bus for Devices
    // ========================================================================

    // Find all connected I2C devices
    scan_i2c_bus(i2c, led);

    // ========================================================================
    // Read WHO_AM_I Register (MPU6050 Example)
    // ========================================================================

    // Read WHO_AM_I register from MPU6050
    uint8_t who_am_i = read_register(i2c, MPU6050_ADDR, MPU6050_WHO_AM_I);

    // Check if MPU6050 is present (WHO_AM_I should return 0x68)
    if (who_am_i == 0x68) {
        // MPU6050 found! Blink LED 3 times
        for (uint8_t i = 0; i < 3; ++i) {
            led.toggle();
            for (volatile uint32_t j = 0; j < 500000; ++j);
            led.toggle();
            for (volatile uint32_t j = 0; j < 500000; ++j);
        }

        // Wake up MPU6050 (clear sleep bit in PWR_MGMT_1)
        write_register(i2c, MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00);
    }

    // ========================================================================
    // Continuous Sensor Reading
    // ========================================================================

    while (true) {
        // Read accelerometer data from MPU6050 (6 bytes: X, Y, Z high/low)
        uint8_t accel_data[6];
        uint8_t reg_addr = MPU6050_ACCEL_XOUT_H;

        // Write register address
        auto write_result = i2c.write(MPU6050_ADDR, &reg_addr, 1);

        if (write_result.is_ok()) {
            // Read 6 bytes of accelerometer data
            auto read_result = i2c.read(MPU6050_ADDR, accel_data, 6);

            if (read_result.is_ok()) {
                // Combine high and low bytes
                int16_t accel_x = (accel_data[0] << 8) | accel_data[1];
                int16_t accel_y = (accel_data[2] << 8) | accel_data[3];
                int16_t accel_z = (accel_data[4] << 8) | accel_data[5];

                // Process accelerometer data
                // (Convert to g's: accel_x / 16384.0 for ±2g range)

                // Blink LED on successful read
                led.toggle();
            }
        }

        // Delay between readings (10 Hz sampling rate)
        for (volatile uint32_t i = 0; i < 1000000; ++i);
    }

    return 0;
}

/**
 * Key Points:
 *
 * 1. One-liner setup: quick_setup() handles all configuration
 * 2. Default settings: 100 kHz, 7-bit addressing (most common)
 * 3. Pullup resistors: 4.7k required on SDA/SCL lines
 * 4. Device scanning: Find all connected I2C devices
 * 5. Register access: Write-then-read pattern for registers
 *
 * I2C Communication Pattern:
 * - Write: START -> ADDR+W -> REG_ADDR -> DATA -> STOP
 * - Read:  START -> ADDR+W -> REG_ADDR -> RESTART -> ADDR+R -> DATA -> STOP
 *
 * Common I2C Speeds:
 * - 100 kHz (Standard-mode):   Most common, widest compatibility
 * - 400 kHz (Fast-mode):        Faster sensors, shorter cables
 * - 1 MHz (Fast-mode Plus):     High-speed sensors, modern MCUs only
 *
 * Pullup Resistor Selection:
 * - 4.7k ohm: Standard value for 100 kHz and 400 kHz
 * - 2.2k ohm: For 400 kHz with long cables
 * - 1k ohm:   For Fast-mode Plus (1 MHz)
 *
 * Troubleshooting:
 * - Device not found: Check pullup resistors, wiring, power
 * - NACK errors: Wrong address, device not ready
 * - Bus lockup: Power cycle, check for short circuits
 * - Timing issues: Reduce speed to 100 kHz
 */
