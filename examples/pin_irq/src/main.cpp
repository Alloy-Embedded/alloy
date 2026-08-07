// A button that reports itself. No polling anywhere: the pin's own edge raises
// an interrupt, the driver's handler runs, and this loop only ever looks at a
// counter the handler bumped. `is_active()` is never called — if it were, the
// firmware could print the positive line with no interrupt at all, and the
// emulation leg would pass for the wrong reason.
//
// on_active() is the portable form: it asks for the edge that means "pressed"
// on THIS board, derived from the board's declared polarity. The Nucleo's B1
// USER is active-low, so this arms the FALLING edge — and the test leg drives
// the pin high first, which must NOT be counted.
//
// On a board whose pin driver has no interrupt hook (or that wires no button
// at all) the `requires` below is false, the method does not exist, and the
// firmware says so instead of failing to compile. No #if anywhere.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

// Set from the pin INTERRUPT. Seeing it true is proof the NVIC line fired and
// the driver's handler ran.
volatile bool g_edge_seen = false;

// Bounded wait: ~10 s of firmware time. An unbounded wait would turn a dead
// interrupt into a hang, and a hang is not a test failure — it is a test that
// never finishes.
constexpr int kWaitBudget = 100;

template <class Uart>
void write_hex32(const Uart& uart, std::uint32_t value) {
    static constexpr char kDigits[] = "0123456789abcdef";
    for (int shift = 28; shift >= 0; shift -= 4) {
        uart.write(static_cast<std::uint8_t>(kDigits[(value >> shift) & 0xFu]));
    }
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy pin_irq\r\n");

    // The templated lambda keeps `B` dependent, so the `requires` below REMOVES
    // the leg on a board without pin interrupts rather than hard-erroring.
    [&uart]<class B>(const B& btn) {
        if constexpr (requires { btn.on_active(nullptr, nullptr); btn.edges(); }) {
            btn.on_active(+[](void* flag) {
                *static_cast<volatile bool*>(flag) = true;
            }, const_cast<bool*>(&g_edge_seen));
            uart.write("pin irq: armed\r\n");

            for (int i = 0; i < kWaitBudget && !g_edge_seen; ++i) {
                alloy::sleep_for(100ms);
            }

            if (g_edge_seen) {
                // The COUNT is the interesting number, not the flag: the test
                // drives the pin the wrong way first, so anything above one
                // would mean the driver armed both edges instead of the one
                // this board's polarity asked for. It also catches the classic
                // failure — a handler that does not clear its write-1-to-clear
                // pending bit re-enters forever, and the count runs away.
                uart.write("pin irq: fired count=");
                write_hex32(uart, btn.edges());
                uart.write("\r\n");
            } else {
                uart.write("pin irq: NOT fired\r\n");
            }

            // Keep running afterwards. A firmware stuck re-entering its own
            // handler never reaches this line, so its absence is the signature
            // of the re-entry trap rather than a silent pass.
            for (int i = 0; i < 3; ++i) {
                alloy::sleep_for(100ms);
            }
            uart.write("pin irq: done\r\n");
        } else {
            uart.write("pin irq: not available on this board\r\n");
        }
    }(board::user_button());

    for (;;) {
        alloy::sleep_for(1s);
    }
}
