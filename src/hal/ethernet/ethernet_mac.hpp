#pragma once

// alloy::hal::ethernet — Ethernet MAC abstraction.
//
// Defines the EthernetMac<T> concept that Ethernet MAC drivers must satisfy
// and the MacError enum for all MAC-level failures.
//
// Concept contract:
//   send_frame(span<const byte>)  — queue one raw Ethernet frame for TX
//   recv_frame(span<byte>)        — receive one raw Ethernet frame from RX ring
//   get_mac_address()             — return the 6-byte EUI-48 address
//   link_up()                     — return true when the PHY reports link
//
// "Raw frame" means the full Ethernet II frame including dst/src/ethertype and
// payload, but excluding the 4-byte FCS (the MAC appends CRC automatically).
//
// send_frame / recv_frame are cooperative (poll-based). Interrupt-driven
// operation is a future extension; cooperative mode is sufficient for Modbus
// TCP at typical control-loop data rates.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace alloy::hal::ethernet {

enum class MacError : std::uint8_t {
    Busy,          // TX descriptor ring is full; retry later
    FrameTooLarge, // frame exceeds 1518 bytes (standard Ethernet max)
    NoFrame,       // RX ring is empty; no frame available yet
    HardwareError, // DMA or GMAC internal error
};

template <typename T>
concept EthernetMac = requires(T& mac,
                                const T& cmac,
                                std::span<const std::byte> tx,
                                std::span<std::byte>       rx) {
    { mac.send_frame(tx)    } -> std::same_as<core::Result<void, MacError>>;
    { mac.recv_frame(rx)    } -> std::same_as<core::Result<std::size_t, MacError>>;
    { cmac.get_mac_address()} -> std::same_as<std::array<std::uint8_t, 6u>>;
    { cmac.link_up()        } -> std::same_as<bool>;
};

}  // namespace alloy::hal::ethernet
