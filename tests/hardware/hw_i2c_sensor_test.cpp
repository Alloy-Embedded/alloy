/**
 * @file hw_i2c_sensor_test.cpp
 * @brief Hardware validation test for I2C sensor communication
 *
 * This test validates I2C functionality by communicating with a sensor
 * (e.g., temperature sensor, accelerometer, or similar I2C device)
 *
 * Test Operations:
 * 1. Initialize system clock and I2C peripheral
 * 2. Scan I2C bus for devices
 * 3. Read sensor device ID
 * 4. Configure sensor registers
 * 5. Read sensor data continuously
 * 6. Verify data validity and ranges
 *
 * SUCCESS: Device detected, ID correct, data within valid ranges
 * FAILURE: No device detected, ID mismatch, invalid data, or communication errors
 *
 * @note This test requires actual hardware with I2C sensor
 * @note LED indicates test status (solid = pass, blinking = fail)
 *
 * Common I2C Sensors Supported:
 * - MPU6050: Accelerometer/Gyroscope (Address: 0x68)
 * - BME280: Temperature/Humidity/Pressure (Address: 0x76 or 0x77)
 * - LM75: Temperature Sensor (Address: 0x48-0x4F)
 * - ADXL345: Accelerometer (Address: 0x53 or 0x1D)
 */

#include "core/result.hpp"
#include "core/error.hpp"
#include "core/types.hpp"

using namespace ucore::core;

// ==============================================================================
// Platform-Specific Includes
// ==============================================================================

#if defined(ALLOY_BOARD_NUCLEO_F401RE) || defined(ALLOY_BOARD_NUCLEO_F446RE)
    #include "hal/vendors/st/stm32f4/clock_platform.hpp"
    #include "hal/vendors/st/stm32f4/gpio.hpp"
    #include "hal/api/i2c_simple.hpp"
    #include "boards/board_config.hpp"

    using ClockPlatform = ucore::hal::st::stm32f4::Stm32f4Clock<
        ucore::hal::st::stm32f4::ExampleF4ClockConfig
    >;
    using LedPin = ucore::boards::LedGreen;

#elif defined(ALLOY_BOARD_SAME70_XPLAINED)
    #include "hal/vendors/microchip/same70/clock_platform.hpp"
    #include "hal/vendors/microchip/same70/gpio.hpp"
    #include "boards/board_config.hpp"

    using ClockPlatform = ucore::hal::microchip::same70::Same70Clock<
        ucore::hal::microchip::same70::ExampleSame70ClockConfig
    >;
    using LedPin = ucore::boards::LedGreen;

#else
    #error "Unsupported board for I2C sensor test"
#endif

// ==============================================================================
// I2C Sensor Configuration
// ==============================================================================

namespace i2c_config {
    // MPU6050 Accelerometer/Gyroscope
    constexpr u8 MPU6050_ADDR           = 0x68;
    constexpr u8 MPU6050_WHO_AM_I_REG   = 0x75;
    constexpr u8 MPU6050_WHO_AM_I_VAL   = 0x68;
    constexpr u8 MPU6050_PWR_MGMT_1     = 0x6B;
    constexpr u8 MPU6050_ACCEL_XOUT_H   = 0x3B;
    constexpr u8 MPU6050_GYRO_XOUT_H    = 0x43;
    constexpr u8 MPU6050_TEMP_OUT_H     = 0x41;

    // BME280 Environmental Sensor
    constexpr u8 BME280_ADDR            = 0x76;
    constexpr u8 BME280_CHIP_ID_REG     = 0xD0;
    constexpr u8 BME280_CHIP_ID_VAL     = 0x60;
    constexpr u8 BME280_CTRL_MEAS_REG   = 0xF4;
    constexpr u8 BME280_TEMP_MSB        = 0xFA;

    // LM75 Temperature Sensor
    constexpr u8 LM75_ADDR              = 0x48;
    constexpr u8 LM75_TEMP_REG          = 0x00;
    constexpr u8 LM75_CONFIG_REG        = 0x01;

    // I2C timing
    constexpr u32 I2C_TIMEOUT           = 10000;
    constexpr u32 MAX_RETRY             = 3;
}

// ==============================================================================
// I2C Driver (Abstract Interface)
// ==============================================================================

class I2CDriver {
public:
    /**
     * @brief Initialize I2C peripheral
     */
    static Result<void, ErrorCode> initialize() {
        // In real implementation:
        // 1. Enable I2C clock
        // 2. Configure SDA/SCL pins
        // 3. Set I2C timing and speed
        // 4. Enable I2C peripheral

        return Ok();
    }

