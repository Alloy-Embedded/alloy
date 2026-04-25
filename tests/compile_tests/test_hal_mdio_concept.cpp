// Compile test: MdioBus concept is satisfied by a minimal fake bus.
// Verifies the concept definition is self-consistent and that user-supplied
// buses need only expose read/write with the correct signatures.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "src/hal/mdio/mdio.hpp"

namespace {

struct FakeMdioBus {
    [[nodiscard]] alloy::core::Result<std::uint16_t, alloy::core::ErrorCode>
    read(std::uint8_t /*phy*/, std::uint8_t /*reg*/) const noexcept {
        return alloy::core::Ok(std::uint16_t{0u});
    }

    [[nodiscard]] alloy::core::Result<void, alloy::core::ErrorCode>
    write(std::uint8_t /*phy*/, std::uint8_t /*reg*/, std::uint16_t /*value*/) noexcept {
        return alloy::core::Ok();
    }
};

static_assert(alloy::hal::mdio::MdioBus<FakeMdioBus>);

[[maybe_unused]] void compile_mdio_concept_usage() {
    FakeMdioBus bus;
    (void)bus.read(0x00u, 0x01u);
    (void)bus.write(0x00u, 0x00u, 0x0000u);
}

}  // namespace
