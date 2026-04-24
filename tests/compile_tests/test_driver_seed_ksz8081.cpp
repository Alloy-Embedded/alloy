// Compile test: KSZ8081 seed driver instantiates against a user-supplied MDIO
// handle matching the documented handle contract (read / write returning
// core::Result). Alloy does not ship an MDIO HAL today, so the driver is
// intentionally templated over this contract rather than over a public HAL
// type.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/net/ksz8081/ksz8081.hpp"

namespace {

struct MockMdioBus {
    [[nodiscard]] auto read(std::uint8_t /*phy*/, std::uint8_t reg) const
        -> alloy::core::Result<std::uint16_t, alloy::core::ErrorCode> {
        // Return the expected KSZ8081 PHY ID on the ID registers so init()
        // succeeds end-to-end under the mock.
        if (reg == alloy::drivers::net::ksz8081::reg::kPhyId1) {
            return alloy::core::Ok(
                static_cast<std::uint16_t>(alloy::drivers::net::ksz8081::kExpectedPhyId1));
        }
        if (reg == alloy::drivers::net::ksz8081::reg::kPhyId2) {
            return alloy::core::Ok(static_cast<std::uint16_t>(
                alloy::drivers::net::ksz8081::kExpectedPhyId2Masked | 0x0001));
        }
        return alloy::core::Ok(static_cast<std::uint16_t>(0));
    }

    [[nodiscard]] auto write(std::uint8_t /*phy*/, std::uint8_t /*reg*/,
                             std::uint16_t /*value*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_ksz8081_against_user_mdio_handle() {
    MockMdioBus bus;
    alloy::drivers::net::ksz8081::Device phy{bus, {.phy_address = 0x01,
                                                   .advertise = 0x01E1,
                                                   .reset_poll_max_iterations = 4u}};

    (void)phy.init();
    (void)phy.soft_reset();
    (void)phy.restart_auto_negotiation();
    (void)phy.read_link_status();
    (void)phy.read_phy_id();
}

}  // namespace
