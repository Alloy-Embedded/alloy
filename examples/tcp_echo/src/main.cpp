// M2: a real TCP/IP stack. Where net_echo (M1) hand-answered ARP+ICMP to prove
// the GMAC, this runs vendored lwIP on top of the SAME NetDevice and serves a
// TCP echo on port 7 — `nc 192.168.15.231 7` and what you type comes back. Proof
// of lwIP + the netif<->NetDevice seam end to end. Static IP for v1 (DHCP next).
#include <alloy/board.hpp>
#include <alloy/net/lwip.hpp>

#include "lwip/tcp.h"

using namespace alloy::literals;

namespace {
constexpr std::uint8_t kMac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x01};

// Raw lwIP TCP callbacks: echo whatever arrives back to the sender.
err_t echo_recv(void* /*arg*/, struct tcp_pcb* pcb, struct pbuf* p, err_t /*err*/) {
    if (p == nullptr) {  // remote closed
        tcp_close(pcb);
        return ERR_OK;
    }
    for (struct pbuf* q = p; q != nullptr; q = q->next) {
        tcp_write(pcb, q->payload, q->len, TCP_WRITE_FLAG_COPY);
    }
    tcp_output(pcb);
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

err_t echo_accept(void* /*arg*/, struct tcp_pcb* pcb, err_t /*err*/) {
    tcp_recv(pcb, echo_recv);
    return ERR_OK;
}
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

        struct tcp_pcb* srv = tcp_new();
        tcp_bind(srv, IP_ADDR_ANY, 7);
        srv = tcp_listen(srv);
        tcp_accept(srv, echo_accept);

        while (true) {
            net.poll();
        }
    } else {
        uart.write("tcp_echo: this board has no Ethernet\r\n");
        while (true) {
            alloy::sleep_for(3s);
        }
    }
}
