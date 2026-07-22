// DMA showcase, zero #ifdefs:
//   1. ADC burst — 32 continuous VREFINT conversions land in RAM by DMA,
//      the CPU only waits; prints first/min/max.
//   2. PWM waveform — a triangle table streams circularly from memory to
//      the LED channel's compare register on every timer update: the LED
//      breathes with ZERO CPU involvement, proven by the foreground loop
//      printing heartbeats meanwhile.
//
// All capability checks live INSIDE generic lambdas: if constexpr only
// skips instantiation in dependent contexts, so boards without a DMA
// controller compile the honest fallback path instead.
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

template <class Uart>
void write_u32(const Uart& uart, std::uint32_t value) {
    char digits[10];
    unsigned n = 0;
    do {
        digits[n++] = static_cast<char>('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (n != 0u) {
        uart.write(static_cast<std::uint8_t>(digits[--n]));
    }
}

template <class Dma>
concept HasDma = requires { alloy::hal::dma_impl<Dma>::enable_controller(); };

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy dma_probe\r\n");

    [&uart]<class Dma, class Adc = board::adc>(Dma*) {
        if constexpr (HasDma<Dma> && board::caps::adc && board::adc_has_vref) {
            auto adc = Adc::open();
            std::uint16_t samples[32] = {};
            if constexpr (requires(alloy::dma::channel<Dma, 1>& c) {
                              adc.read_burst(c, board::adc_vref_channel,
                                             std::span<std::uint16_t>{samples});
                          }) {
                auto chan = alloy::dma::channel<Dma, 1>::claim();
                const bool ok = adc.read_burst(chan, board::adc_vref_channel,
                                               std::span<std::uint16_t>{samples});
                std::uint16_t lo = 0xFFFF, hi = 0;
                for (auto s : samples) {
                    lo = s < lo ? s : lo;
                    hi = s > hi ? s : hi;
                }
                uart.write(ok ? "adc burst DMA: OK  first="
                              : "adc burst DMA: FALHOU  first=");
                write_u32(uart, samples[0]);
                uart.write(" min=");
                write_u32(uart, lo);
                uart.write(" max=");
                write_u32(uart, hi);
                uart.write("\r\n");
            } else {
                uart.write("adc burst DMA: driver sem hooks DMA\r\n");
            }
        } else {
            uart.write("adc burst DMA: sem DMA/ADC nesta board\r\n");
        }
    }(static_cast<board::dma_t*>(nullptr));

    [&uart]<class Dma, class Pwm = board::led_pwm>(Dma*) {
        if constexpr (HasDma<Dma> && board::caps::led_pwm) {
            auto pwm = Pwm::open({.freq_hz = 1'000});
            // Triangle waveform in RAW timer ticks; static — the DMA reads
            // it forever after main() moves on. 1024 points at one point
            // per 1 kHz update = ~1 s breathing period.
            static std::uint16_t wave[1024];
            if constexpr (requires(alloy::dma::channel<Dma, 2>& c) {
                              pwm.stream_duty(c, std::span<const std::uint16_t>{wave});
                              pwm.period_ticks();
                          }) {
                auto chan = alloy::dma::channel<Dma, 2>::claim();
                const std::uint32_t top = pwm.period_ticks() - 1u;
                for (unsigned i = 0; i < 512; ++i) {
                    const std::uint32_t v = top * i / 511u;
                    wave[i] = static_cast<std::uint16_t>(v);
                    wave[1023 - i] = static_cast<std::uint16_t>(v);
                }
                pwm.stream_duty(chan, std::span<const std::uint16_t>{wave});
                uart.write("pwm waveform DMA: LED respirando sem CPU\r\n");
            } else {
                uart.write("pwm waveform DMA: driver sem hooks DMA\r\n");
            }
        } else {
            uart.write("pwm waveform DMA: sem DMA/PWM nesta board\r\n");
        }
    }(static_cast<board::dma_t*>(nullptr));

    std::uint32_t beat = 0;
    while (true) {
        uart.write("heartbeat ");
        write_u32(uart, beat++);
        uart.write("\r\n");
        alloy::sleep_for(2s);
    }
}
