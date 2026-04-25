// Compile test: EthernetMac concept is satisfied by a minimal fake MAC.
// Also verifies MacError enum values are accessible.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "src/hal/ethernet/ethernet_mac.hpp"

namespace {

struct FakeEthernetMac {
    [[nodiscard]] alloy::core::Result<void, alloy::hal::ethernet::MacError>
    send_frame(std::span<const std::byte> /*buf*/) noexcept {
        return alloy::core::Ok();
    }

    [[nodiscard]] alloy::core::Result<std::size_t, alloy::hal::ethernet::MacError>
    recv_frame(std::span<std::byte> /*buf*/) noexcept {
        return alloy::core::Err(alloy::hal::ethernet::MacError::NoFrame);
    }

    [[nodiscard]] std::array<std::uint8_t, 6u> get_mac_address() const noexcept {
        return {0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u};
    }

    [[nodiscard]] bool link_up() const noexcept { return true; }
};

static_assert(alloy::hal::ethernet::EthernetMac<FakeEthernetMac>);

[[maybe_unused]] void compile_ethernet_mac_concept_usage() {
    FakeEthernetMac mac;
    std::byte tx_buf[64]{};
    std::byte rx_buf[1536]{};
    (void)mac.send_frame(std::span{tx_buf});
    (void)mac.recv_frame(std::span{rx_buf});
    (void)mac.get_mac_address();
    (void)mac.link_up();
}

}  // namespace
