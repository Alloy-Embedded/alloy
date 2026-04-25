// Compile test: EspAtInterface satisfies NetworkInterface.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"
#include "drivers/net/esp_at/esp_at_interface.hpp"
#include "drivers/net/network_interface.hpp"

namespace {

struct FakeUartHandle {};

static_assert(alloy::net::NetworkInterface<alloy::net::EspAtInterface<FakeUartHandle>>);

[[maybe_unused]] void compile_esp_at_interface_usage() {
    FakeUartHandle uart;
    alloy::net::EspAtInterface<FakeUartHandle> iface{uart};
    std::byte buf[1536]{};
    (void)iface.send_packet(std::span{buf, 64u});
    (void)iface.recv_packet(std::span{buf});
    (void)iface.link_up();
    (void)iface.mac_address();
}

}  // namespace
