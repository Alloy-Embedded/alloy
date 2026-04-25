// Compile test: W5500Interface satisfies NetworkInterface.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "drivers/net/network_interface.hpp"
#include "drivers/net/w5500/w5500_interface.hpp"

namespace {

struct FakeSpiHandle {};

static_assert(alloy::net::NetworkInterface<alloy::net::W5500Interface<FakeSpiHandle>>);

[[maybe_unused]] void compile_w5500_interface_usage() {
    FakeSpiHandle spi;
    alloy::net::W5500Interface<FakeSpiHandle> iface{spi};
    std::byte buf[1536]{};
    (void)iface.send_packet(std::span{buf, 64u});
    (void)iface.recv_packet(std::span{buf});
    (void)iface.link_up();
    (void)iface.mac_address();
}

}  // namespace
