// Quadrature encoder — the timer's OTHER personality.
//
// The same TIM3 that alloy::pwm would drive as a waveform generator is bound
// here as a position counter: channels 1 and 2 become the A and B inputs, the
// counter is clocked by the shaft rather than by the bus, and CNT is an angle
// index that wraps at the encoder's counts per revolution.
//
// Turn the knob and the board prints position, movement and direction. Nothing
// is fitted on a bare Nucleo, so with the pins floating the counter stays put
// and this prints a steady zero — which is the correct output for "no encoder
// attached", not a failure. Wire A to PA6 (Arduino D12) and B to PA7 (D11).
//
// NO PREPROCESSOR. A board with no encoder role compiles the same source
// against a stub and takes the other branch of an `if constexpr`.
//
// WHAT THIS EXAMPLE CANNOT SHOW YOU, stated where you would otherwise assume
// it: there is no emulation leg. Renode's Timers.STM32_Timer refuses the two
// register writes this personality is made of — "Unhandled write to offset
// 0x8 ... Tags: SMS" for the slave-mode select, and "Channel 1: input capture
// mode is not supported" for the input mapping — and it has no way to be fed
// quadrature edges. The counting is unwitnessed by anything but hardware.
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

template <class Uart>
void write_i32(const Uart& uart, std::int32_t value) {
    if (value < 0) {
        uart.write("-");
        write_u32(uart, static_cast<std::uint32_t>(-(value + 1)) + 1u);
        return;
    }
    write_u32(uart, static_cast<std::uint32_t>(value));
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy encoder\r\n");

    if constexpr (board::caps::encoder) {
        // `encoder_defaults` is what the board file says is plugged in — a
        // project fact, overridable from alloy.toml. Layer 2 (`opts`) is left
        // at its default here, which is encoder mode 3: both inputs' edges,
        // x4 on a quadrature source.
        auto knob = board::encoder::open(board::encoder_defaults);
        uart.write("period: ");
        write_u32(uart, knob.period());
        uart.write(" counts/rev\r\n");

        for (;;) {
            const std::int32_t moved = knob.delta();
            uart.write("pos ");
            write_u32(uart, knob.count());
            uart.write(" delta ");
            write_i32(uart, moved);
            uart.write(knob.direction() == alloy::encoder::direction::up
                           ? " dir up\r\n"
                           : " dir down\r\n");
            alloy::sleep_for(200ms);
        }
    } else {
        uart.write("encoder: no encoder role on this board\r\n");
        for (;;) { alloy::sleep_for(1s); }
    }
}
