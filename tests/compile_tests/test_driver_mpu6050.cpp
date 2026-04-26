// Compile test: mpu6050 seed driver instantiates against the documented public
// I2C HAL surface. Exercises init()/read() so that any drift in the bus handle's
// signature or the driver's register-access helpers fails the build.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/sensor/mpu6050/mpu6050.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t /*address*/,
                             std::span<const std::uint8_t> /*data*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto read(std::uint16_t /*address*/,
                            std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0;
        return alloy::core::Ok();
    }

    [[nodiscard]] auto write_read(std::uint16_t /*address*/,
                                  std::span<const std::uint8_t> /*tx*/,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        // Zero-fill the receive buffer so the read() path never reads
        // uninitialised memory.  For the WHO_AM_I check in init(), we need to
        // return 0x68 so the device-identity guard passes.
        for (auto& b : rx) b = 0;
        if (!rx.empty()) {
            rx[0] = 0x68;  // WHO_AM_I expected value
        }
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_mpu6050_against_public_i2c_handle() {
    MockI2cBus bus;

    // Default config: address 0x68, ±2 g, ±250 dps.
    alloy::drivers::sensor::mpu6050::Device sensor{bus};
    (void)sensor.init();

    auto m = sensor.read();
    (void)m.is_ok();
}

[[maybe_unused]] void compile_mpu6050_with_explicit_config() {
    MockI2cBus bus;

    alloy::drivers::sensor::mpu6050::Config cfg{
        .address     = alloy::drivers::sensor::mpu6050::kSecondaryAddress,
        .accel_range = alloy::drivers::sensor::mpu6050::AccelRange::G16,
        .gyro_range  = alloy::drivers::sensor::mpu6050::GyroRange::Dps2000,
    };
    alloy::drivers::sensor::mpu6050::Device sensor{bus, cfg};
    (void)sensor.init();

    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
