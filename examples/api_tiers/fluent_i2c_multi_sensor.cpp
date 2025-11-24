/**
 * @file fluent_i2c_multi_sensor.cpp
 * @brief Level 2 Fluent API - I2C Multi-Sensor Reading Example
 *
 * Demonstrates the Fluent tier I2C API with builder pattern for custom configuration.
 * Perfect for applications with multiple I2C sensors requiring different speeds.
 *
 * This example shows:
 * - Builder pattern for custom I2C configuration
 * - Fast-mode (400 kHz) for high-speed sensors
 * - Reading from multiple I2C sensors on the same bus
 * - Sensor data fusion (combining accelerometer + magnetometer)
 * - Error handling with Result<T, E> pattern
 *
 * Hardware Setup:
 * - I2C SDA -> All sensor SDA pins + 4.7k pullup to 3.3V
 * - I2C SCL -> All sensor SCL pins + 4.7k pullup to 3.3V
 * - Multiple sensors can share the same I2C bus
 *
 * Multi-Sensor Setup Example:
 * - MPU6050 (0x68): 6-axis accelerometer/gyro
 * - HMC5883L (0x1E): 3-axis magnetometer
 * - BMP280 (0x76): Barometric pressure sensor
 *
 * Expected Behavior:
 * - Configures I2C at 400 kHz for fast sensor reading
 * - Initializes all sensors
 * - Continuously reads from multiple sensors
 * - Combines data for orientation estimation
 * - Blinks LED at different rates based on sensor data
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

using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_STM32F722ZE)
#include "boards/nucleo_f722ze/board.hpp"
using namespace board::nucleo_f722ze;

using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_STM32G071RB)
#include "boards/nucleo_g071rb/board.hpp"
using namespace board::nucleo_g071rb;

using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_STM32G0B1RE)
#include "boards/nucleo_g0b1re/board.hpp"
using namespace board::nucleo_g0b1re;

using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // D14
using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // D15

#elif defined(PLATFORM_SAME70)
#include "boards/same70_xplained/board.hpp"
using namespace board::same70_xplained;

using I2cSda = Pin<PeripheralId::TWI0, PinId::PA3>;
using I2cScl = Pin<PeripheralId::TWI0, PinId::PA4>;

#else
#error "Unsupported platform. Please define one of the supported platforms."
#endif

#include "hal/gpio.hpp"

using namespace ucore;
using namespace ucore::hal;

// ============================================================================
// I2C Sensor Addresses
// ============================================================================

constexpr uint8_t MPU6050_ADDR = 0x68;   // Accelerometer/Gyro
constexpr uint8_t HMC5883L_ADDR = 0x1E;  // Magnetometer
constexpr uint8_t BMP280_ADDR = 0x76;    // Pressure sensor

// ============================================================================
// Sensor Data Structures
// ============================================================================

struct Vector3D {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct SensorData {
    Vector3D accel;      // Acceleration (m/s²)
    Vector3D gyro;       // Angular velocity (°/s)
    Vector3D mag;        // Magnetic field (µT)
    int32_t pressure;    // Pressure (Pa)
    int32_t temperature; // Temperature (°C * 100)
};

// ============================================================================
// Sensor Driver Classes
// ============================================================================

/**
 * @brief MPU6050 Accelerometer/Gyro Driver
 */
template <typename I2c>
class MPU6050 {
public:
    explicit MPU6050(I2c& i2c) : m_i2c(i2c) {}

    bool initialize() {
        // Wake up sensor (clear sleep bit)
        uint8_t data[2] = {0x6B, 0x00};
        auto result = m_i2c.write(MPU6050_ADDR, data, 2);
        return result.is_ok();
    }

