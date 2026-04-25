// Compile test: PhyDriver concept is satisfied by KSZ8081::Device when
// paired with a mock MdioBus. Verifies the wrapper methods (reset,
// auto_negotiate, link_status) added to the seed driver are correct.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/net/ksz8081/ksz8081.hpp"
#include "drivers/net/phy_driver.hpp"

namespace {

struct MockMdioBus {
    [[nodiscard]] alloy::core::Result<std::uint16_t, alloy::core::ErrorCode>
    read(std::uint8_t /*phy*/, std::uint8_t reg) const noexcept {
        if (reg == alloy::drivers::net::ksz8081::reg::kPhyId1)
            return alloy::core::Ok(static_cast<std::uint16_t>(
                alloy::drivers::net::ksz8081::kExpectedPhyId1));
        if (reg == alloy::drivers::net::ksz8081::reg::kPhyId2)
            return alloy::core::Ok(static_cast<std::uint16_t>(
                alloy::drivers::net::ksz8081::kExpectedPhyId2Masked | 0x1u));
        return alloy::core::Ok(std::uint16_t{0u});
    }
    [[nodiscard]] alloy::core::Result<void, alloy::core::ErrorCode>
    write(std::uint8_t, std::uint8_t, std::uint16_t) noexcept {
        return alloy::core::Ok();
    }
};

using KszDevice = alloy::drivers::net::ksz8081::Device<MockMdioBus>;

static_assert(alloy::net::PhyDriver<KszDevice>);

[[maybe_unused]] void compile_ksz8081_phy_driver_concept() {
    MockMdioBus bus;
    KszDevice phy{bus, {.reset_poll_max_iterations = 4u}};

    (void)phy.reset();
    (void)phy.auto_negotiate();
    (void)phy.link_status();
}

}  // namespace
