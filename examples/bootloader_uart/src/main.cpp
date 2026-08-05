// UART update bootloader — the resident half of `alloy update`. Build with
// `alloy build --slot bl` (it owns the first pages of flash); apps are built
// with `--slot a|b`, packed by `alloy image`, and streamed in over the debug
// UART through alloy/ota/uart_transport.
//
// Boot policy (v1, stateless A/B): verify both slots and boot the NEWEST valid
// image (higher image_version); updates always land in the OTHER slot, so the
// running image is never touched and a corrupt/interrupted transfer just fails
// verify and the old image keeps booting — power-loss-safe rollback with no
// persistent boot state. (The trial/confirm machine in alloy/ota/boot_state can
// layer on top later.)
//
// After reset it listens ~500 ms for a host HELLO before jumping; any traffic
// extends the window. With no valid image at all it stays in update mode
// forever — a blank board is recoverable over the same wire.
#include <alloy/arch/cpu.hpp>
#include <alloy/board.hpp>
#include <alloy/ota.hpp>
#include <alloy/ota/uart_transport.hpp>
#include <alloy/slots.hpp>
#include <alloy/time.hpp>

#include <cstdint>
#include <span>

namespace {

using flash_hal = alloy::hal::flash_impl<alloy::dev::flash_t>;

struct picked {
    int boot = -1;        // -1: nothing valid
    std::uint8_t target = 0;
    std::uint32_t running_version = 0;
};

picked pick_slots() {
    const alloy::ota::slot a{alloy::slots::slot_a_base, alloy::slots::slot_a_size};
    const alloy::ota::slot b{alloy::slots::slot_b_base, alloy::slots::slot_b_size};
    const auto va = alloy::ota::verify_slot(a);
    const auto vb = alloy::ota::verify_slot(b);
    picked p;
    if (va && vb) {
        p.boot = vb->image_version > va->image_version ? 1 : 0;
    } else if (va) {
        p.boot = 0;
    } else if (vb) {
        p.boot = 1;
    }
    if (p.boot >= 0) {
        p.running_version = (p.boot == 1 ? vb : va)->image_version;
        p.target = p.boot == 0 ? 1 : 0;
    }
    return p;
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy bootloader\r\n");

    const picked p = pick_slots();
    const alloy::ota::slot target{
        p.target == 0 ? alloy::slots::slot_a_base : alloy::slots::slot_b_base,
        p.target == 0 ? alloy::slots::slot_a_size : alloy::slots::slot_b_size};

    alloy::ota::updater upd{flash_hal{}, target, flash_hal::page_size};
    auto tx = [&uart](std::span<const std::uint8_t> frame) {
        for (auto b : frame) {
            uart.write(b);
        }
    };
    alloy::ota::uart_receiver rx{
        upd, tx, {.target_slot = p.target, .running_version = p.running_version}};

    // Update window: ~500 ms of silence after reset, extended by any traffic
    // (an in-flight transfer keeps the window open by itself).
    std::uint32_t deadline = alloy::uptime_ms() + 500u;
    for (;;) {
        std::uint8_t byte;
        if (uart.read(byte)) {
            rx.on_byte(byte);
            deadline = alloy::uptime_ms() + 3000u;
        }
        if (rx.finished()) {
            uart.write("update ok, rebooting\r\n");
            alloy::arch::system_reset();
        }
        if (static_cast<std::int32_t>(alloy::uptime_ms() - deadline) >= 0) {
            break;  // window closed with no (completed) session
        }
    }

    if (p.boot >= 0) {
        uart.write(p.boot == 0 ? "boot slot A\r\n" : "boot slot B\r\n");
        const std::uintptr_t base =
            p.boot == 0 ? alloy::slots::slot_a_base : alloy::slots::slot_b_base;
        alloy::arch::boot_image(base + alloy::slots::app_offset);
    }

    // Nothing bootable: stay in update mode — a blank/bricked board is always
    // recoverable over the same UART.
    uart.write("no valid image, waiting for update\r\n");
    for (;;) {
        std::uint8_t byte;
        if (uart.read(byte)) {
            rx.on_byte(byte);
        }
        if (rx.finished()) {
            alloy::arch::system_reset();
        }
    }
}