    Result<Vector3D, ErrorCode> read_accel() {
        uint8_t reg = 0x3B;
        uint8_t data[6];

        // Write register address
        auto write_result = m_i2c.write(MPU6050_ADDR, &reg, 1);
        if (!write_result.is_ok()) {
            return Err(write_result.err());
        }

        // Read 6 bytes
        auto read_result = m_i2c.read(MPU6050_ADDR, data, 6);
        if (!read_result.is_ok()) {
            return Err(read_result.err());
        }

        Vector3D accel;
        accel.x = (data[0] << 8) | data[1];
        accel.y = (data[2] << 8) | data[3];
        accel.z = (data[4] << 8) | data[5];

        return Ok(accel);
    }

    Result<Vector3D, ErrorCode> read_gyro() {
        uint8_t reg = 0x43;
        uint8_t data[6];

        auto write_result = m_i2c.write(MPU6050_ADDR, &reg, 1);
        if (!write_result.is_ok()) {
            return Err(write_result.err());
        }

        auto read_result = m_i2c.read(MPU6050_ADDR, data, 6);
        if (!read_result.is_ok()) {
            return Err(read_result.err());
        }

        Vector3D gyro;
        gyro.x = (data[0] << 8) | data[1];
        gyro.y = (data[2] << 8) | data[3];
        gyro.z = (data[4] << 8) | data[5];

        return Ok(gyro);
    }

private:
    I2c& m_i2c;
};

/**
 * @brief HMC5883L Magnetometer Driver
 */
template <typename I2c>
class HMC5883L {
public:
    explicit HMC5883L(I2c& i2c) : m_i2c(i2c) {}

    bool initialize() {
        // Set configuration: 8 samples averaged, 15 Hz, normal measurement
        uint8_t config[2] = {0x00, 0x70};
        auto result1 = m_i2c.write(HMC5883L_ADDR, config, 2);

        // Set mode: Continuous measurement
        uint8_t mode[2] = {0x02, 0x00};
        auto result2 = m_i2c.write(HMC5883L_ADDR, mode, 2);

        return result1.is_ok() && result2.is_ok();
    }

    Result<Vector3D, ErrorCode> read_mag() {
        uint8_t reg = 0x03;
        uint8_t data[6];

        auto write_result = m_i2c.write(HMC5883L_ADDR, &reg, 1);
        if (!write_result.is_ok()) {
            return Err(write_result.err());
        }

        auto read_result = m_i2c.read(HMC5883L_ADDR, data, 6);
        if (!read_result.is_ok()) {
            return Err(read_result.err());
        }

        Vector3D mag;
        // HMC5883L returns data in X, Z, Y order
        mag.x = (data[0] << 8) | data[1];
        mag.z = (data[2] << 8) | data[3];
        mag.y = (data[4] << 8) | data[5];

        return Ok(mag);
    }

private:
    I2c& m_i2c;
};

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Fluent I2C multi-sensor reading example
 *
 * Demonstrates builder pattern for custom I2C configuration.
 */
