// Host-side lwIP loopback test.
// Uses lwIP's built-in LOOPIF (127.0.0.1) to send data through
// LwipAdapter → TcpStream (client) → TcpListener → TcpStream (server).
//
// Requires: ALLOY_LWIP_AVAILABLE (link alloy::net_lwip).
//
// Run: ctest -R test_net_lwip_loopback

#ifdef ALLOY_LWIP_AVAILABLE

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "core/result.hpp"
#include "drivers/net/lwip/lwip_adapter.hpp"
#include "drivers/net/network_interface.hpp"

// ---------------------------------------------------------------------------
// LoopbackInterface: a NetworkInterface that routes packets through lwIP's
// loopback adapter (no physical hardware required).
// ---------------------------------------------------------------------------

class LoopbackInterface {
   public:
    [[nodiscard]] alloy::core::Result<void, alloy::net::NetError>
    send_packet(std::span<const std::byte> /*pkt*/) noexcept {
        // lwIP loopback interface handles its own routing — this path is
        // never called for localhost connections.
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

static_assert(alloy::net::NetworkInterface<LoopbackInterface>);

// ---------------------------------------------------------------------------

TEST_CASE("LwipAdapter loopback: TcpListener accepts TcpStream") {
    LoopbackInterface lo;
    alloy::net::LwipAdapter<LoopbackInterface> adapter{lo};

    // Static IP on the loopback subnet (127.0.0.1).
    adapter.init(
        {127u, 0u, 0u, 1u},   // ip
        {255u, 0u, 0u, 0u},   // mask
        {127u, 0u, 0u, 1u},   // gw
        /* use_dhcp */ false);

    // Server: listen on port 9999.
    auto listener_r = adapter.listen(9999u);
    REQUIRE(listener_r.is_ok());
    auto& listener = listener_r.unwrap();

    // Client: connect to 127.0.0.1:9999.
    auto client_r = adapter.connect({127u, 0u, 0u, 1u}, 9999u, 500'000u);
    REQUIRE(client_r.is_ok());
    auto& client = client_r.unwrap();

    // Server: accept the connection.
    auto server_r = listener.accept(200'000u);
    REQUIRE(server_r.is_ok());
    auto& server = server_r.unwrap();

    // Client → Server.
    constexpr std::string_view kMsg = "Hello Alloy";
    {
        auto sv = std::span{reinterpret_cast<const std::byte*>(kMsg.data()), kMsg.size()};
        REQUIRE(client.write(sv).is_ok());
        REQUIRE(client.flush(0u).is_ok());
    }

    // Server: read.
    std::byte rx[64]{};
    auto n_r = server.read(std::span{rx}, 200'000u);
    REQUIRE(n_r.is_ok());
    REQUIRE(n_r.unwrap() == kMsg.size());
    const std::string_view received{reinterpret_cast<const char*>(rx), n_r.unwrap()};
    REQUIRE(received == kMsg);
}

TEST_CASE("LwipAdapter loopback: modbus ByteStream concept via TcpStream") {
    // Verify TcpStream satisfies ByteStream at compile time (no runtime work).
    static_assert(alloy::modbus::ByteStream<alloy::net::TcpStream>);
    SUCCEED("TcpStream satisfies modbus::ByteStream");
}

#else

// Stub when lwIP is not available so the file compiles.
#include <catch2/catch_test_macros.hpp>
TEST_CASE("lwIP loopback skipped: ALLOY_LWIP_AVAILABLE not defined") {
    SUCCEED("skipped");
}

#endif  // ALLOY_LWIP_AVAILABLE
