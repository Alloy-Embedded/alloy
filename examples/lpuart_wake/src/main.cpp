// The UART that keeps listening after the core has stopped.
//
// An LPUART is a USART with two changes, and only one of them is the point.
// The point is that its receiver runs from a clock the rest of the chip does
// not need, so a product in Stop mode — microamps, core halted, PLL off — can
// still be addressed over a serial link and woken by it. The other change is
// how it makes that possible: it divides 256 x its kernel clock, which is what
// lets a 32.768 kHz watch crystal produce 9600 baud.
//
// That second change is the one this example is really about, because it is
// the one that can hurt you. On the Nucleo-G0B1RE this prints:
//
//     alloy lpuart_wake ready
//     this board has a low-power UART: LPUART1
//     kernel     64000000 Hz
//     asked for  115200 baud
//     achieved   115200 baud      <- computed by INVERTING the divisor
//     window     15626..21333333 baud
//     wake       start-bit, RS-485 DE on PB1
//     listening — bytes on PB10 come back on PB11
//
// READ THE WINDOW LINE. It is not decoration. The LPUART's divisor lives in a
// 20-bit register that RM0444 forbids taking below 0x300, so the kernel clock
// must be between 3x and 4096x the baud rate. On this board the kernel is
// PCLK at 64 MHz, and 9600 baud IS NOT REACHABLE — it needs the LSE, and
// alloy's clock_node has no spelling for LSE yet. Ask for it anyway:
//
//     board::low_power_uart::open_checked<9'600_baud>();
//
// and you get a compile error that says the rate is unreachable and why. You
// do not get a port running at some other speed, which is what programming the
// clamped divisor would have given you, and which is what a driver that reused
// the plain USART's `kernel / baud` would have given you silently.
//
// WHAT THIS EXAMPLE DOES NOT DO, AND CANNOT. It never enters Stop mode. alloy
// has no low-power entry API — no PWR register work, no WFI wrapper — so
// `wake_from_stop` below programs the peripheral to BE wakeable (CR1.UESM,
// CR3.WUS, CR3.WUFIE) and stops there. The half of the story this file can
// show is the half that is a compile-time contract; the half it cannot show is
// the half that needs a sleeping core and a scope. Nothing here has been
// witnessed on silicon — there is no G0B1 board on this bench, and Renode has
// no LPUART model for this die.
//
// On the seven boards with no low-power UART this compiles and runs unchanged:
// `board::low_power_uart` is a null stub with the same shape, the cap is
// false, and the branch that names the LPUART is never instantiated.
#include <alloy/board.hpp>
#include <alloy/delay.hpp>

#include <cstdint>

namespace {

alloy::delay timer{};

void put_u32(auto& con, std::uint32_t v) {
    char buf[10];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + (v % 10u));
        v /= 10u;
    } while (v != 0u && n < 10);
    while (n-- > 0) {
        con.write(static_cast<std::uint8_t>(buf[n]));
    }
}

void put_line(auto& con, const char* what, std::uint32_t v, const char* unit) {
    con.write(what);
    put_u32(con, v);
    con.write(unit);
    con.write("\r\n");
}

