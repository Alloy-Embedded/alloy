// NVM demo: a boot counter that survives power-off. Read the stored count,
// increment it, write it back to a reserved flash page. Every reset the number
// keeps climbing — the proof the key/value store is really persistent. Zero
// #ifdefs: board::nvm exists on every board (a no-op stub where absent), and
// the demo path is guarded by `if constexpr (board::caps::nvm)`.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
// Any u32 key other than 0xFFFF'FFFF (which marks an erased/free slot).
constexpr std::uint32_t kBootCount = 1;

template <class Uart>
void print_u32(const Uart& uart, std::uint32_t v) {
    char buf[10];
    int i = 10;
    do {
        buf[--i] = static_cast<char>('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    while (i < 10) {
        uart.write(static_cast<std::uint8_t>(buf[i++]));
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy nvm demo\r\n");

    if constexpr (board::caps::nvm) {
        const std::uint32_t boots = board::nvm.get(kBootCount, 0u) + 1u;
        const bool ok = board::nvm.set(kBootCount, boots);
        uart.write("boot #");
        print_u32(uart, boots);
        uart.write(ok ? " (saved to flash)\r\n" : " (SAVE FAILED)\r\n");
        uart.write("reset or power-cycle -> the count keeps rising\r\n");
    } else {
        uart.write("this board declares no nvm role\r\n");
    }

    while (true) {
        alloy::sleep_for(1s);
    }
}
