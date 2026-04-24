#pragma once

// drivers/net/ksz8081/ksz8081.hpp
//
// Driver for Microchip KSZ8081RNA/KSZ8081RND 10/100 Ethernet PHY accessed via
// MDIO (IEEE 802.3 clause 22).
// Written against datasheet revision 1.4 (Microchip DS00002202E, 2017).
// Seed driver: PHY-ID check, soft reset, auto-negotiation kickoff, and a
// link-status read that reports speed + duplex. Vendor-specific interrupt
// and wake-on-LAN features are out of scope.
//
// Transport contract — alloy does not yet ship an MDIO HAL, so this driver is
// templated over a user-supplied `MdioBus` handle. The handle must expose:
//
//   auto read(uint8_t phy_address, uint8_t reg) const
//       -> alloy::core::Result<uint16_t, alloy::core::ErrorCode>;
//   auto write(uint8_t phy_address, uint8_t reg, uint16_t value) const
//       -> alloy::core::Result<void, alloy::core::ErrorCode>;
//
// The driver never holds ownership of the bus and performs no dynamic
// allocation. See drivers/README.md.

#include <cstdint>
#include <utility>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::net::ksz8081 {

// Standard MII register map (IEEE 802.3 clause 22).
namespace reg {
inline constexpr std::uint8_t kBasicControl = 0x00;
inline constexpr std::uint8_t kBasicStatus = 0x01;
inline constexpr std::uint8_t kPhyId1 = 0x02;
inline constexpr std::uint8_t kPhyId2 = 0x03;
inline constexpr std::uint8_t kAutoNegAdvertise = 0x04;
inline constexpr std::uint8_t kAutoNegLinkPartner = 0x05;
// KSZ8081 vendor-specific registers used by the seed driver.
inline constexpr std::uint8_t kPhyControl1 = 0x1E;
}  // namespace reg

namespace control_bit {
inline constexpr std::uint16_t kSoftReset = 1u << 15;
inline constexpr std::uint16_t kRestartAutoNeg = 1u << 9;
inline constexpr std::uint16_t kEnableAutoNeg = 1u << 12;
inline constexpr std::uint16_t kSpeedSelect100 = 1u << 13;
inline constexpr std::uint16_t kFullDuplex = 1u << 8;
}  // namespace control_bit

namespace status_bit {
inline constexpr std::uint16_t kLinkUp = 1u << 2;
inline constexpr std::uint16_t kAutoNegComplete = 1u << 5;
}  // namespace status_bit

// Per datasheet §5.1.5: PHY ID1 = 0x0022, PHY ID2 = 0x1561 (KSZ8081RNA/RND).
// The low four bits of PHY ID2 encode silicon revision, so the driver masks
// them out of the check.
inline constexpr std::uint16_t kExpectedPhyId1 = 0x0022;
inline constexpr std::uint16_t kExpectedPhyId2Masked = 0x1560;
inline constexpr std::uint16_t kPhyId2RevisionMask = 0xFFF0;

inline constexpr std::uint8_t kDefaultPhyAddress = 0x00;

struct Config {
    std::uint8_t phy_address = kDefaultPhyAddress;
    // Auto-negotiation advertisement. Default: 100BASE-TX full & half duplex,
    // 10BASE-T full & half duplex, IEEE 802.3 selector (0x01).
    std::uint16_t advertise = 0x01E1;
    // Upper bound on soft-reset polling iterations before returning Timeout.
    std::uint32_t reset_poll_max_iterations = 100'000u;
};

struct LinkStatus {
    bool up;
    bool auto_negotiation_complete;
    std::uint16_t speed_mbps;  // 10, 100, or 0 (link down / unknown)
    bool full_duplex;
};

