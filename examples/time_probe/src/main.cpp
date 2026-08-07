// Microsecond-timebase conformance probe (sibling of memory_probe/dma_probe).
// uptime_us() interpolates the SysTick counter between 1 ms ticks; the two
// properties a consumer (Modbus RTU silence detection, edge timing) relies on
// are asserted IN-BAND and reported over the UART, so the emulation leg fails
// on the printed verdict, not on a timing coincidence:
//
//   1. monotonic — 100k back-to-back samples never step backwards, including
//      across tick boundaries (the seqlock retry in uptime_us is what this
//      exercises; a torn read shows up as a ~1000 µs reverse step).
//   2. rate — a sleep_for(50ms) measures 50'000..60'000 µs. sleep_for
//      guarantees AT LEAST the duration plus up to two tick edges of slack,
//      so the window is asymmetric by design.
#include <alloy/board.hpp>
#include <alloy/time.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
template <class Uart>
void write_u32(const Uart& uart, std::uint32_t value) {
    char buf[10];
    unsigned n = 0;
    do { buf[n++] = static_cast<char>('0' + value % 10u); value /= 10u; } while (value != 0u);
    while (n != 0u) { uart.write(static_cast<std::uint8_t>(buf[--n])); }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy time_probe\r\n");

    // 1) Monotonicity across ~100 tick boundaries.
    bool mono = true;
    std::uint32_t prev = alloy::uptime_us();
    for (std::uint32_t i = 0; i < 100'000u; ++i) {
        const std::uint32_t now = alloy::uptime_us();
        if (static_cast<std::int32_t>(now - prev) < 0) {  // wrap-safe compare
            mono = false;
            break;
        }
        prev = now;
    }
    uart.write(mono ? "us monotonic: ok\r\n" : "us monotonic: FAIL\r\n");

    // 2) Advance rate against the ms timebase it interpolates.
    const std::uint32_t t0 = alloy::uptime_us();
    alloy::sleep_for(50ms);
    const std::uint32_t dt = alloy::uptime_us() - t0;
    if (dt >= 50'000u && dt <= 60'000u) {
        uart.write("us rate: ok\r\n");
    } else {
        uart.write("us rate: FAIL dt=");
        write_u32(uart, dt);
        uart.write("\r\n");
    }

    for (;;) { alloy::sleep_for(1s); }
}