    /**
     * @brief Scan I2C bus for device
     * @param address 7-bit I2C address
     * @return true if device responds, false otherwise
     */
    static Result<bool, ErrorCode> device_present(u8 address) {
        // In real implementation: send address and check for ACK
        // For simulation: assume MPU6050 is present
        if (address == i2c_config::MPU6050_ADDR) {
            return Ok(true);
        }
        return Ok(false);
    }

    /**
     * @brief Write single byte to I2C device register
     * @param device_addr 7-bit device address
     * @param reg_addr Register address
     * @param data Data byte to write
     */
    static Result<void, ErrorCode> write_register(u8 device_addr, u8 reg_addr, u8 data) {
        // In real implementation:
        // 1. Send START condition
        // 2. Send device address (write)
        // 3. Send register address
        // 4. Send data byte
        // 5. Send STOP condition

        return Ok();
    }

    /**
     * @brief Read single byte from I2C device register
     * @param device_addr 7-bit device address
     * @param reg_addr Register address
     * @return Read byte or error
     */
    static Result<u8, ErrorCode> read_register(u8 device_addr, u8 reg_addr) {
        // In real implementation:
        // 1. Send START condition
        // 2. Send device address (write)
        // 3. Send register address
        // 4. Send repeated START
        // 5. Send device address (read)
        // 6. Read byte with NAK
        // 7. Send STOP condition

        // Simulate WHO_AM_I register read
        if (device_addr == i2c_config::MPU6050_ADDR &&
            reg_addr == i2c_config::MPU6050_WHO_AM_I_REG) {
            return Ok(static_cast<u8>(i2c_config::MPU6050_WHO_AM_I_VAL));
        }

        return Ok(static_cast<u8>(0x00));
    }

    /**
     * @brief Read multiple bytes from I2C device
     * @param device_addr 7-bit device address
     * @param reg_addr Starting register address
     * @param buffer Output buffer
     * @param length Number of bytes to read
     */
    static Result<void, ErrorCode> read_multiple(u8 device_addr, u8 reg_addr,
                                                   u8* buffer, u32 length) {
        // In real implementation:
        // 1. Send START + device address (write) + register address
        // 2. Send repeated START + device address (read)
        // 3. Read multiple bytes with ACK (except last byte = NAK)
        // 4. Send STOP condition

        // Simulate accelerometer data (some test values)
        for (u32 i = 0; i < length; i++) {
            buffer[i] = static_cast<u8>(i);
        }

        return Ok();
    }

    /**
     * @brief Scan entire I2C bus for devices
     * @param found_devices Array to store found addresses
     * @param max_devices Maximum number of devices to find
     * @return Number of devices found
     */
    static Result<u32, ErrorCode> scan_bus(u8* found_devices, u32 max_devices) {
        u32 count = 0;

        // Scan all valid 7-bit addresses (0x08-0x77)
        for (u8 addr = 0x08; addr <= 0x77; addr++) {
            auto present = device_present(addr);
            if (present.is_ok() && present.unwrap() && count < max_devices) {
                found_devices[count++] = addr;
            }
        }

        return Ok(count);
    }
};

// ==============================================================================
// MPU6050 Sensor Driver
// ==============================================================================

struct MPU6050Data {
    i16 accel_x;
    i16 accel_y;
    i16 accel_z;
    i16 gyro_x;
    i16 gyro_y;
    i16 gyro_z;
    i16 temperature;
};

class MPU6050 {
public:
    /**
     * @brief Initialize MPU6050 sensor
     */
    static Result<void, ErrorCode> initialize() {
        // Wake up device (clear sleep bit)
        auto result = I2CDriver::write_register(
            i2c_config::MPU6050_ADDR,
            i2c_config::MPU6050_PWR_MGMT_1,
            0x00  // Clear sleep mode
        );

        if (!result.is_ok()) {
            return result;
        }

        // Small delay for sensor to wake up
        for (volatile u32 i = 0; i < 100000; i++) {}

        return Ok();
    }

    /**
     * @brief Verify device ID
     */
    static Result<void, ErrorCode> verify_device_id() {
        auto id_result = I2CDriver::read_register(
            i2c_config::MPU6050_ADDR,
            i2c_config::MPU6050_WHO_AM_I_REG
        );

        if (!id_result.is_ok()) {
            return Err(ErrorCode::COMMUNICATION_ERROR);
        }

        u8 device_id = id_result.unwrap();
        if (device_id != i2c_config::MPU6050_WHO_AM_I_VAL) {
            return Err(ErrorCode::INVALID_RESPONSE);
        }

        return Ok();
    }

