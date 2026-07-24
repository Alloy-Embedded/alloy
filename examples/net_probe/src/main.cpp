// Connectivity capability probe (M0). Proves the capability mechanism:
// this SAME zero-#ifdef source compiles on EVERY board — boards with no
// network only ever reach the poisoned board::eth inside a discarded
// if-constexpr branch, so it is never instantiated. The NetDevice driver
// and real link bring-up arrive in M1.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy net_probe\r\n");
    while (true) {
        if constexpr (board::caps::net) {
            [&](auto& u) {
                if constexpr (board::caps::ethernet) {
                    u.write("net: ethernet present (PHY addr ");
                    u.write(static_cast<std::uint8_t>('0' + board::eth_phy_addr));
                    u.write(") — NetDevice bring-up in M1\r\n");
                } else {
                    u.write("net: wifi present — vendor-blob path (later)\r\n");
                }
            }(uart);
        } else {
            uart.write("net: no connectivity on this board\r\n");
        }
        alloy::sleep_for(3s);
    }
}
