// Compile test: ina3221 seed driver instantiates against the documented public
// I2C HAL surface. Exercises init()/read() so that any drift in
// the bus handle's signature fails the build.
//
// Mock write_read returns [0x54, 0x49, 0x00, 0x00] so that:
//   - MANUFACTURER_ID reads as 0x5449 ("TI") → init() passes the identity check.
//   - Any subsequent 4-byte channel burst reads are zero-filled (valid, current = 0).

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/power/ina3221/ina3221.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t, std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto read(std::uint16_t, std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0; return alloy::core::Ok();
    }
    // Returns 0x5449 in the first two bytes so MANUFACTURER_ID check passes.
    // Remaining bytes (for channel burst reads) are zero-filled.
    [[nodiscard]] auto write_read(std::uint16_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0;
        if (rx.size() >= 2) { rx[0] = 0x54u; rx[1] = 0x49u; }
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_ina3221_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::power::ina3221::Device sensor{bus};
    (void)sensor.init();
    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
