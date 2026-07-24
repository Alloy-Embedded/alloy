// DSP demo: run a synthetic "clean 50 Hz sine + high-frequency noise" signal
// through a 200 Hz low-pass biquad and print raw vs filtered side by side. The
// raw column swings wildly (the injected noise); the filtered column tracks the
// smooth sine — visible proof the filter runs (with libm float) on the MCU.
// Zero peripherals beyond the debug UART.
#include <alloy/board.hpp>
#include <alloy/dsp/biquad.hpp>

#include <cmath>
#include <cstdint>

using namespace alloy::literals;

namespace {
template <class Uart>
void print_signed(const Uart& uart, int v) {
    if (v < 0) {
        uart.write(static_cast<std::uint8_t>('-'));
        v = -v;
    }
    char buf[10];
    unsigned n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0) {
        uart.write(static_cast<std::uint8_t>(buf[--n]));
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy dsp filter demo: 50 Hz sine + noise -> 80 Hz low-pass\r\n");
    uart.write("peak-to-peak per window, x1000 (filt should be much smaller)\r\n");

    constexpr float fs = 8000.0f;
    constexpr std::uint32_t window = 8000u;  // one "second" of samples per report
    alloy::dsp::biquad lp{alloy::dsp::lowpass(80.0f, fs)};
    std::uint32_t seed = 0x1234567u;

    for (;;) {
        float raw_lo = 1e9f;
        float raw_hi = -1e9f;
        float filt_lo = 1e9f;
        float filt_hi = -1e9f;
        for (std::uint32_t n = 0; n < window; ++n) {
            seed = seed * 1103515245u + 12345u;  // LCG pseudo-noise
            const float noise = static_cast<float>((seed >> 16) & 0x1FFu) / 512.0f - 0.5f;
            const float clean = std::sin(2.0f * 3.14159265f * 50.0f * static_cast<float>(n) / fs);
            const float raw = clean + noise;
            const float filt = lp.process(raw);
            raw_lo = raw < raw_lo ? raw : raw_lo;
            raw_hi = raw > raw_hi ? raw : raw_hi;
            filt_lo = filt < filt_lo ? filt : filt_lo;
            filt_hi = filt > filt_hi ? filt : filt_hi;
        }
        uart.write("raw_pp=");
        print_signed(uart, static_cast<int>((raw_hi - raw_lo) * 1000.0f));
        uart.write(" filt_pp=");
        print_signed(uart, static_cast<int>((filt_hi - filt_lo) * 1000.0f));
        uart.write("\r\n");
        alloy::sleep_for(700ms);
    }
}
