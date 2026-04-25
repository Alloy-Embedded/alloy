#pragma once

// alloy::net::PhyDriver — Ethernet PHY driver abstraction.
//
// Defines the PhyDriver<T> concept and the LinkStatus struct used across
// all network layers. Any PHY driver (KSZ8081, LAN8742, DP83848, …) that
// satisfies this concept can be used with EthernetInterface.
//
// Concept contract:
//   reset()          — soft-reset the PHY; blocks until reset clears or timeout
//   auto_negotiate() — (re)start auto-negotiation; returns immediately
//   link_status()    — poll current link state; non-failing (absorbs MDIO errors)

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::net {

struct LinkStatus {
    bool     up          = false;
    bool     full_duplex = false;
    std::uint16_t speed_mbps = 0u;  // 10 or 100; 0 when link is down
};

template <typename T>
concept PhyDriver = requires(T& phy) {
    { phy.reset()          } -> std::same_as<core::Result<void, core::ErrorCode>>;
    { phy.auto_negotiate() } -> std::same_as<core::Result<void, core::ErrorCode>>;
    { phy.link_status()    } -> std::same_as<LinkStatus>;
};

}  // namespace alloy::net
