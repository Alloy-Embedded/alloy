// M2 capstone: an HTTP server serving a web page from bare metal. Same vendored
// lwIP on the same NetDevice (the GMAC) as the echo examples, but instead of a raw
// echo it speaks HTTP/1.1 on port 80 — point a browser at http://192.168.15.231/
// and you get an actual page. User code sees only alloy::net (stack + http_server);
// a handler maps (method, path) to a response whose body is a static page. Static
// IP for v1 (DHCP sibling: examples/dhcp_echo). The whole request/respond path is
// host-tested over lwIP loopback (tests/test_net.cpp::net_http_serves_a_page); this
// proves it on silicon.
#include <alloy/board.hpp>
#include <alloy/net/http.hpp>
#include <alloy/net/lwip.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
constexpr std::uint8_t kMac[6] = {0x02, 0xAE, 0x70, 0x00, 0x00, 0x01};

// The page, in static storage — its bytes outlive every send (the handler hands
// the server a view of it).
constexpr char kPage[] =
    "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>alloy</title><style>"
    "body{font-family:system-ui,sans-serif;margin:0;display:grid;place-items:center;"
    "min-height:100vh;background:#0b0f14;color:#e6edf3}"
    ".card{max-width:34rem;padding:2.5rem;text-align:center}"
    "h1{font-size:3rem;margin:0 0 .5rem;letter-spacing:-.03em}"
    "p{line-height:1.6;color:#9fb0c0}code{color:#7ee787}"
    "</style></head><body><div class=\"card\"><h1>alloy</h1>"
    "<p>Served from bare metal &mdash; vendored lwIP on a SAME70 Cortex-M7, no RTOS, "
    "no heap. The same <code>main.cpp</code> recompiles for any board with an "
    "Ethernet MAC.</p></div></body></html>";

alloy::net::http_response route(const alloy::net::http_request& rq) {
    if (rq.method == "GET" && rq.path == "/") {
        return {200, "OK", "text/html", kPage};
    }
    return {404, "Not Found", "text/plain", "not found\n"};
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy http_server (lwIP)\r\n");

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
        uart.write("ready: http://192.168.15.231/\r\n");

        static alloy::net::http_server<> http;
        http.listen(80);
        while (true) {
            net.poll();       // drive lwIP (RX in, timers, TCP)
            http.poll(route);  // accept -> read request -> serve the page -> close
        }
    } else {
        uart.write("http_server: this board has no Ethernet\r\n");
        while (true) {
            alloy::sleep_for(3s);
        }
    }
}