template <typename MdioBus>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultU16 = alloy::core::Result<std::uint16_t, alloy::core::ErrorCode>;
    using ResultStatus = alloy::core::Result<LinkStatus, alloy::core::ErrorCode>;

    explicit Device(MdioBus& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    // Verifies PHY ID, issues a soft reset, waits for the reset bit to clear,
    // programs the advertisement register, and kicks off auto-negotiation.
    [[nodiscard]] auto init() -> ResultVoid {
        auto id1 = bus_->read(cfg_.phy_address, reg::kPhyId1);
        if (id1.is_err()) {
            return alloy::core::Err(std::move(id1).err());
        }
        if (id1.unwrap() != kExpectedPhyId1) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        auto id2 = bus_->read(cfg_.phy_address, reg::kPhyId2);
        if (id2.is_err()) {
            return alloy::core::Err(std::move(id2).err());
        }
        if ((id2.unwrap() & kPhyId2RevisionMask) != kExpectedPhyId2Masked) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        if (auto r = soft_reset(); r.is_err()) {
            return r;
        }

        if (auto r = bus_->write(cfg_.phy_address, reg::kAutoNegAdvertise, cfg_.advertise);
            r.is_err()) {
            return r;
        }
        return restart_auto_negotiation();
    }

    [[nodiscard]] auto soft_reset() -> ResultVoid {
        if (auto r = bus_->write(cfg_.phy_address, reg::kBasicControl,
                                 control_bit::kSoftReset);
            r.is_err()) {
            return r;
        }
        // Reset bit self-clears when the PHY finishes internal reset (§3.2.7).
        for (std::uint32_t i = 0; i < cfg_.reset_poll_max_iterations; ++i) {
            auto v = bus_->read(cfg_.phy_address, reg::kBasicControl);
            if (v.is_err()) {
                return alloy::core::Err(std::move(v).err());
            }
            if ((v.unwrap() & control_bit::kSoftReset) == 0) {
                return alloy::core::Ok();
            }
        }
        return alloy::core::Err(alloy::core::ErrorCode::Timeout);
    }

    [[nodiscard]] auto restart_auto_negotiation() -> ResultVoid {
        auto bc = bus_->read(cfg_.phy_address, reg::kBasicControl);
        if (bc.is_err()) {
            return alloy::core::Err(std::move(bc).err());
        }
        const std::uint16_t new_bc = static_cast<std::uint16_t>(
            bc.unwrap() | control_bit::kEnableAutoNeg | control_bit::kRestartAutoNeg);
        return bus_->write(cfg_.phy_address, reg::kBasicControl, new_bc);
    }

    [[nodiscard]] auto read_link_status() -> ResultStatus {
        auto bs = bus_->read(cfg_.phy_address, reg::kBasicStatus);
        if (bs.is_err()) {
            return alloy::core::Err(std::move(bs).err());
        }
        const std::uint16_t status = bs.unwrap();
        // Reading Basic Status twice clears latched link-down — caller can
        // re-read if they want the instantaneous state.

        LinkStatus out{};
        out.up = (status & status_bit::kLinkUp) != 0;
        out.auto_negotiation_complete = (status & status_bit::kAutoNegComplete) != 0;

        // Derive speed + duplex from the link partner ability if A/N complete,
        // else from Basic Control (our own forced configuration).
        auto partner = bus_->read(cfg_.phy_address, reg::kAutoNegLinkPartner);
        auto basic = bus_->read(cfg_.phy_address, reg::kBasicControl);
        if (partner.is_err()) {
            return alloy::core::Err(std::move(partner).err());
        }
        if (basic.is_err()) {
            return alloy::core::Err(std::move(basic).err());
        }

        if (out.auto_negotiation_complete && out.up) {
            const std::uint16_t lp = partner.unwrap();
            if (lp & (1u << 8)) {
                out.speed_mbps = 100;
                out.full_duplex = true;
            } else if (lp & (1u << 7)) {
                out.speed_mbps = 100;
                out.full_duplex = false;
            } else if (lp & (1u << 6)) {
                out.speed_mbps = 10;
                out.full_duplex = true;
            } else if (lp & (1u << 5)) {
                out.speed_mbps = 10;
                out.full_duplex = false;
            } else {
                out.speed_mbps = 0;
                out.full_duplex = false;
            }
        } else {
            const std::uint16_t bc = basic.unwrap();
            out.speed_mbps = (bc & control_bit::kSpeedSelect100) ? 100 : 10;
            out.full_duplex = (bc & control_bit::kFullDuplex) != 0;
        }
        return alloy::core::Ok(std::move(out));
    }

    [[nodiscard]] auto read_phy_id() -> ResultU16 {
        return bus_->read(cfg_.phy_address, reg::kPhyId2);
    }

private:
    MdioBus* bus_;
    Config cfg_;
};

}  // namespace alloy::drivers::net::ksz8081
