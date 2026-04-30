// analog_probe_complete — exercises every extended ADC HAL lever added in
// openspec extend-adc-coverage (tasks 2.1–2.8).
//
// Boards validated:
//   - nucleo_g071rb  (STM32G071RB)  — primary; exercises sequence + trigger
//   - same70_xplained               — secondary; exercises per-channel enable
//   - nucleo_f401re  (STM32F401RE)  — tertiary; mirrors G071RB path
//
// What this example demonstrates:
//   2.1  set_resolution(Bits12) + set_alignment(Right)
//   2.2  set_continuous(true) + stop()
//   2.3  set_sample_time — per-channel ticks (slow on internal, fast on others)
//   2.4  set_sequence — 4-channel ordered sequence via typed Channel enum
//   2.5  enable_channel / disable_channel / channel_enabled (SAME70 AFEC path)
//   2.6  set_hardware_trigger — TIM3/TC0 TRGO at 1 kHz (source index 3/1)
//   2.7  read_sequence — polling drain of N conversions
//   2.8  end_of_sequence + overrun + clear_overrun
//   1.3  Adc::Channel typed enum — channel indices named at compile time
//
// Hardware needed: USB-to-ST-Link cable only.
// External signal on CH0/CH1/CH4 is optional; floating inputs read ~0 or
// rail-to-rail noise which is sufficient for a spot-check.
//
// DMA circular mode is deferred to when ADC DMA bindings are published in
// alloy-devices (see openspec out-of-scope task 8.1–8.3).

#include BOARD_HEADER
#include BOARD_ANALOG_HEADER
#include BOARD_UART_HEADER

#include <array>
#include <cstdint>
#include <span>

#include "examples/common/uart_console.hpp"
#include "hal/adc.hpp"
#include "hal/systick.hpp"

namespace {

namespace console = alloy::examples::uart_console;

using alloy::core::ErrorCode;
using alloy::core::Result;

// ── helpers ──────────────────────────────────────────────────────────────────

template <typename Uart>
void report(Uart& uart, const char* label,
            const Result<void, ErrorCode>& result) {
    console::write_text(uart, label);
    if (result.is_ok()) {
        console::write_line(uart, "  OK");
    } else {
        console::write_text(uart, "  NotSupported(");
        console::write_unsigned(uart, static_cast<std::uint32_t>(result.err()));
        console::write_line(uart, ")");
    }
}

template <typename Uart>
void report_bool(Uart& uart, const char* label, bool value) {
    console::write_text(uart, label);
    console::write_line(uart, value ? "  true" : "  false");
}

[[noreturn]] void blink_error() {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(150);
    }
}

// ── section 1: resolution + alignment ────────────────────────────────────────

template <typename Uart, typename Adc>
void probe_resolution(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.1 resolution / alignment ---");

    report(uart, "set_resolution(Bits12)",
           adc.set_resolution(alloy::hal::adc::Resolution::Bits12));
    report(uart, "set_alignment(Right)",
           adc.set_alignment(alloy::hal::adc::Alignment::Right));
}

// ── section 2: mode ───────────────────────────────────────────────────────────

template <typename Uart, typename Adc>
void probe_mode(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.2 continuous mode ---");

    report(uart, "set_continuous(true)",  adc.set_continuous(true));
    report(uart, "stop()",                adc.stop());
    report(uart, "set_continuous(false)", adc.set_continuous(false));
}

// ── section 3: sample time ───────────────────────────────────────────────────

template <typename Uart, typename Adc>
void probe_sample_time(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.3 sample time ---");

    // Fast sample time for regular channels (1.5 cycles — value 0).
    report(uart, "set_sample_time(CH0,  0)", adc.set_sample_time(
        static_cast<std::uint8_t>(Adc::Channel::CH0), 0u));
    report(uart, "set_sample_time(CH1,  0)", adc.set_sample_time(
        static_cast<std::uint8_t>(Adc::Channel::CH1), 0u));

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_F401RE)
    report(uart, "set_sample_time(CH4,  0)", adc.set_sample_time(
        static_cast<std::uint8_t>(Adc::Channel::CH4), 0u));
    // Slow for Vrefint: ≥4 µs as required by the datasheet (use 160 cycles).
    report(uart, "set_sample_time(Vrefint, 160)", adc.set_sample_time(
        static_cast<std::uint8_t>(Adc::Channel::Vrefint), 160u));
