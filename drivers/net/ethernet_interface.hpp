#pragma once

// alloy::net::EthernetInterface — combines an EthernetMac and a PhyDriver
// into a single NetworkInterface.
//
// EthernetInterface<Mac, Phy> wraps:
//   Mac  — any type satisfying alloy::hal::ethernet::EthernetMac
//   Phy  — any type satisfying alloy::net::PhyDriver
//
// poll_link() should be called periodically from the main loop.
// It reads the PHY link status and, on state change, runs auto-negotiation.
//
// send_packet / recv_packet forward to the MAC's send_frame / recv_frame,
// mapping MacError → NetError.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "drivers/net/network_interface.hpp"
#include "drivers/net/phy_driver.hpp"
#include "src/hal/ethernet/ethernet_mac.hpp"

namespace alloy::net {

template <hal::ethernet::EthernetMac Mac, PhyDriver Phy>
class EthernetInterface {
   public:
    EthernetInterface(Mac& mac, Phy& phy) noexcept : mac_{&mac}, phy_{&phy} {}

    // Call from main loop: reads PHY link status; restarts auto-neg on drop.
    void poll_link() noexcept {
        const LinkStatus ls = phy_->link_status();
        if (link_was_up_ && !ls.up) {
            (void)phy_->auto_negotiate();
        }
        link_was_up_ = ls.up;
    }

    // NetworkInterface::send_packet
    [[nodiscard]] core::Result<void, NetError>
    send_packet(std::span<const std::byte> pkt) noexcept {
        if (!link_was_up_) return core::Err(NetError::NotConnected);
        auto r = mac_->send_frame(pkt);
        if (r.is_err()) return core::Err(map_mac_error(r.unwrap_err()));
        return core::Ok();
    }

    // NetworkInterface::recv_packet
    [[nodiscard]] core::Result<std::size_t, NetError>
    recv_packet(std::span<std::byte> buf) noexcept {
        auto r = mac_->recv_frame(buf);
        if (r.is_err()) return core::Err(map_mac_error(r.unwrap_err()));
        return core::Ok(r.unwrap());
    }

    // NetworkInterface::link_up
    [[nodiscard]] bool link_up() const noexcept { return link_was_up_; }

    // NetworkInterface::mac_address
    [[nodiscard]] std::array<std::uint8_t, 6u> mac_address() const noexcept {
        return mac_->get_mac_address();
    }

   private:
    Mac* mac_;
    Phy* phy_;
    bool link_was_up_ = false;

    [[nodiscard]] static constexpr NetError
    map_mac_error(hal::ethernet::MacError e) noexcept {
        switch (e) {
            case hal::ethernet::MacError::Busy:          return NetError::Busy;
            case hal::ethernet::MacError::FrameTooLarge: return NetError::FrameTooLarge;
            case hal::ethernet::MacError::NoFrame:       return NetError::NoPacket;
            default:                                     return NetError::HardwareError;
        }
    }
};

}  // namespace alloy::net