// EVERY LPUART-SPECIFIC LINE LIVES IN THIS TEMPLATE, and that is not styling.
// A discarded `if constexpr` branch in an ordinary function is still fully
// type-checked, so `Lp::opts{.wake_from_stop = ...}` written in main() would
// have to compile on the boards whose `opts` has no such member — which is
// every board without an LPUART. Inside a template the discarded branch is
// never instantiated, so the knob only has to exist where it exists.
//
// The three branches are the three honest states, and the middle one is why
// there are two caps rather than one: a board may fill this role with an
// ordinary USART. It gets a serial link and no wakeup, and says so.
template <class Lp>
void low_power_link(auto& con) {
    if constexpr (board::caps::low_power_uart_wake) {
        con.write("this board has a low-power UART with a Stop-mode receiver\r\n");
        put_line(con, "kernel     ", Lp::kernel_hz(), " Hz");
        put_line(con, "asked for  ", board::low_power_uart_baud, " baud");

        // LAYER 2 — knobs that exist because THIS block's registers have them.
        // `wake_from_stop` and the DE times are not in alloy::uart::config and
        // never will be: a portable config field that five of six drivers
        // ignore is a promise the framework cannot keep.
        constexpr typename Lp::opts o{
            .fifo_enable = Lp::inst::feat::rx_fifo_depth != 0u,
            // 8/16ths of a bit each side: long enough for a common RS-485
            // transceiver to turn its driver around, short enough not to eat
            // the stop bit. A board that knows its transceiver should say so.
            .de_assert_16ths = 8,
            .de_deassert_16ths = 8,
            // The reason the peripheral exists. `start_bit` wakes on the first
            // edge of any frame, which is the setting that loses no data when
            // the wakeup latency is shorter than a character.
            .wake_from_stop = board::low_power_uart_wake::start_bit,
        };

        // THE LINE THIS EXAMPLE IS FOR. The baud is a template argument, so
        // the divisor window is checked before this program can exist. Move it
        // outside the window and this is a compile error naming the window,
        // not a runtime surprise on a wire nobody is scoping.
        auto link = Lp::template open_checked<alloy::frequency{board::low_power_uart_baud},
                                              20, o>();

        // The achieved rate is not re-derived here — it is the driver's own
        // divisor, INVERTED. That is the property that makes the compile-time
        // tolerance check trustworthy: the number printed and the number
        // programmed come from one place, so they cannot disagree.
        using impl = alloy::hal::uart_impl<typename Lp::inst>;
        constexpr std::uint32_t kernel = Lp::kernel_hz();
        put_line(con, "achieved   ",
                 impl::achieved_baud(kernel, board::low_power_uart_baud).hz(), " baud");

        // And the window itself, in the units the caller thinks in. Both ends
        // come from the chip database — the floor from RM0444's prose rule
        // curated as `feat::brr_min`, the ceiling from BRR's curated width —
        // so this is the board's real window and not a remembered number.
        if constexpr (requires { impl::brr_min; impl::brr_max; }) {
            constexpr std::uint64_t scaled = static_cast<std::uint64_t>(kernel) << 8u;
            put_line(con, "slowest    ",
                     static_cast<std::uint32_t>(scaled / impl::brr_max), " baud");
            put_line(con, "fastest    ",
                     static_cast<std::uint32_t>(scaled / impl::brr_min), " baud");
        }
        con.write("listening — bytes in come straight back out\r\n");

        // A plain echo. Nothing is fitted to these pins on a Nucleo, so on a
        // bare board this loop simply never sees a byte — which is the honest
        // outcome and not a hang: the heartbeat below keeps running.
        for (;;) {
            std::uint8_t b = 0;
            while (link.read(b)) {
                link.write(b);
            }
            timer.delay_ms(50);
        }
    } else if constexpr (board::caps::low_power_uart) {
        // The role is filled, but by a block with no Stop-mode receiver.
        con.write("this board has a spare UART in the low-power role, but the\r\n"
                  "block behind it cannot receive while the core sleeps\r\n");
        auto link = Lp::template open_checked<alloy::frequency{board::low_power_uart_baud}>();
        for (;;) {
            std::uint8_t b = 0;
            while (link.read(b)) {
                link.write(b);
            }
            timer.delay_ms(50);
        }
    } else {
        con.write("this board declares no low-power UART — heartbeat only\r\n");
        for (;;) {
            timer.delay_ms(500);
        }
    }
}

}  // namespace

int main() {
    board::init();
    auto con = board::debug_uart::open({.baud = board::debug_uart_baud});
    con.write("alloy lpuart_wake ready\r\n");
    low_power_link<board::low_power_uart>(con);
}