#elif defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    // SAME70 AFEC: TempSensor requires longer acquisition; CH2 is fast.
    report(uart, "set_sample_time(CH2, 0)", adc.set_sample_time(
        static_cast<std::uint8_t>(Adc::Channel::CH2), 0u));
    report(uart, "set_sample_time(TempSensor, 160)", adc.set_sample_time(
        static_cast<std::uint8_t>(Adc::Channel::TempSensor), 160u));
#endif
}

// ── section 4: sequence (G071RB / F401RE only — SAME70 uses per-channel enable)

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_F401RE)

template <typename Uart, typename Adc>
void probe_sequence(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.4 sequence builder ---");

    // Build 4-channel sequence using the typed Channel enum (task 1.3).
    constexpr std::array<std::uint8_t, 4> kSeq = {{
        static_cast<std::uint8_t>(Adc::Channel::CH0),
        static_cast<std::uint8_t>(Adc::Channel::CH1),
        static_cast<std::uint8_t>(Adc::Channel::CH4),
        static_cast<std::uint8_t>(Adc::Channel::Vrefint),
    }};
    report(uart, "set_sequence({CH0,CH1,CH4,Vrefint})",
           adc.set_sequence(std::span<const std::uint8_t>{kSeq}));
}

#endif

// ── section 5: per-channel enable (SAME70 AFEC only) ─────────────────────────

#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)

template <typename Uart, typename Adc>
void probe_channel_enable(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.5 per-channel enable (AFEC) ---");

    report(uart, "enable_channel(CH0)",  adc.enable_channel(
        static_cast<std::uint8_t>(Adc::Channel::CH0)));
    report(uart, "enable_channel(CH1)",  adc.enable_channel(
        static_cast<std::uint8_t>(Adc::Channel::CH1)));
    report(uart, "enable_channel(CH2)",  adc.enable_channel(
        static_cast<std::uint8_t>(Adc::Channel::CH2)));
    report(uart, "enable_channel(TempSensor)", adc.enable_channel(
        static_cast<std::uint8_t>(Adc::Channel::TempSensor)));

    report_bool(uart, "channel_enabled(CH0)", adc.channel_enabled(
        static_cast<std::uint8_t>(Adc::Channel::CH0)));

    report(uart, "disable_channel(CH2)", adc.disable_channel(
        static_cast<std::uint8_t>(Adc::Channel::CH2)));
    report_bool(uart, "channel_enabled(CH2) after disable",
                adc.channel_enabled(static_cast<std::uint8_t>(Adc::Channel::CH2)));

    // Re-enable for the read_sequence section below.
    static_cast<void>(adc.enable_channel(
        static_cast<std::uint8_t>(Adc::Channel::CH2)));
}

#endif

// ── section 6: hardware trigger ───────────────────────────────────────────────

template <typename Uart, typename Adc>
void probe_hardware_trigger(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.6 hardware trigger ---");

    // G071RB / F401RE: TIM3_TRGO = source index 3 (rising edge → start conversion).
    // SAME70 AFEC:     TC0 TIOA0 = source index 1.
    // After probing, disable the trigger so the read_sequence section below uses
    // software trigger (start()) instead of waiting for the timer.
#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_F401RE)
    report(uart, "set_hardware_trigger(TIM3_TRGO=3, Rising)",
           adc.set_hardware_trigger(3u, alloy::hal::adc::TriggerEdge::Rising));
#elif defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    report(uart, "set_hardware_trigger(TC0_TIOA0=1, Rising)",
           adc.set_hardware_trigger(1u, alloy::hal::adc::TriggerEdge::Rising));
#endif

    // Disable again so software trigger works in the remainder of the example.
    report(uart, "set_hardware_trigger(disabled)",
           adc.set_hardware_trigger(0u, alloy::hal::adc::TriggerEdge::Disabled));
}

// ── section 7: status flags ───────────────────────────────────────────────────

template <typename Uart, typename Adc>
void probe_status(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.8 status flags ---");
    report_bool(uart, "end_of_sequence()", adc.end_of_sequence());
    report_bool(uart, "overrun()",          adc.overrun());
    report(uart,      "clear_overrun()",    adc.clear_overrun());
}

// ── section 8: read_sequence (polling) ───────────────────────────────────────

