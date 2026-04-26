// ESP32 dual-core demo.
//
// Core 0 (PRO_CPU): blinks the built-in LED at 1 Hz and prints messages
//                   received from CrossCoreChannel over UART0.
//
// Core 1 (APP_CPU): sends an incrementing counter to core 0 via the channel
//                   every ~500 ms (busy-wait loop).
//
// The CrossCoreChannel<uint32_t, 64> carries values from core 1 → core 0
// without any shared locks — acquire/release atomics keep it SMP-safe.

#include <cstdint>

#include "boards/esp32_devkit/board.hpp"
#include "boards/esp32_devkit/board_uart_raw.hpp"
#include "runtime/cross_core_channel.hpp"

// Shared channel: core 1 pushes, core 0 pops.
// CrossCoreChannel uses acquire/release atomics — safe across Xtensa LX6 cores.
static alloy::tasks::CrossCoreChannel<std::uint32_t, 64> g_channel;

// Busy-wait ~1 ms at 80 MHz APB clock.
static void busy_ms(std::uint32_t ms) noexcept {
    // ~80 000 iterations ≈ 1 ms at 80 MHz with 1 cycle/iter (conservative)
    for (std::uint32_t i = 0; i < ms * 80'000u; ++i) {
        asm volatile("nop");
    }
}

// Core 1 entry: push an incrementing counter every ~500 ms.
static void core1_main() noexcept {
    std::uint32_t counter = 0;
    while (true) {
        (void)g_channel.try_push(counter++);
        busy_ms(500);
    }
}

int main() {
    board::init();
    board::uart_raw::writeln("[core0] A: init done");

    board::uart_raw::writeln("[core0] B: before start_app_cpu");
    board::start_app_cpu([] { core1_main(); });
    board::uart_raw::writeln("[core0] C: after start_app_cpu");

    // Core 0 loop: receive from channel + blink LED.
    bool led_state = false;
    std::uint32_t blink_timer = 0;
    std::uint32_t hb_timer = 0;

    while (true) {
        // Drain all pending channel messages.
        while (auto val = g_channel.try_pop()) {
            board::uart_raw::write("[core0] recv: ");
            board::uart_raw::write_uint32(*val);
            board::uart_raw::writeln("");
        }

        // Blink at ~1 Hz (1000 ms period, toggling every 500 ms).
        busy_ms(1);
        if (++blink_timer >= 500u) {
            blink_timer = 0;
            led_state ? board::led::off() : board::led::on();
            led_state = !led_state;
        }

        // Heartbeat every ~2 s so we can confirm core 0 is alive in the loop.
        if (++hb_timer >= 2000u) {
            hb_timer = 0;
            board::uart_raw::writeln("[core0] heartbeat");
        }
    }
}
