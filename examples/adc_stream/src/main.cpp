// ADC DMA-ring conformance for emulation — the design's anchor 2.1 shape
// (docs/design/dma-streams.md §2.1): conversions stream into a ring while the
// consumer takes hardware-stable halves; the CPU never touches the sampling
// path. The firmware asserts IN-BAND, in take()-order, the three facts the
// leg exists for: the FIRST stable half is the FIRST half (the half event
// preceded the full event), the SECOND half follows it (the full event
// delivered), and the first half comes around AGAIN (the ring wrapped — the
// pattern written twice). Each phase prints the half's min..max sample value,
// which the robot pins to the millivolts it fed — an unrouted or unfed
// channel converts 0 and prints v=0..0, so neither the routing nor the data
// can pass by coincidence.
//
// Consumption is BOUNDED (pending() polls with a spin budget, never a bare
// take()): a boundary that never arrives prints STUCK and fails the leg in
// seconds instead of hanging the runner — a hang is a spin, never a pass.
//
// missed() is printed as a diagnostic and deliberately NOT asserted: Renode's
// STM32G0DMA re-raises HTIF on every sample of the second half (model
// behaviour, read in the 1.16.1 source), so under emulation the counter
// measures the model, not the consumer. On silicon it is the §4 overrun
// evidence.
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

#include <cstdint>
#include <span>

using namespace alloy::literals;

namespace {
// 256 samples = 128 per half — the anchor's geometry. Static storage is the
// lifetime the stream needs, and the reference-taking API pushes toward it.
alloy::dma::ring_storage<std::uint16_t, 256> samples;

// The analog input this example converts. 3 to match adc_read's fixture: the
// emulation robot feeds a known voltage to channel 3, so the printed counts
// are the fed millivolts and a wrong channel prints 0.
constexpr std::uint8_t kChannel = 3;

template <class Uart>
void write_u32(const Uart& uart, std::uint32_t value) {
    char buf[10];
    unsigned n = 0;
    do { buf[n++] = static_cast<char>('0' + value % 10u); value /= 10u; } while (value != 0u);
    while (n != 0u) { uart.write(static_cast<std::uint8_t>(buf[--n])); }
}

// Bounded wait for a stable half. The budget is generous against the
// emulated sample pacing (a half period is ~1.6M spin iterations at the
// platform's 1 kHz fixture rate) and small against the leg timeout.
template <class Stream>
bool wait_boundary(Stream& s) {
    for (std::uint32_t spin = 0; spin < 40'000'000u; spin = spin + 1u) {
        if (s.pending()) {
            return true;
        }
    }
    return false;
}

// Print one taken half as "<label>: half <A|B> v=<min>..<max>".
template <class Uart>
void report_half(const Uart& uart, const char* label,
                 std::span<const std::uint16_t> half, bool is_first_half) {
    std::uint16_t lo = 0xFFFFu;
    std::uint16_t hi = 0u;
    for (std::uint16_t v : half) {
        lo = v < lo ? v : lo;
        hi = v > hi ? v : hi;
    }
    uart.write(label);
    uart.write(": half ");
    uart.write(is_first_half ? "A" : "B");
    uart.write(" v=");
    write_u32(uart, lo);
    uart.write("..");
    write_u32(uart, hi);
    uart.write("\r\n");
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy adc_stream\r\n");

    // Templated lambda (the dma_uart pattern) so the probes are DEPENDENT:
    // requires-gates then fold to "not available" on boards whose binder
    // carries no adc.conv route, whose ADC driver has no DMA hooks, or whose
    // routed controller cannot do circular + half events — instead of
    // hard-erroring in a concrete context.
    [&uart]<class AdcBind>(AdcBind*) {
        if constexpr (requires(decltype(AdcBind::open()) h) {
                          h.ring(samples, kChannel);
                      }) {
            auto adc = AdcBind::open();
            auto ring = adc.ring(samples, kChannel);
            const std::uint16_t* first_half = &samples.data[0];

            // Phase 1: the first stable half must be half A — the half event
            // fired before the full event.
            if (!wait_boundary(ring)) {
                uart.write("adc stream: STUCK before first half\r\n");
            } else {
                std::span<const std::uint16_t> h = ring.take();
                report_half(uart, "first", h, h.data() == first_half);

                // Phase 2: cycle until the OTHER half arrives (the full
                // event); phase 3: until half A again (the wrap). Both
                // bounded: two laps of re-raised boundaries at most.
                const char* labels[2] = {"then", "wrap"};
                const std::uint16_t* want[2] = {&samples.data[128], first_half};
                for (unsigned phase = 0; phase < 2u; phase = phase + 1u) {
                    bool got = false;
                    for (unsigned n = 0; n < 600u && !got; n = n + 1u) {
                        if (!wait_boundary(ring)) {
                            break;
                        }
                        std::span<const std::uint16_t> s = ring.take();
                        if (s.data() == want[phase]) {
                            report_half(uart, labels[phase], s,
                                        s.data() == first_half);
                            got = true;
                        }
                    }
                    if (!got) {
                        uart.write("adc stream: STUCK cycling\r\n");
                        break;
                    }
                    if (phase == 1u) {
                        // Diagnostic only — see the header note.
                        uart.write("missed=");
                        write_u32(uart, ring.missed());
                        uart.write(" (diagnostic)\r\n");
                        uart.write("adc stream: done\r\n");
                    }
                }
            }
        } else {
            uart.write("adc ring: not available on this board\r\n");
        }
    }(static_cast<board::adc*>(nullptr));

    for (;;) {
        alloy::sleep_for(1s);
    }
}
