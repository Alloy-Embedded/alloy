#pragma once

// alloy::net::EspAtInterface — ESP8266/ESP32 running ESP-AT firmware.
//
// Communicates via UART AT commands. Implements NetworkInterface directly;
// no MAC/PHY split — the ESP module handles all networking internally.
//
// Status: SCAFFOLDED — concept check only.
// Full implementation is deferred. Stub methods return HardwareError.
//
// Template parameter:
//   UartHandle — any type satisfying alloy::hal::uart::port_handle; must expose
//     write(span<const byte>), read(span<byte>) → Result.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/result.hpp"
#include "drivers/net/network_interface.hpp"

namespace alloy::net {

template <typename UartHandle>
class EspAtInterface {
   public:
    explicit EspAtInterface(UartHandle& uart) noexcept : uart_{&uart} {}

    // NetworkInterface::send_packet (stub)
    [[nodiscard]] core::Result<void, NetError>
    send_packet(std::span<const std::byte> /*pkt*/) noexcept {
        return core::Err(NetError::HardwareError);  // TODO: AT+SEND
    }

    // NetworkInterface::recv_packet (stub)
    [[nodiscard]] core::Result<std::size_t, NetError>
    recv_packet(std::span<std::byte> /*buf*/) noexcept {
        return core::Err(NetError::NoPacket);  // TODO: +IPD polling
    }

    [[nodiscard]] bool link_up() const noexcept { return false; }  // TODO: AT+CWJAP?

    [[nodiscard]] std::array<std::uint8_t, 6u> mac_address() const noexcept {
        return mac_address_;
    }

    // Retrieve MAC address via AT+CIPSTAMAC? (stub).
    void fetch_mac() noexcept { /* TODO */ }

    // Join AP: AT+CWJAP="ssid","password" (stub).
    [[nodiscard]] core::Result<void, NetError>
    join_ap(std::string_view /*ssid*/, std::string_view /*pass*/,
            std::uint32_t /*timeout_us*/) noexcept {
        return core::Err(NetError::HardwareError);  // TODO
    }

   private:
    UartHandle* uart_;
    std::array<std::uint8_t, 6u> mac_address_{};
};

// Concept check with a minimal fake UART handle.
namespace _detail {
struct _FakeUart {};
}  // namespace _detail

static_assert(NetworkInterface<EspAtInterface<_detail::_FakeUart>>);

}  // namespace alloy::net