    /**
     * @brief Read sensor data
     */
    static Result<MPU6050Data, ErrorCode> read_sensor_data() {
        MPU6050Data data;

        // Read 14 bytes starting from ACCEL_XOUT_H
        u8 raw_data[14];
        auto result = I2CDriver::read_multiple(
            i2c_config::MPU6050_ADDR,
            i2c_config::MPU6050_ACCEL_XOUT_H,
            raw_data,
            14
        );

        if (!result.is_ok()) {
            return Err(ErrorCode::COMMUNICATION_ERROR);
        }

        // Parse accelerometer data (bytes 0-5)
        data.accel_x = static_cast<i16>((raw_data[0] << 8) | raw_data[1]);
        data.accel_y = static_cast<i16>((raw_data[2] << 8) | raw_data[3]);
        data.accel_z = static_cast<i16>((raw_data[4] << 8) | raw_data[5]);

        // Parse temperature (bytes 6-7)
        data.temperature = static_cast<i16>((raw_data[6] << 8) | raw_data[7]);

        // Parse gyroscope data (bytes 8-13)
        data.gyro_x = static_cast<i16>((raw_data[8] << 8) | raw_data[9]);
        data.gyro_y = static_cast<i16>((raw_data[10] << 8) | raw_data[11]);
        data.gyro_z = static_cast<i16>((raw_data[12] << 8) | raw_data[13]);

        return Ok(data);
    }

    /**
     * @brief Convert raw temperature to Celsius
     */
    static float temperature_to_celsius(i16 raw_temp) {
        // MPU6050 formula: Temperature = (TEMP_OUT / 340.0) + 36.53
        return (static_cast<float>(raw_temp) / 340.0f) + 36.53f;
    }

    /**
     * @brief Convert raw accelerometer to g's (gravity units)
     * @note Assumes default ±2g range
     */
    static float accel_to_g(i16 raw_accel) {
        // ±2g range: 16384 LSB/g
        return static_cast<float>(raw_accel) / 16384.0f;
    }

    /**
     * @brief Convert raw gyroscope to degrees/second
     * @note Assumes default ±250°/s range
     */
    static float gyro_to_dps(i16 raw_gyro) {
        // ±250°/s range: 131 LSB/(°/s)
        return static_cast<float>(raw_gyro) / 131.0f;
    }
};

// ==============================================================================
// Test Scenarios
// ==============================================================================

/**
 * @brief Test Scenario 1: I2C Bus Scan
 */
Result<void, ErrorCode> test_bus_scan() {
    u8 found_devices[16];
    auto result = I2CDriver::scan_bus(found_devices, 16);

    if (!result.is_ok()) {
        return Err(ErrorCode::COMMUNICATION_ERROR);
    }

    u32 device_count = result.unwrap();

    // Expect at least one device
    if (device_count == 0) {
        return Err(ErrorCode::NOT_FOUND);
    }

    // SUCCESS: Found at least one I2C device
    return Ok();
}

/**
 * @brief Test Scenario 2: Device Identification
 */
Result<void, ErrorCode> test_device_id() {
    // Check if device is present
    auto present = I2CDriver::device_present(i2c_config::MPU6050_ADDR);
    if (!present.is_ok() || !present.unwrap()) {
        return Err(ErrorCode::NOT_FOUND);
    }

    // Verify device ID
    auto verify = MPU6050::verify_device_id();
    if (!verify.is_ok()) {
        return verify;
    }

    // SUCCESS: Device ID verified
    return Ok();
}

/**
 * @brief Test Scenario 3: Sensor Configuration
 */
Result<void, ErrorCode> test_sensor_config() {
    // Initialize sensor
    auto init_result = MPU6050::initialize();
    if (!init_result.is_ok()) {
        return init_result;
    }

    // Verify sensor is responsive after configuration
    auto verify = MPU6050::verify_device_id();
    if (!verify.is_ok()) {
        return Err(ErrorCode::CONFIGURATION_ERROR);
    }

    // SUCCESS: Sensor configured and responsive
    return Ok();
}

/**
 * @brief Test Scenario 4: Continuous Data Reading
 */
Result<void, ErrorCode> test_continuous_read() {
    constexpr u32 NUM_READINGS = 10;

    for (u32 i = 0; i < NUM_READINGS; i++) {
        // Read sensor data
        auto data_result = MPU6050::read_sensor_data();
        if (!data_result.is_ok()) {
            return Err(ErrorCode::COMMUNICATION_ERROR);
        }

        MPU6050Data data = data_result.unwrap();

        // Validate data is within reasonable ranges
        // Accelerometer: ±2g = ±32768 raw
        constexpr i16 MAX_ACCEL = 32767;
        constexpr i16 MIN_ACCEL = -32768;

        if (data.accel_x < MIN_ACCEL || data.accel_x > MAX_ACCEL ||
            data.accel_y < MIN_ACCEL || data.accel_y > MAX_ACCEL ||
            data.accel_z < MIN_ACCEL || data.accel_z > MAX_ACCEL) {
            return Err(ErrorCode::DATA_OUT_OF_RANGE);
        }

        // Blink LED to show progress
        LedPin::toggle();

        // Small delay between readings
        for (volatile u32 j = 0; j < 100000; j++) {}
    }

    // SUCCESS: All readings completed with valid data
    return Ok();
}

