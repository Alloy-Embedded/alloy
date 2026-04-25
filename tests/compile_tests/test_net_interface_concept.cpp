// Compile test: NetworkInterface concept is satisfied by a minimal fake
// implementation. Also verifies EthernetInterface<FakeMac, FakePhy>
// satisfies the concept.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "drivers/net/ethernet_interface.hpp"
#include "drivers/net/network_interface.hpp"
#include "drivers/net/phy_driver.hpp"
#include "src/hal/ethernet/ethernet_mac.hpp"

namespace {

// Minimal NetworkInterface implementation.
struct FakeNetIface {
    [[nodiscard]] alloy::core::Result<void, alloy::net::NetError>
    send_packet(std::span<const std::byte> /*pkt*/) noexcept {
        return alloy::core::Ok();
    }
    [[nodiscard]] alloy::core::Result<std::size_t, alloy::net::NetError>
    recv_packet(std::span<std::byte> /*buf*/) noexcept {
        return alloy::core::Err(alloy::net::NetError::NoPacket);
    }
    [[nodiscard]] bool link_up() const noexcept { return true; }
    [[nodiscard]] std::array<std::uint8_t, 6u> mac_address() const noexcept {
        return {0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u};
    }
};

static_assert(alloy::net::NetworkInterface<FakeNetIface>);

// Verify EthernetInterface<FakeMac, FakePhy> satisfies NetworkInterface.
struct FakeMac {
    [[nodiscard]] alloy::core::Result<void, alloy::hal::ethernet::MacError>
    send_frame(std::span<const std::byte>) noexcept { return alloy::core::Ok(); }
    [[nodiscard]] alloy::core::Result<std::size_t, alloy::hal::ethernet::MacError>
    recv_frame(std::span<std::byte>) noexcept {
        return alloy::core::Err(alloy::hal::ethernet::MacError::NoFrame);
    }
    [[nodiscard]] std::array<std::uint8_t, 6u> get_mac_address() const noexcept {
        return {};
    }
    [[nodiscard]] bool link_up() const noexcept { return true; }
};

struct FakePhy {
    [[nodiscard]] alloy::core::Result<void, alloy::core::ErrorCode>
    reset() { return alloy::core::Ok(); }
    [[nodiscard]] alloy::core::Result<void, alloy::core::ErrorCode>
    auto_negotiate() { return alloy::core::Ok(); }
    [[nodiscard]] alloy::net::LinkStatus link_status() noexcept { return {}; }
};

static_assert(alloy::hal::ethernet::EthernetMac<FakeMac>);
static_assert(alloy::net::PhyDriver<FakePhy>);
static_assert(alloy::net::NetworkInterface<
    alloy::net::EthernetInterface<FakeMac, FakePhy>>);

[[maybe_unused]] void compile_ethernet_interface_usage() {
    FakeMac mac;
    FakePhy phy;
    alloy::net::EthernetInterface iface{mac, phy};
    iface.poll_link();
    std::byte buf[1536]{};
    (void)iface.send_packet(std::span{buf, 64u});
    (void)iface.recv_packet(std::span{buf});
    (void)iface.link_up();
    (void)iface.mac_address();
}

}  // namespace
