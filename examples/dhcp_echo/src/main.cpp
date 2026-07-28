// M2 + DHCP: lease an address from the network's DHCP server, print it, then serve
// a TCP echo on port 7 — the connectivity deliverable from CONNECTIVITY.md (DHCP
// lease + TCP echo on the physical SAME70). up_dhcp() drives DISCOVER/OFFER/REQUEST/
// ACK from the poll loop; has_lease()/ip_address() report the result. The DHCP
// client is host-tested (tests/test_net.cpp::net_dhcp_client_leases); this proves it
// on silicon. Static-IP sibling: examples/tcp_echo. Enabled via [net] dhcp = true in
// alloy.toml, which turns LWIP_DHCP on in the generated lwipopts.h.
#include <alloy/board.hpp>
#include <alloy/net/lwip.hpp>
#include <alloy/net/socket.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
constexpr std::uint8_t kMac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x01};

// Emit a byte as decimal (no libc): the dotted-quad the lease prints as.
template <class Uart>
void write_u8(Uart& uart, std::uint8_t v) {
    char b[3];
    int n = 0;
    do { b[n++] = static_cast<char>('0' + v % 10); v = static_cast<std::uint8_t>(v / 10); } while (v != 0);
    while (n-- > 0) uart.write(static_cast<std::uint8_t>(b[n]));
}
template <class Uart>
void write_ip(Uart& uart, alloy::net::ipv4 ip) {
    for (int i = 0; i < 4; ++i) {
        if (i != 0) uart.write(static_cast<std::uint8_t>('.'));
        write_u8(uart, ip.o[i]);
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy dhcp_echo (lwIP)\r\n");

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
        net.up_dhcp(kMac);
        uart.write("dhcp: requesting lease...\r\n");

        // Announce the lease once, then echo on port 7. The listener starts only
        // after we have an address (before that there's nothing to reply from).
        static alloy::net::tcp_listener<> srv;
        static alloy::net::tcp_socket conn;
        bool announced = false;
        bool listening = false;
        bool have = false;
        std::uint8_t buf[256];
        while (true) {
            net.poll();  // drive lwIP: RX in, DHCP + lease timers, TCP
            if (!announced && net.has_lease()) {
                uart.write("leased: ");
                write_ip(uart, net.ip_address());
                uart.write(" -> nc <ip> 7\r\n");
                announced = true;
            }
            if (announced && !listening) {
                listening = srv.listen(7);
            }
            if (listening) {
                if (!have) {
                    have = srv.accept(conn);
                }
                if (have) {
                    const std::uint32_t n = conn.recv(buf);
                    if (n != 0) {
                        (void)conn.send({buf, n});
                    }
                    if (conn.eof()) {
                        conn.close();
                        have = false;
                    }
                }
            }
        }
    } else {
        uart.write("dhcp_echo: this board has no Ethernet\r\n");
        while (true) {
            alloy::sleep_for(3s);
        }
    }
}
