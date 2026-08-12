// An ADC analog watchdog, armed and disarmed while the converter runs.
//
// The watchdog is a SUB-RESOURCE, not a knob on open(): it exists three times
// inside this ADC, each with its own enable, window and sticky flag, and this
// program arms one long after the port is open. `adc.watchdog<0>()` hands back
// its own handle; the ordinal is checked against the generated count
// `Inst::feat::analog_watchdogs`, so `watchdog<3>()` here would be a compile
// error with both numbers in it rather than three registers nobody owns.
//
// WHAT THE EMULATION LEG PROVES, and why it cannot pass by coincidence. The
// robot feeds two DISTINCT voltages to two DISTINCT channels (1650 mV on ch3
// and 3300 mV on ch4 at the platform's 3.3 V reference, so Renode's own
// Analog.STM32G0_ADC converts round(mV/3300*4095) = 2048 and 4095). One window
// — [1000, 3000] counts — is armed against each in turn, and the same firmware
// must print QUIET for the sample inside it and TRIPPED for the one outside.
// A model with a stuck flag fails one of those two lines whichever way it is
// stuck. Then the flag is cleared (quiet again, so the trip was sticky and not
// re-raised by nothing) and the watchdog disarmed (quiet across a conversion
// that WOULD have tripped it, so the enable is real). Four lines, two of which
// contradict each other unless the watchdog actually works.
//
// Renode's model declares watchdogCount: 1, so ordinals 1 and 2 — which this
// chip and this driver both have — are NOT exercised here. Said plainly rather
// than implied by omission.
//
// Zero preprocessor: boards without an ADC, and boards whose ADC has no
// watchdog alloy can reach, take the `if constexpr` branch that says so.
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

// Convert `channel` once, then report whether the watchdog is holding a trip.
// The conversion is what gives the watchdog something to compare, so the two
// belong in one step.
template <class Uart, class Adc, class Wd>
void convert_and_report(const Uart& uart, const Adc& adc, const Wd& wd,
                        std::uint8_t channel, const char* what) {
    // A real supervision loop samples on a period, and so does this one — the
    // watchdog is armed once and every conversion after it is a comparison.
    alloy::sleep_for(std::chrono::milliseconds{5});
    const std::uint16_t raw = adc.read(channel);
    uart.write(what);
    uart.write(" ch");
    write_u32(uart, channel);
    uart.write("=");
    write_u32(uart, raw);
    uart.write(wd.tripped() ? ": TRIPPED\r\n" : ": quiet\r\n");
}

// The whole sequence lives in a TEMPLATE on the ADC bind, and that is load
// bearing rather than tidy: `if constexpr` only leaves its discarded branch
// uninstantiated INSIDE a template. Written directly in main(), the branch that
// calls `watchdog<0>()` would still be compiled on every board whose ADC has
// none — which is most of them — and the zero-preprocessor claim would need an
// #ifdef to survive.
template <class Adc, class Uart>
void probe_watchdog(const Uart& uart) {
    if constexpr (Adc::watchdogs > 0u) {
        auto adc = Adc::open();

        // Counts, on the same scale read() returns. 1650 mV of a 3.3 V
        // reference converts to 2048, which is inside; 3300 mV converts to
        // 4095, which is not.
        constexpr std::uint16_t kLow = 1000;
        constexpr std::uint16_t kHigh = 3000;

        auto wd = adc.template watchdog<0>({.channel = 3, .low = kLow, .high = kHigh});
        convert_and_report(uart, adc, wd, 3, "in-window");

        // Re-point the SAME watchdog at a channel whose sample is outside the
        // window. rearm() is one call because the guarded channel and the
        // enable share a register the silicon only lets you write with the ADC
        // disabled — see alloy/hal/adc/st_adc_v2.hpp.
        wd.rearm({.channel = 4, .low = kLow, .high = kHigh});
        convert_and_report(uart, adc, wd, 4, "out-of-window");

        // Sticky until acknowledged, and acknowledged by nothing else.
        wd.clear();
        convert_and_report(uart, adc, wd, 3, "after-clear");

        // Disarmed: the very sample that tripped it above now says nothing.
        wd.disarm();
        wd.clear();
        convert_and_report(uart, adc, wd, 4, "after-disarm");
    } else {
        uart.write("adc watchdog: not available on this board\r\n");
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy adc_watchdog\r\n");

    probe_watchdog<board::adc>(uart);

    for (;;) { alloy::sleep_for(1s); }
}
