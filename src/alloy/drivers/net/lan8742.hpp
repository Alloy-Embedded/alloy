// Microchip/SMSC LAN8742(A) Ethernet PHY — generic clause-22 driver.
//
// Constrained on the Mdio concept + a reset OutputPin. No MAC coupling: the
// ETH MAC (or any MAC) presents the MDIO bus, this drives the PHY over it.
// Bring-up: release the active-low hardware reset, confirm the ID
// (0x0007:C13x), soft-reset, advertise 10/100 both duplex, restart
// auto-negotiation. link_up()/speed_100()/full_duplex() read the negotiated
// result the MAC programs into its RMII speed/duplex fields.
//
// Same interface as ksz8081: init()/link_up()/speed_100()/full_duplex(); the
// only PHY-specific facts are the ID and the vendor speed/duplex register.
// Not silicon-validated yet (Ethernet cable pending) — the MDIO ID read is
// cable-free and is the first bring-up checkpoint.

#pragma once

#include <cstdint>

#include "alloy/concepts.hpp"
#include "alloy/time.hpp"

namespace alloy::drivers {

template <class Bus, class ResetPin>
    requires alloy::Mdio<Bus> && alloy::OutputPin<ResetPin>
class lan8742 {
public:
    static constexpr std::uint16_t kId1 = 0x0007;       // OUI high (PHYSID1)
    static constexpr std::uint16_t kId2Masked = 0xC130;  // model (mask 0xFFF0)

    // Clause-22 register numbers used here. PSCSR (0x1F) is the LAN8742 vendor
    // "PHY Special Control/Status" register.
    enum reg : std::uint8_t { BMCR = 0x00, BMSR = 0x01, ID1 = 0x02, ID2 = 0x03,
                              ANAR = 0x04, ANLPAR = 0x05, PSCSR = 0x1F };

    constexpr lan8742(const Bus& bus, const ResetPin& reset, std::uint8_t addr)
        : bus_(bus), reset_(reset), addr_(addr) {}

    // Release reset, verify the part, negotiate. False if the ID is wrong
    // (no PHY / MDIO not working / wrong address).
    [[nodiscard]] bool init() const {
        using namespace alloy::literals;
        reset_.init();
        reset_.set_low();               // assert active-low reset
        alloy::sleep_for(2ms);
        reset_.set_high();              // release
        alloy::sleep_for(2ms);          // LAN8742 needs >=1 ms post-reset

        if (bus_.read(addr_, ID1) != kId1) {
            return false;
        }
        if ((bus_.read(addr_, ID2) & 0xFFF0u) != kId2Masked) {
            return false;
        }
        bus_.write(addr_, BMCR, 0x8000u);  // soft reset (self-clearing)
        for (unsigned i = 0; i < 1000 && (bus_.read(addr_, BMCR) & 0x8000u); ++i) {
        }
        bus_.write(addr_, ANAR, 0x01E1u);  // advertise 10/100, half+full, 802.3
        bus_.write(addr_, BMCR, 0x1200u);  // AN enable | AN restart
        return true;
    }

    [[nodiscard]] bool link_up() const {
        // BMSR link-status latches low: read twice for the live state.
        (void)bus_.read(addr_, BMSR);
        return (bus_.read(addr_, BMSR) & 0x0004u) != 0u;
    }

    // Negotiated result via the LAN8742 PHY Special Control/Status register
    // (reg 0x1F), HCDSPEED field [4:2]: 001=10-HD 010=100-HD 101=10-FD
    // 110=100-FD. (These are the same code points as the KSZ8081's op-mode
    // field, just shifted up two bits into a different register.)
    [[nodiscard]] bool speed_100() const {
        const std::uint16_t mode = (bus_.read(addr_, PSCSR) >> 2) & 0x0007u;
        return mode == 0b010 || mode == 0b110;
    }
    [[nodiscard]] bool full_duplex() const {
        const std::uint16_t mode = (bus_.read(addr_, PSCSR) >> 2) & 0x0007u;
        return mode == 0b101 || mode == 0b110;
    }

private:
    const Bus& bus_;
    const ResetPin& reset_;
    std::uint8_t addr_;
};

}  // namespace alloy::drivers