template <typename Uart, typename Adc>
void probe_read_sequence(Uart& uart, Adc& adc) {
    console::write_line(uart, "--- 2.7 read_sequence (polling) ---");

    constexpr std::size_t kChannelCount = 4u;
    std::array<std::uint16_t, kChannelCount> samples{};

    // Enable continuous mode for a self-feeding conversion train.
    static_cast<void>(adc.set_continuous(true));
    static_cast<void>(adc.start());

    const auto result = adc.read_sequence(std::span<std::uint16_t>{samples});

    // Stop after draining one batch.
    static_cast<void>(adc.stop());
    static_cast<void>(adc.set_continuous(false));

    report(uart, "read_sequence(4 samples)", result);

    if (result.is_ok()) {
        for (std::size_t i = 0u; i < samples.size(); ++i) {
            console::write_text(uart, "  sample[");
            console::write_unsigned(uart, static_cast<std::uint32_t>(i));
            console::write_text(uart, "] = ");
            console::write_unsigned(uart, samples[i]);
            console::write_line(uart, "");
        }
    }
}

// ── capability introspection ──────────────────────────────────────────────────

template <typename Uart, typename Adc>
void print_capabilities(Uart& uart) {
    console::write_line(uart, "--- capability introspection ---");
    report_bool(uart, "has_resolution()",      Adc::has_resolution());
    report_bool(uart, "has_alignment()",        Adc::has_alignment());
    report_bool(uart, "has_continuous()",       Adc::has_continuous());
    report_bool(uart, "has_sample_time()",      Adc::has_sample_time());
    report_bool(uart, "has_sequence()",         Adc::has_sequence());
    report_bool(uart, "has_channel_enable()",   Adc::has_channel_enable());
    report_bool(uart, "has_hardware_trigger()", Adc::has_hardware_trigger());
    report_bool(uart, "has_end_of_sequence()",  Adc::has_end_of_sequence());
    report_bool(uart, "has_overrun()",          Adc::has_overrun());
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (uart.configure().is_err()) {
        blink_error();
    }

    console::write_line(uart, "=== analog_probe_complete ===");

    auto adc = board::make_adc({.enable_on_configure = true, .start_immediately = false});
    if (adc.configure().is_err()) {
        console::write_line(uart, "ADC configure failed");
        blink_error();
    }

    using Adc = board::BoardAdc;

    print_capabilities<decltype(uart), Adc>(uart);
    probe_resolution(uart, adc);
    probe_mode(uart, adc);
    probe_sample_time(uart, adc);

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_F401RE)
    probe_sequence(uart, adc);
#elif defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    probe_channel_enable(uart, adc);
#endif

    probe_hardware_trigger(uart, adc);
    probe_status(uart, adc);
    probe_read_sequence(uart, adc);

    console::write_line(uart, "=== done ===");

    // Re-arm sequence for the monitor loop.
#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_F401RE)
    {
        constexpr std::array<std::uint8_t, 4> kSeq = {{
            static_cast<std::uint8_t>(Adc::Channel::CH0),
            static_cast<std::uint8_t>(Adc::Channel::CH1),
            static_cast<std::uint8_t>(Adc::Channel::CH4),
            static_cast<std::uint8_t>(Adc::Channel::Vrefint),
        }};
        static_cast<void>(adc.set_sequence(std::span<const std::uint8_t>{kSeq}));
    }
#endif

    // Overrun monitor loop: single-shot conversions polled every 500 ms.
    // Demonstrates adc.overrun() + clear_overrun() as an active watchdog.
    // (Continuous + DMA circular is deferred to when ADC DMA bindings land.)
    console::write_line(uart, "--- overrun monitor loop ---");

    std::uint32_t overrun_count = 0u;
    std::array<std::uint16_t, 4u> buf{};

    while (true) {
        board::led::toggle();

        static_cast<void>(adc.start());
        const auto rseq = adc.read_sequence(std::span<std::uint16_t>{buf});

        if (adc.overrun()) {
            ++overrun_count;
            console::write_text(uart, "OVERRUN #");
            console::write_unsigned(uart, overrun_count);
            console::write_line(uart, "");
            static_cast<void>(adc.clear_overrun());
        } else if (rseq.is_ok()) {
            console::write_text(uart, "samples:");
            for (const auto s : buf) {
                console::write_text(uart, " ");
                console::write_unsigned(uart, s);
            }
            console::write_line(uart, "");
        }

        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
