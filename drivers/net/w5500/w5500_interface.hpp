#pragma once

// alloy::net::W5500Interface — WIZnet W5500 all-in-one TCP/IP module.
//
// The W5500 integrates MAC, PHY, and a hardware TCP/IP stack on-chip. It
// is connected via SPI. This driver implements NetworkInterface directly,
// bypassing the MAC/PHY split used for GMAC + KSZ8081.
//
// Status: SCAFFOLDED — concept check only.
// Full implementation is deferred. Stub methods return HardwareError until
// the SPI register driver is implemented.
//
// Template parameter:
//   SpiHandle — any type satisfying alloy::hal::spi::port_handle; must expose
//     transfer(std::span<const byte> tx, std::span<byte> rx) → Result<void>.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "drivers/net/network_interface.hpp"

namespace alloy::net {

template <typename SpiHandle>
class W5500Interface {
   public:
    explicit W5500Interface(SpiHandle& spi) noexcept : spi_{&spi} {}

    // NetworkInterface::send_packet (stub)
    [[nodiscard]] core::Result<void, NetError>
    send_packet(std::span<const std::byte> /*pkt*/) noexcept {
        return core::Err(NetError::HardwareError);  // TODO: implement W5500 TX
    }

    // NetworkInterface::recv_packet (stub)
    [[nodiscard]] core::Result<std::size_t, NetError>
    recv_packet(std::span<std::byte> /*buf*/) noexcept {
        return core::Err(NetError::NoPacket);  // TODO: implement W5500 RX
    }

    [[nodiscard]] bool link_up() const noexcept { return false; }  // TODO

    [[nodiscard]] std::array<std::uint8_t, 6u> mac_address() const noexcept {
        return mac_address_;
    }

    // Set MAC address before calling init().
    void set_mac_address(std::span<const std::uint8_t, 6u> mac) noexcept {
        for (std::size_t i = 0u; i < 6u; ++i) mac_address_[i] = mac[i];
    }

   private:
    SpiHandle* spi_;
    std::array<std::uint8_t, 6u> mac_address_{};
};

// Concept check with a minimal fake SPI handle.
namespace _detail {
struct _FakeSpi {};
}  // namespace _detail

static_assert(NetworkInterface<W5500Interface<_detail::_FakeSpi>>);

}  // namespace alloy::net