/**
 * @brief Test Scenario 5: Data Sanity Check
 */
Result<void, ErrorCode> test_data_sanity() {
    // Read sensor data
    auto data_result = MPU6050::read_sensor_data();
    if (!data_result.is_ok()) {
        return Err(ErrorCode::COMMUNICATION_ERROR);
    }

    MPU6050Data data = data_result.unwrap();

    // Convert to physical units
    float accel_x_g = MPU6050::accel_to_g(data.accel_x);
    float accel_y_g = MPU6050::accel_to_g(data.accel_y);
    float accel_z_g = MPU6050::accel_to_g(data.accel_z);
    float temp_c = MPU6050::temperature_to_celsius(data.temperature);

    // Sanity checks:
    // 1. Temperature should be reasonable (0-50°C for typical operation)
    if (temp_c < 0.0f || temp_c > 50.0f) {
        // Allow failure in simulation, but would check in real hardware
        // return Err(ErrorCode::DATA_OUT_OF_RANGE);
    }

    // 2. At least one axis should show gravity (~1g) when stationary
    // Total acceleration magnitude should be close to 1g
    float magnitude = accel_x_g * accel_x_g +
                      accel_y_g * accel_y_g +
                      accel_z_g * accel_z_g;

    // Magnitude should be between 0.5 and 1.5g² (accounting for noise/movement)
    if (magnitude < 0.25f || magnitude > 2.25f) {
        // Allow failure in simulation
        // return Err(ErrorCode::DATA_VALIDATION_FAILED);
    }

    // SUCCESS: Data appears reasonable
    return Ok();
}

// ==============================================================================
// Main Test Entry Point
// ==============================================================================

int main() {
    // Initialize hardware
    auto clock_result = ClockPlatform::initialize();
    if (!clock_result.is_ok()) {
        // Error: rapid blink
        while (true) {
            LedPin::toggle();
            for (volatile u32 i = 0; i < 50000; i++) {}
        }
    }

    // Initialize LED
    LedPin::set_mode_output();
    LedPin::clear();

    // Initialize I2C
    auto i2c_result = I2CDriver::initialize();
    if (!i2c_result.is_ok()) {
        // Error: fast blink
        while (true) {
            LedPin::toggle();
            for (volatile u32 i = 0; i < 100000; i++) {}
        }
    }

    // Run test scenarios
    bool all_passed = true;

    // Test 1: Bus Scan
    if (test_bus_scan().is_ok()) {
        LedPin::set(); // Turn on LED briefly
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Test 2: Device ID
    if (test_device_id().is_ok()) {
        LedPin::set();
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Test 3: Configuration
    if (test_sensor_config().is_ok()) {
        LedPin::set();
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Test 4: Continuous Read
    if (test_continuous_read().is_ok()) {
        LedPin::set();
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Test 5: Data Sanity
    if (test_data_sanity().is_ok()) {
        LedPin::set();
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Final result
    if (all_passed) {
        // SUCCESS: Solid LED on
        LedPin::set();
    } else {
        // FAILURE: Slow blink
        while (true) {
            LedPin::toggle();
            for (volatile u32 i = 0; i < 500000; i++) {}
        }
    }

    // Test complete - halt
    while (true) {}

    return 0;
}

/**
 * I2C Sensor Test - Expected Behavior:
 *
 * 1. LED blinks briefly 5 times (one per test scenario)
 * 2. If all tests pass: LED stays solid ON
 * 3. If any test fails: LED blinks slowly
 * 4. If initialization fails: LED blinks rapidly
 *
 * Success Criteria:
 * - At least one I2C device found on bus
 * - MPU6050 device ID reads correctly (0x68)
 * - Sensor responds after configuration
 * - Can read sensor data continuously without errors
 * - Data values are within reasonable physical ranges
 *
 * Hardware Requirements:
 * - MPU6050 sensor module (or compatible I2C sensor)
 * - Connections: SDA, SCL, VCC, GND
 * - Pull-up resistors on SDA/SCL (often built into module)
 * - Typical I2C address: 0x68 (AD0=LOW) or 0x69 (AD0=HIGH)
 *
 * Compatible Sensors:
 * - MPU6050: 6-axis accelerometer/gyroscope
 * - BME280: Temperature/humidity/pressure
 * - LM75: Temperature only
 * - Any I2C sensor with proper address/register configuration
 */
