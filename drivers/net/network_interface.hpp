#pragma once

// alloy::net::NetworkInterface — hardware-agnostic network packet interface.
//
// Both Ethernet (MAC + PHY) and WiFi modules (W5500, ESP-AT) implement
// this concept. The TCP/IP stack (LwipAdapter) is templated over it and
// never sees MAC/PHY details.
//
// Concept contract:
//   send_packet(span<const byte>) — transmit one IP packet (raw L3 frame)
//   recv_packet(span<byte>)       — receive one IP packet; NoPacket when empty
//   link_up()                     — true when the physical link is established
//   mac_address()                 — 6-byte EUI-48 address of this interface

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace alloy::net {

enum class NetError : std::uint8_t {
    Busy,          // TX buffer full; retry
    NoPacket,      // RX queue empty
    FrameTooLarge, // packet exceeds interface MTU
    HardwareError, // peripheral or driver fault
    NotConnected,  // link is down or association failed (WiFi)
};

template <typename T>
concept NetworkInterface = requires(T& iface,
                                     const T& ciface,
                                     std::span<const std::byte> tx,
                                     std::span<std::byte>       rx) {
    { iface.send_packet(tx)   } -> std::same_as<core::Result<void, NetError>>;
    { iface.recv_packet(rx)   } -> std::same_as<core::Result<std::size_t, NetError>>;
    { ciface.link_up()        } -> std::same_as<bool>;
    { ciface.mac_address()    } -> std::same_as<std::array<std::uint8_t, 6u>>;
};

}  // namespace alloy::net
