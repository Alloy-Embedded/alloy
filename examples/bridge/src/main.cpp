// Three-phase complementary bridge — the timer's THIRD personality.
//
// The same kind of block that alloy::pwm drives as a waveform generator and
// alloy::encoder reads as a shaft position is bound here as an inverter
// bridge: three CHx/CHxN pairs, a hardware dead time between them, and a
// break input that switches all six outputs off without software in the path.
//
// ── READ THIS BEFORE YOU CONNECT ANYTHING ───────────────────────────────
//
// A bare Nucleo-G0B1RE has NO POWER STAGE. This example drives six morpho-
// header pins with a slow sine and prints what it programmed; nothing here
// has ever switched a transistor and no silicon has run this driver. There is
// no emulation leg either, and the reason is checked: alloy's generated
// platform does not instantiate TIM1 for Renode, so the dead time, the break
// input and the main output enable are unmodelled — running this under Renode
// logs the register writes and simulates none of their effects.
//
// If you are attaching this to a real bridge: put a scope on CH1 and CH1N
// first and measure the gap at both edges. `dead_time_ns()` below reports
// what the hardware says it programmed, and that number is the one to check
// against the trace.
//
// ── WHAT THE OUTPUT SHOWS ───────────────────────────────────────────────
//
// The requested dead time and the ACHIEVED one, which differ whenever the
// generator's encoding cannot represent the request exactly — it rounds UP,
// never down, because too much dead time distorts a waveform and too little
// destroys a bridge.
//
// Then a slow three-phase sine, 120 degrees apart, with the fault state
// printed on every step. `faulted()` reads the hardware's break flag; on a
// board with the BKIN pin floating it will be whatever the pin does.
//
// NO PREPROCESSOR. A board with no bridge role compiles the same source
// against a stub and takes the other branch of an `if constexpr`.
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

// A quarter-sine in Q15, 17 points, so the modulation below needs no math
// library and no floating point on a Cortex-M0+ that has neither.
constexpr std::uint16_t kQuarter[17] = {
        0,  3212,  6393,  9512, 12539, 15446, 18204, 20787,
    23170, 25330, 27245, 28898, 30273, 31357, 32138, 32610, 32767,
};

// sin(angle) for angle in 0..63, scaled to a 0..0xFFFF duty with 0x8000 at
// the zero crossing — which is what a three-phase modulator feeds a bridge:
// the neutral point is 50 % on every phase.
[[nodiscard]] std::uint16_t duty_at(unsigned angle) {
    const unsigned a = angle & 63u;
    const unsigned quadrant = a >> 4u;
    const unsigned step = a & 15u;
    const std::int32_t q = static_cast<std::int32_t>(
        quadrant == 0u || quadrant == 2u ? kQuarter[step] : kQuarter[16u - step]);
    const std::int32_t signed_sine = quadrant < 2u ? q : -q;
    // 0x8000 + sine * 0.9 — 90 % modulation depth leaves the duty away from
    // the rails, where a bridge's minimum pulse width lives.
    return static_cast<std::uint16_t>(32768 + (signed_sine * 9) / 10);
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy bridge\r\n");

    if constexpr (board::caps::bridge) {
        // `bridge_defaults` is what the board file says the power stage
        // needs — switching rate and gate-driver dead time, both project
        // facts, overridable from alloy.toml. There is no default dead time
        // anywhere in this chain: leaving it out is a compile error.
        //
        // open_checked<>, not open(): the config goes in the TEMPLATE
        // parameter, which is what turns the five admissions into
        // static_asserts that fire at every optimization level rather than
        // [[gnu::error]]s that need the optimizer to have folded the constant.
        // `bridge_defaults` is `inline constexpr`, so it is already a usable
        // template argument and nothing about the call site had to change but
        // the brackets.
        //
        // Layer 2 (`opts`) is left at its default: no break filter (the
        // conservative direction — an unfiltered break trips on noise, a
        // filtered one can ride through a real fault), center-aligned
        // variant 1, no configuration lock.
        auto inv = board::bridge::open_checked<board::bridge_defaults>();

        uart.write("phases: ");
        write_u32(uart, decltype(inv)::phases);
        uart.write("\r\nperiod: ");
        write_u32(uart, inv.period_ticks());
        uart.write(" ticks\r\ndead time: requested ");
        write_u32(uart, board::bridge_defaults.dead_time.requested_ns());
        uart.write(" ns, programmed ");
        write_u32(uart, inv.dead_time_ns());
        uart.write(" ns\r\n");

        // Duties first, outputs second. Every pin is still off here — open()
        // left the main output enable clear on purpose — so the bridge is
        // never live with a duty nobody chose.
        inv.set_duty<1>(0x8000);
        inv.set_duty<2>(0x8000);
        inv.set_duty<3>(0x8000);
        uart.write("outputs: ");
        uart.write(inv.outputs_enabled() ? "on\r\n" : "off (as opened)\r\n");

        inv.enable_outputs();
        uart.write("outputs enabled\r\n");

        unsigned angle = 0;
        for (;;) {
            inv.set_duty<1>(duty_at(angle));
            inv.set_duty<2>(duty_at(angle + 21u));   // +120 degrees of 64
            inv.set_duty<3>(duty_at(angle + 43u));   // +240
            angle = (angle + 1u) & 63u;

            if (inv.faulted()) {
                // The outputs are ALREADY off — the hardware did that before
                // this loop got a turn. Acknowledging the flag does not bring
                // them back, and re-arming is a decision, so this example
                // reports and keeps the bridge down.
                uart.write("BREAK: fault latched, outputs off in hardware\r\n");
                inv.acknowledge_fault();
            }

            if ((angle & 15u) == 0u) {
                uart.write("angle ");
                write_u32(uart, angle);
                uart.write(" duty1 ");
                write_u32(uart, duty_at(angle));
                uart.write("\r\n");
            }
            alloy::sleep_for(50ms);
        }
    } else {
        uart.write("bridge: no bridge role on this board\r\n");
        for (;;) { alloy::sleep_for(1s); }
    }
}
