// Compile test: max17048 seed driver instantiates against the documented public
// I2C HAL surface. Exercises init()/read() so that any drift in
// the bus handle's signature fails the build.
//
// Mock write_read returns [0x00, 0x01] so that the VERSION register reads as
// 0x0001 (non-zero), allowing init() to succeed past the presence check.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/power/max17048/max17048.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t, std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto read(std::uint16_t, std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0; return alloy::core::Ok();
    }
    // Returns [0x00, 0x01] so VERSION = 0x0001 (non-zero) → init() passes presence check.
    // CONFIG read also returns 0x0001, which is a valid (non-zero) config value.
    [[nodiscard]] auto write_read(std::uint16_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (rx.size() >= 2) { rx[0] = 0x00u; rx[1] = 0x01u; }
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_max17048_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::power::max17048::Device sensor{bus};
    (void)sensor.init();
    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