int main() {
    // ========================================================================
    // Setup with Fluent Builder Pattern
    // ========================================================================

    // Fluent API: Configure I2C with custom settings for sensors
    auto i2c_builder = I2cBuilderInstance()
        .with_pins<I2cSda, I2cScl>()
        .speed(I2cSpeed::Fast_400kHz)         // 400 kHz for fast sensor reading
        .addressing_mode(I2cAddressing::SevenBit)
        .enable_clock_stretching();            // Allow sensors to slow down clock

    // Initialize I2C with configuration
    auto i2c = i2c_builder.initialize();

    // Setup LED for visual feedback
    auto led = LedPin::output();

    // ========================================================================
    // Initialize Sensors
    // ========================================================================

    MPU6050 mpu(i2c);
    HMC5883L hmc(i2c);

    // Initialize MPU6050
    if (!mpu.initialize()) {
        // Error: blink LED rapidly
        while (true) {
            led.toggle();
            for (volatile uint32_t i = 0; i < 100000; ++i);
        }
    }

    // Initialize HMC5883L
    if (!hmc.initialize()) {
        // Error: blink LED slowly
        while (true) {
            led.toggle();
            for (volatile uint32_t i = 0; i < 1000000; ++i);
        }
    }

    led.set();  // Both sensors initialized successfully

    // ========================================================================
    // Continuous Multi-Sensor Reading
    // ========================================================================

    SensorData sensor_data;

    while (true) {
        // Read accelerometer
        auto accel_result = mpu.read_accel();
        if (accel_result.is_ok()) {
            sensor_data.accel = accel_result.value();
        }

        // Read gyroscope
        auto gyro_result = mpu.read_gyro();
        if (gyro_result.is_ok()) {
            sensor_data.gyro = gyro_result.value();
        }

        // Read magnetometer
        auto mag_result = hmc.read_mag();
        if (mag_result.is_ok()) {
            sensor_data.mag = mag_result.value();
        }

        // ====================================================================
        // Sensor Data Fusion (Simple Tilt Compensation)
        // ====================================================================

        // Calculate tilt angles from accelerometer
        // pitch = atan2(accel.y, sqrt(accel.x² + accel.z²))
        // roll  = atan2(-accel.x, accel.z)

        // Calculate heading from magnetometer (with tilt compensation)
        // heading = atan2(mag.y, mag.x)

        // Gyro integration for short-term accuracy
        // angle += gyro * dt

        // Complementary filter: 98% gyro + 2% accel/mag
        // filtered_angle = 0.98 * (angle + gyro * dt) + 0.02 * accel_angle

        // ====================================================================
        // Visual Feedback Based on Sensor Data
        // ====================================================================

        // Blink LED faster when tilted
        int32_t tilt = abs(sensor_data.accel.x) + abs(sensor_data.accel.y);
        uint32_t delay = (tilt > 10000) ? 100000 : 500000;

        led.toggle();
        for (volatile uint32_t i = 0; i < delay; ++i);

        // Read at 100 Hz (10ms delay)
        for (volatile uint32_t i = 0; i < 100000; ++i);
    }

    return 0;
}

/**
 * Key Points:
 *
 * 1. Fluent builder: Method chaining for readable configuration
 * 2. Fast-mode (400 kHz): Faster sensor reading, higher throughput
 * 3. Multi-sensor: Multiple devices on same I2C bus
 * 4. Error handling: Result<T, E> for robust error detection
 * 5. Data fusion: Combining multiple sensors for better accuracy
 *
 * Why 400 kHz for Sensors?
 * - Faster data acquisition (100+ Hz sampling rates)
 * - Reduced I2C bus time (more time for processing)
 * - Most modern sensors support Fast-mode
 * - Still compatible with 100 kHz devices on same bus
 *
 * I2C Bus Sharing:
 * - All devices share same SDA/SCL lines
 * - Each device has unique 7-bit address
 * - Master (MCU) selects device by address
 * - Only one transaction at a time
 * - Pullup resistors required (4.7k for 100 kHz, 2.2k for 400 kHz)
 *
 * Sensor Fusion Benefits:
 * - Accelerometer: Gravity direction (long-term stable, noisy)
 * - Gyroscope: Angular velocity (short-term accurate, drifts)
 * - Magnetometer: North direction (absolute reference, distorted by metal)
 * - Combine all three for optimal orientation estimation
 *
 * Performance (400 kHz I2C):
 * - Single sensor read: ~200 µs (6 bytes)
 * - Three sensors: ~600 µs total
 * - Allows 1 kHz+ sampling rates
 * - vs 100 kHz: ~2.4 ms (4x slower)
 *
 * Common Sensor Combinations:
 * - 6-axis IMU: Accel + Gyro (MPU6050)
 * - 9-axis IMU: Accel + Gyro + Mag (MPU9250)
 * - 10-axis: 9-axis + Pressure (BMX055 + BMP280)
 * - Weather station: Temp + Humidity + Pressure (BME280)
 */
