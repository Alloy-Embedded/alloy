// UART update bootloader — the resident half of `alloy update`. Build with
// `alloy build --slot bl`; apps are built with `--slot a|b`, packed by
// `alloy image`, and streamed in over the debug UART (alloy/ota/uart_transport).
//
// Boot policy (v2): the ota/boot_state trial/confirm machine over the power-
// atomic two-page boot_store (the layout's store region). An update is armed as
// a one-shot TRIAL via ota::install (verify, then one atomic record); the app
// must boot_manager::confirm() after its own health check or the bootloader
// reverts to the confirmed slot within N boots. plan_boot() consumes an attempt
// BEFORE the jump, so a hanging trial can't loop forever (pair with a watchdog
// so hangs become resets). Anti-brick: the planned slot is still verify_slot()ed
// here — a failing trial is reject_pending()ed and the confirmed slot boots; if
// NOTHING verifies the bootloader stays in update mode, so a blank or fully
// corrupt board is always recoverable over the same wire.
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
using store_t = alloy::ota::boot_store<flash_hal>;
using manager_t = alloy::ota::boot_manager<store_t>;

constexpr alloy::ota::slot kSlots[2] = {
    {alloy::slots::slot_a_base, alloy::slots::slot_a_size},
    {alloy::slots::slot_b_base, alloy::slots::slot_b_size},
};

// The transport's sink: stream into the updater, and on FINISH run the
// verify-then-arm tie (ota::install) so a crash in between leaves pending==none
// and the confirmed firmware still boots.
struct arming_sink {
    alloy::ota::updater<flash_hal>& up;
    manager_t& mgr;
    std::uint8_t target;
    using rvoid = alloy::Result<void, alloy::ota::ota_error>;
    [[nodiscard]] rvoid begin() { return up.begin(); }
    [[nodiscard]] rvoid write(std::span<const std::uint8_t> c) { return up.write(c); }
    [[nodiscard]] rvoid finish() {
        if (auto h = alloy::ota::install(up, mgr, target); !h) {
            return h.error();
        }
        return {};
    }
};

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy bootloader\r\n");

    store_t store{flash_hal{}, alloy::slots::store_base,
                  alloy::slots::store_base + alloy::slots::page_size};
    manager_t mgr{store};

    // Decide once per reset (consumes a trial attempt), then verify what we were
    // told to boot; fall back per the anti-brick doctrine.
    const auto plan = mgr.plan_boot();
    int boot = -1;
    std::uint32_t running_version = 0;
    if (const auto h = alloy::ota::verify_slot(kSlots[plan.slot])) {
        boot = plan.slot;
        running_version = h->image_version;
    } else {
        if (plan.kind == alloy::ota::boot_kind::trial) {
            (void)mgr.reject_pending();  // never re-try a slot that doesn't verify
        }
        const std::uint8_t other = static_cast<std::uint8_t>(1u - plan.slot);
        if (const auto ho = alloy::ota::verify_slot(kSlots[other])) {
            boot = other;
            running_version = ho->image_version;
        }
    }

    const std::uint8_t target = mgr.update_target();
    alloy::ota::updater<flash_hal> up{flash_hal{}, kSlots[target], flash_hal::page_size};
    arming_sink sink{up, mgr, target};
    auto tx = [&uart](std::span<const std::uint8_t> frame) {
        for (auto b : frame) {
            uart.write(b);
        }
    };
    alloy::ota::uart_receiver rx{
        sink, tx, {.target_slot = target, .running_version = running_version}};

    // Update window: ~500 ms of silence after reset, extended by any traffic.
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
            break;
        }
    }

    if (boot >= 0) {
        if (plan.kind == alloy::ota::boot_kind::trial && boot == plan.slot) {
            uart.write(boot == 0 ? "trial boot slot A\r\n" : "trial boot slot B\r\n");
        } else if (plan.kind == alloy::ota::boot_kind::reverted) {
            uart.write(boot == 0 ? "reverted, boot slot A\r\n" : "reverted, boot slot B\r\n");
        } else {
            uart.write(boot == 0 ? "boot slot A\r\n" : "boot slot B\r\n");
        }
        alloy::arch::boot_image(kSlots[boot].base + alloy::slots::app_offset);
    }

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
