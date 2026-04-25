// examples/ethernet_basic/main.cpp
//
// SAME70 Xplained Ultra — Ethernet basic: link up, DHCP, ping reply.
//
// What it does:
//   1. Initialise board, UART console, PMC clocks, MDIO pins.
//   2. Release KSZ8081 from hardware reset; run PHY init (soft reset + auto-neg).
//   3. Initialise GMAC DMA descriptors (Same70Gmac).
//   4. Feed the EthernetInterface to LwipAdapter; start DHCP.
//   5. Main loop: poll_link() + adapter.poll() + LED heartbeat.
//   6. Print IP address to UART when DHCP assigns one.
//   7. lwIP handles ICMP echo (ping) automatically via the netif.
//
// Expected UART output:
//   ethernet_basic: ready
//   ethernet_basic: PHY init ok — link negotiating …
//   ethernet_basic: link UP 100 Mbps full duplex
//   ethernet_basic: DHCP bound — IP 192.168.1.42

#include <array>
#include <cstdint>

#include BOARD_HEADER
#include BOARD_UART_HEADER

#include "boards/same70_xplained/board_ethernet.hpp"
#include "device/runtime.hpp"
#include "drivers/net/lwip/lwip_adapter.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace {

using namespace alloy::examples::uart_console;

// Print a dotted-decimal IPv4 address to the UART console.
template <typename Uart>
void print_ip(const Uart& uart, std::array<std::uint8_t, 4u> ip) {
    auto byte_str = [&](std::uint8_t b) {
        char buf[4]{};
        std::size_t i = 3u;
        buf[i] = '\0';
        if (b == 0) { buf[--i] = '0'; }
        while (b > 0) { buf[--i] = static_cast<char>('0' + (b % 10)); b /= 10; }
        write_text(uart, std::string_view{buf + i, 3u - i});
    };
    byte_str(ip[0]); write_text(uart, ".");
    byte_str(ip[1]); write_text(uart, ".");
    byte_str(ip[2]); write_text(uart, ".");
    byte_str(ip[3]);
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    (void)uart.configure();
    write_line(uart, "ethernet_basic: ready");

    // ---- PMC + pin mux ----
    board::enable_ethernet_clocks();
    board::mux_ethernet_pins();
    board::release_phy_reset();

    // ---- MDIO + PHY init ----
    auto mdio = board::make_mdio();
    mdio.enable_management();

    auto mac_bytes = board::mac_from_eui48();
    auto gmac = board::make_gmac(std::span<const std::uint8_t, 6u>{mac_bytes});
    auto phy  = board::make_phy(mdio);

    if (phy.init().is_err()) {
        write_line(uart, "ethernet_basic: FAIL — PHY init error");
        while (true) { board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(100u); }
    }
    write_line(uart, "ethernet_basic: PHY init ok — link negotiating ...");

    // ---- MAC init ----
    gmac.init();

    // ---- Network stack ----
    auto eth     = board::make_ethernet_interface(gmac, phy);
    alloy::net::LwipAdapter<board::BoardEth> adapter{eth};
    adapter.init();  // DHCP

    bool ip_printed    = false;
    bool link_reported = false;

    // ---- Main cooperative loop ----
    while (true) {
        eth.poll_link();

        if (!link_reported && eth.link_up()) {
            const auto ls = phy.link_status();
            write_text(uart, "ethernet_basic: link UP ");
            write_text(uart, ls.speed_mbps == 100u ? "100" : "10");
            write_line(uart, ls.full_duplex ? " Mbps full duplex" : " Mbps half duplex");
            link_reported = true;
        }

        adapter.poll();

        if (!ip_printed && adapter.dhcp_bound()) {
            write_text(uart, "ethernet_basic: DHCP bound — IP ");
            print_ip(uart, adapter.ip_address());
            write_text(uart, "\r\n");
            ip_printed = true;
        }

        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500u);
    }
}
