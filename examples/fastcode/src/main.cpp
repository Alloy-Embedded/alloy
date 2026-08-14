// fastcode demo: mark a hot function ALLOY_FASTCODE and it runs from RAM instead
// of flash. The proof is its ADDRESS — a fastcode function links into the RAM
// region (0x2000_xxxx on this STM32G0) while a normal one stays in flash
// (0x0800_xxxx). Calling it and getting the right answer proves the startup
// copied it from flash to RAM correctly. Zero #ifdefs.
#include <alloy/board.hpp>
#include <alloy/fastcode.hpp>

#include <cstdint>

using namespace alloy::literals;

// Runs from fast RAM (no flash wait states) — the kind of thing a control loop
// or a hot ISR wants.
// The loop's STATE, in the same fast memory as its code. On this G0 there is
// no coupled RAM, so it lands in ordinary SRAM and costs nothing; on a part
// with DTCM or CCM it moves there without the source changing.
ALLOY_FASTDATA std::uint32_t g_calls = 1;
ALLOY_FASTBSS std::uint32_t g_last;

ALLOY_FASTCODE std::uint32_t hot_sum(std::uint32_t n) {
    std::uint32_t s = 0;
    for (std::uint32_t i = 1; i <= n; ++i) {
        s += i;
    }
    return s;
}

// Same work, left in flash for comparison.
[[gnu::noinline]] std::uint32_t cold_sum(std::uint32_t n) {
    std::uint32_t s = 0;
    for (std::uint32_t i = 1; i <= n; ++i) {
        s += i;
    }
    return s;
}

namespace {
template <class Uart>
void print_hex32(const Uart& uart, std::uint32_t v) {
    for (int i = 7; i >= 0; --i) {
        const std::uint32_t d = (v >> (4u * static_cast<unsigned>(i))) & 0xFu;
        uart.write(static_cast<std::uint8_t>(d < 10u ? '0' + d : 'a' + (d - 10u)));
    }
}
template <class Uart>
void print_u32(const Uart& uart, std::uint32_t v) {
    char buf[10];
    unsigned n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    while (n > 0) {
        uart.write(static_cast<std::uint8_t>(buf[--n]));
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy fastcode demo\r\n");

    uart.write("hot_sum  @ 0x");
    print_hex32(uart, reinterpret_cast<std::uint32_t>(&hot_sum));
    uart.write("  (expect 0x2000.... = RAM)\r\n");
    uart.write("cold_sum @ 0x");
    print_hex32(uart, reinterpret_cast<std::uint32_t>(&cold_sum));
    uart.write("  (expect 0x0800.... = flash)\r\n");

    // Calling it proves the startup copied the code correctly.
    g_calls += 1;
    g_last = hot_sum(100);
    uart.write("hot_sum(100) = ");
    print_u32(uart, hot_sum(100));
    uart.write("  (expect 5050)\r\n");

    for (;;) {
        alloy::sleep_for(1s);
    }
}
