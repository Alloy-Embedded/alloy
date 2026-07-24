// Ethernet bring-up (M1): read the PHY over MDIO (CABLE-FREE checkpoint),
// bring up the link, and answer ARP + ICMP echo so the board is pingable.
// No lwIP — a hand-written responder proves the GMAC RX/TX + PHY + the
// NetDevice concept end to end; the real TCP/IP stack (lwIP behind the
// Socket concept) is M2. Zero #ifdefs: this compiles on every board, and
// runs the network path only where board::caps::ethernet.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

template <class Uart>
void hex8(const Uart& u, std::uint8_t v) {
    constexpr char d[] = "0123456789abcdef";
    u.write(static_cast<std::uint8_t>(d[v >> 4]));
    u.write(static_cast<std::uint8_t>(d[v & 0xF]));
}
template <class Uart>
void hex16(const Uart& u, std::uint16_t v) {
    hex8(u, static_cast<std::uint8_t>(v >> 8));
    hex8(u, static_cast<std::uint8_t>(v));
}

// Static station identity (M1: no DHCP yet — that's the lwIP milestone).
constexpr std::uint8_t kMac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x01};
constexpr std::uint8_t kIp[4] = {192, 168, 1, 231};

std::uint16_t rd16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}
void wr16(std::uint8_t* p, std::uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v >> 8);
    p[1] = static_cast<std::uint8_t>(v);
}
// Internet checksum over a byte range (RFC 1071).
std::uint16_t inet_csum(const std::uint8_t* p, std::uint32_t n) {
    std::uint32_t sum = 0;
    for (std::uint32_t i = 0; i + 1 < n; i += 2) {
        sum += rd16(p + i);
    }
    if (n & 1u) {
        sum += static_cast<std::uint16_t>(p[n - 1] << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return static_cast<std::uint16_t>(~sum);
}

// Handle one received frame: reply to ARP requests for our IP and to ICMP
// echo requests. `f` is modified in place and re-transmitted.
template <class Eth>
void handle_frame(Eth& eth, std::uint8_t* f, std::uint32_t len) {
    if (len < 42) {
        return;
    }
    const std::uint16_t ethertype = rd16(f + 12);

    if (ethertype == 0x0806 && rd16(f + 20) == 1) {  // ARP request
        bool for_us = true;
        for (int i = 0; i < 4; ++i) {
            for_us = for_us && f[38 + i] == kIp[i];  // target protocol addr
        }
        if (!for_us) {
            return;
        }
        // Build the reply in place: swap MAC, set opcode=2, fill sender.
        for (int i = 0; i < 6; ++i) {
            f[i] = f[6 + i];       // dst = old src
            f[6 + i] = kMac[i];    // src = us
        }
        wr16(f + 20, 2);           // ARP reply
        for (int i = 0; i < 6; ++i) {
            f[32 + i] = f[22 + i]; // target hw = old sender hw
            f[22 + i] = kMac[i];   // sender hw = us
        }
        for (int i = 0; i < 4; ++i) {
            std::uint8_t t = f[28 + i];
            f[38 + i] = t;         // target proto = old sender proto
            f[28 + i] = kIp[i];    // sender proto = us
        }
        (void)eth.transmit({f, 42});
        return;
    }

    if (ethertype == 0x0800 && f[23] == 1) {  // IPv4 + ICMP
        const std::uint32_t ihl = (f[14] & 0x0F) * 4u;
        std::uint8_t* icmp = f + 14 + ihl;
        if (icmp[0] != 8) {  // not an echo request
            return;
        }
        // Swap MACs and IPs, turn the request into a reply, recompute sums.
        for (int i = 0; i < 6; ++i) {
            f[i] = f[6 + i];     // dst = original sender
            f[6 + i] = kMac[i];  // src = us
        }
        for (int i = 0; i < 4; ++i) {
            std::uint8_t t = f[26 + i];   // src IP
            f[26 + i] = kIp[i];           // src = us
            f[30 + i] = t;                // dst = original sender
        }
        icmp[0] = 0;                      // echo reply
        icmp[2] = icmp[3] = 0;            // clear ICMP checksum
        const std::uint32_t icmp_len = len - 14 - ihl;
        wr16(icmp + 2, inet_csum(icmp, icmp_len));
        f[24] = f[25] = 0;                // clear IP header checksum
        wr16(f + 24, inet_csum(f + 14, ihl));
        (void)eth.transmit({f, len});
    }
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy net_echo\r\n");

    if constexpr (board::caps::ethernet) {
        board::eth_configure_pins();
        board::eth.begin_mdio();

        // CABLE-FREE checkpoint: read the PHY ID over MDIO. A correct
        // 0022:15xx proves clocks + MDC divider + MAN sequence, no cable.
        const std::uint16_t id1 = board::eth.read(board::eth_phy_addr, 2);
        const std::uint16_t id2 = board::eth.read(board::eth_phy_addr, 3);
        uart.write("phy id: ");
        hex16(uart, id1);
        uart.write(":");
        hex16(uart, id2);
        uart.write((id1 == 0x0022 && (id2 & 0xFFF0) == 0x1560)
                       ? "  (KSZ8081 OK)\r\n"
                       : "  (unexpected!)\r\n");

        if (!board::eth_phy.init()) {
            uart.write("phy init failed\r\n");
        }
        // Wait for link (needs the Ethernet cable), then go live.
        for (unsigned i = 0; i < 50 && !board::eth_phy.link_up(); ++i) {
            alloy::sleep_for(100ms);
        }
        const bool up = board::eth_phy.link_up();
        uart.write(up ? "link: UP\r\n" : "link: down (plug the cable)\r\n");
        board::eth.start(kMac, board::eth_phy.speed_100(), board::eth_phy.full_duplex());
        uart.write("ready: ping 192.168.1.231\r\n");

        static std::uint8_t frame[1536];
        while (true) {
            const std::uint32_t n = board::eth.receive({frame, sizeof(frame)});
            if (n != 0) {
                handle_frame(board::eth, frame, n);
            }
        }
    } else {
        uart.write("net_echo: this board has no Ethernet\r\n");
        while (true) {
            alloy::sleep_for(3s);
        }
    }
}
