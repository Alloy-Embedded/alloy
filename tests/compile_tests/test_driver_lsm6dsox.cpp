// Compile test: LSM6DSOX seed driver instantiates against the documented public
// I2C HAL surface. Exercises init()/read() so the driver's register-access
// helpers stay pinned to `write()` / `write_read()` from the public I2C API.
//
// The mock write_read returns 0x6C in rx[0] so that the WHO_AM_I check inside
// init() succeeds and the full init sequence is exercised.
// The mock also sets kStatusXlda | kStatusGda | kStatusTda (0x07) in the status
// byte so the data-ready poll inside read() passes immediately.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/sensor/lsm6dsox/lsm6dsox.hpp"

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
                                  std::span<const std::uint8_t> tx,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0;
        if (!tx.empty() && !rx.empty()) {
            // WHO_AM_I register (0x0F) → return 0x6C so init() passes.
            if (tx[0] == 0x0Fu) {
                rx[0] = 0x6Cu;
            }
            // STATUS_REG (0x1E) → set XLDA | GDA | TDA so read() poll passes.
            if (tx[0] == 0x1Eu) {
                rx[0] = 0x07u;
            }
            // OUT_TEMP_L burst (0x20) → all zeros (benign default measurement).
        }
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_lsm6dsox_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::sensor::lsm6dsox::Device sensor{
        bus,
        {.address  = alloy::drivers::sensor::lsm6dsox::kDefaultAddress,
         .accel_fs = alloy::drivers::sensor::lsm6dsox::AccelFullScale::G2,
         .gyro_fs  = alloy::drivers::sensor::lsm6dsox::GyroFullScale::Dps250}};

    (void)sensor.init();
    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
