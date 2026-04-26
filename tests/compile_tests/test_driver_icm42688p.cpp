// Compile test: icm42688p seed driver instantiates against the documented public
// SPI HAL surface. Exercises init()/read() so that any drift in the bus
// handle's signature fails the build.

#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/sensor/icm42688p/icm42688p.hpp"

namespace {

struct MockSpiBus {
    /// Zero-fills the rx buffer and returns Ok — simulates a quiet bus.
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_icm42688p_against_public_spi_handle() {
    MockSpiBus bus;

    // Default construction: NoOpCsPolicy, default Config.
    alloy::drivers::sensor::icm42688p::Device sensor{bus};

    // init() must compile and be callable (result ignored in compile test).
    (void)sensor.init();

    // read() must compile; verify the result type has is_ok().
    auto m = sensor.read();
    (void)m.is_ok();
}

}  // namespace
