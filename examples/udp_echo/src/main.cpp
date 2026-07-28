// M2, datagram edition: the connectionless sibling of tcp_echo. Same vendored
// lwIP on the same NetDevice (the GMAC), but a UDP echo on port 7 — send it a
// datagram with `nc -u 192.168.15.231 7` and it comes straight back to whoever
// sent it. User code sees only alloy::net (stack + the udp_socket facade), never
// lwIP. Static IP for v1. Unlike TCP there's no connection lifecycle: bind once,
// then recv_from/send_to in the poll loop. The facade is host-tested over lwIP
// loopback (tests/test_net.cpp); this proves it on silicon.
#include <alloy/board.hpp>
#include <alloy/net/lwip.hpp>
#include <alloy/net/socket.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
constexpr std::uint8_t kMac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x01};
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy udp_echo (lwIP)\r\n");

    if constexpr (board::caps::ethernet) {
        board::eth_configure_pins();
        board::eth.begin_mdio();
        if (!board::eth_phy.init()) {
            uart.write("phy init failed\r\n");
        }
        for (unsigned i = 0; i < 50 && !board::eth_phy.link_up(); ++i) {
            alloy::sleep_for(100ms);
        }
        uart.write(board::eth_phy.link_up() ? "link: UP\r\n" : "link: down (plug cable)\r\n");
        board::eth.start(kMac, board::eth_phy.speed_100(), board::eth_phy.full_duplex());

        static alloy::net::stack<decltype(board::eth)> net{board::eth};
        net.up({192, 168, 15, 231}, {255, 255, 255, 0}, {192, 168, 15, 1}, kMac);
        uart.write("ready: nc -u 192.168.15.231 7\r\n");

        // Connectionless echo: one datagram in, the same datagram back to its
        // sender. No accept/close lifecycle — just bind, then recv_from/send_to.
        static alloy::net::udp_socket<> sock;
        (void)sock.bind(7);
        std::uint8_t buf[1472];  // max UDP payload over a 1500-byte MTU
        while (true) {
            net.poll();  // drive lwIP (RX in, timers)
            alloy::ip_endpoint peer{};
            const std::uint32_t n = sock.recv_from(buf, peer);
            if (n != 0) {
                (void)sock.send_to({buf, n}, peer);  // echo straight back to the sender
            }
        }
    } else {
        uart.write("udp_echo: this board has no Ethernet\r\n");
        while (true) {
            alloy::sleep_for(3s);
        }
    }
}
