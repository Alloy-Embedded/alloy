// Compile test: sht4x seed driver instantiates against the documented public
// I2C HAL surface. Exercises init()/read() so that any drift in
// the bus handle's signature fails the build.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/sensor/sht4x/sht4x.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t, std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto read(std::uint16_t, std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0; return alloy::core::Ok();
    }
    [[nodiscard]] auto write_read(std::uint16_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0; return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_sht4x_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::sensor::sht4x::Device sensor{bus};
    (void)sensor.init();
    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
