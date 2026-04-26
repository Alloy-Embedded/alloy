// Compile test: lis3mdl seed driver instantiates against the documented public
// I2C HAL surface. Exercises init()/read() so that any drift in
// the bus handle's signature fails the build.
//
// MockI2cBus zero-fills all rx buffers so that:
//   - write_read([WHO_AM_I], [1]) returns 0x00 (WHO_AM_I mismatch → init() returns Err,
//     which is still a valid Result — the test only checks that the call compiles and
//     produces a Result<void, ErrorCode>).
//   - write_read([STATUS_REG], [1]) returns 0x00 (ZYXDA clear → read() polls until
//     Timeout, which is also a valid Result).
// No assertions are made: this is a compile-only smoke test.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/sensor/lis3mdl/lis3mdl.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t, std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto read(std::uint16_t, std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
    [[nodiscard]] auto write_read(std::uint16_t,
                                  std::span<const std::uint8_t>,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_lis3mdl_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::sensor::lis3mdl::Device sensor{bus};
    (void)sensor.init();
    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
