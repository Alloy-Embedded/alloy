// A hardware timer as a periodic tick — the basic and small timers (TIM6,
// TIM7, TIM14, TIM15, TIM16, TIM17 on the STM32G0).
//
// The board file says which block is free and what rate this project wants;
// this file never names TIM6. It blinks the LED on the timer's update event
// and prints the rate the block ACTUALLY runs at, which is not always the one
// that was asked for — the division is integer.
//
// THE POINT IS THE POLLING LOOP, and it is worth reading twice: nothing here
// sleeps. `alloy::sleep_for` is the SysTick timebase, shared by the whole
// program; `tick.expired()` asks a peripheral this program owns whether its
// own period has elapsed. That is what makes a second, independent rate
// possible — and what makes the same code work when the tick is later pointed
// at an ADC instead of at the CPU.
//
// NO PREPROCESSOR. A board with no `tick` role compiles this same source
// against a stub and takes the other branch of an `if constexpr`. So does a
// board whose timer cannot drive a trigger output: `feat::trgo` is a generated
// number, and the branch below is chosen by it rather than by a chip name.
//
// WHAT THIS EXAMPLE CANNOT SHOW YOU, stated where you would otherwise assume
// it: there is no emulation leg for the G0's TIM6. Renode 1.16.1 has no model
// bound to this instance on this die, so the counting here is unwitnessed by
// anything but hardware, and no hardware was attached.
#include <alloy/board.hpp>

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
    uart.write("alloy tick\r\n");

    if constexpr (board::caps::tick) {
        // `tick_defaults` is the rate the project asked for — overridable from
        // alloy.toml, because the same board serves a housekeeping tick and a
        // control loop.
        auto tick = board::tick::open(board::tick_defaults);

        uart.write("requested ");
        write_u32(uart, board::tick_defaults.hz);
        uart.write(" Hz, running at ");
        write_u32(uart, tick.achieved_hz());
        uart.write(" Hz over ");
        write_u32(uart, tick.period_ticks());
        uart.write(" counts\r\n");

        // DEGREE, not a family name. `trgo` is 1 on TIM6/TIM7/TIM15 and 0 on
        // TIM14/TIM16/TIM17, and it comes from the curated IP data — so this
        // branch is a fact about the block rather than a chip name in an
        // #ifdef. On a real instance the guard is ALSO enforced by the type:
        // `handle::trigger_on_update()` carries `requires (feat::trgo != 0)`,
        // so calling it on a TIM16 is a compile error naming the instance.
        //
        // A GCC 14 FINDING, recorded because it cost a matrix run: `if
        // constexpr` in a NON-TEMPLATE function does not save you here. The
        // discarded branch is still checked, and the usual escape — hide the
        // call in a generic lambda so it becomes dependent — does not work
        // either, because GCC instantiates that lambda anyway. Which is why
        // the generated stub for a board with no tick role carries a no-op
        // `trigger_on_update()`: the branch below has to COMPILE on every
        // board even where it can never run.
        if constexpr (board::tick::inst::feat::trgo != 0u) {
            tick.trigger_on_update();
            uart.write("trgo: update event is on this timer's trigger output\r\n");
        } else {
            uart.write("trgo: this timer has no master-mode output\r\n");
        }

        std::uint32_t periods = 0;
        for (;;) {
            if (tick.expired()) {
                board::led.toggle();
                ++periods;
                if (periods % 10u == 0u) {
                    uart.write("periods ");
                    write_u32(uart, periods);
                    uart.write("\r\n");
                }
            }
        }
    } else {
        uart.write("tick: no tick role on this board\r\n");
        for (;;) { alloy::sleep_for(1s); }
    }
}
