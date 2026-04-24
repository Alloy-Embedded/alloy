// Compile test: BME280 seed driver instantiates against the documented public
// I2C HAL surface. Exercises init()/read() so the driver's register-access
// helpers stay pinned to `write()` / `write_read()` from the public I2C API.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/sensor/bme280/bme280.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t /*address*/,
                             std::span<const std::uint8_t> /*data*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto read(std::uint16_t /*address*/,
                            std::span<std::uint8_t> data) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : data) b = 0;
        return alloy::core::Ok();
    }

    [[nodiscard]] auto write_read(std::uint16_t /*address*/,
                                  std::span<const std::uint8_t> /*tx*/,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        // Populate with benign zero calibration + a plausible chip ID so the
        // compile test exercises the full init path without tripping the ID
        // check.
        for (auto& b : rx) b = 0;
        if (!rx.empty()) {
            rx[0] = 0x60;  // BME280 chip ID
        }
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_bme280_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::sensor::bme280::Device sensor{
        bus,
        {.address = alloy::drivers::sensor::bme280::kPrimaryAddress,
         .osrs_temperature = alloy::drivers::sensor::bme280::Oversampling::X1,
         .osrs_pressure = alloy::drivers::sensor::bme280::Oversampling::X1,
         .osrs_humidity = alloy::drivers::sensor::bme280::Oversampling::X1,
         .filter = alloy::drivers::sensor::bme280::Filter::Off,
         .mode = alloy::drivers::sensor::bme280::Mode::Normal}};

    (void)sensor.init();
    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
