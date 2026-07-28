// M2: a real TCP/IP stack. Where net_echo (M1) hand-answered ARP+ICMP to prove
// the GMAC, this runs vendored lwIP on top of the SAME NetDevice and serves a
// TCP echo on port 7 — `nc 192.168.15.231 7` and what you type comes back. User
// code sees only alloy::net (stack + Socket facade), never lwIP directly. Static
// IP for v1 (DHCP next). The netif<->NetDevice glue + the Socket facade are
// host-tested (tests/test_net.cpp); this proves them on silicon.
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
    uart.write("\r\nalloy tcp_echo (lwIP)\r\n");

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
        uart.write("ready: nc 192.168.15.231 7\r\n");

        // Echo one connection at a time through the Socket facade.
        static alloy::net::tcp_listener<> srv;
        static alloy::net::tcp_socket conn;
        srv.listen(7);
        bool have = false;
        std::uint8_t buf[256];
        while (true) {
            net.poll();  // drive lwIP (RX in, timers)
            if (!have) {
                have = srv.accept(conn);
            }
            if (have) {
                const std::uint32_t n = conn.recv(buf);
                if (n != 0) {
                    (void)conn.send({buf, n});
                }
                if (conn.eof()) {  // peer closed + drained -> ready for the next
                    conn.close();
                    have = false;
                }
            }
        }
    } else {
        uart.write("tcp_echo: this board has no Ethernet\r\n");
        while (true) {
            alloy::sleep_for(3s);
        }
    }
}
