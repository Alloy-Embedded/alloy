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
//
// v3 adds the two things a fielded fleet needs and a signature alone cannot
// give: an ANTI-ROLLBACK floor (a genuinely signed OLDER image is refused at
// accept time — see arming_sink) and a trusted KEY RING (a leaked signing key is
// retired by shipping a bootloader whose ring zeroes it). Both are policy on top
// of the same verify_slot gate; neither changes the boot path.
#include <alloy/arch/cpu.hpp>
#include <alloy/board.hpp>
#include <alloy/ota.hpp>
#include <alloy/ota/signed.hpp>
#include <alloy/ota/uart_transport.hpp>
#include <alloy/ota_key.hpp>
#include <alloy/slots.hpp>
#include <alloy/time.hpp>

#include <chrono>
#include <cstdint>
#include <span>

namespace {

using flash_hal = alloy::hal::flash_impl<alloy::slots::flash_ctrl_t>;
using store_t = alloy::ota::boot_store<flash_hal>;
using manager_t = alloy::ota::boot_manager<store_t>;

constexpr alloy::ota::slot kSlots[2] = {
    {alloy::slots::slot_a_base, alloy::slots::slot_a_size},
    {alloy::slots::slot_b_base, alloy::slots::slot_b_size},
};

// The project's root of trust, in ONE place: Ed25519 authenticity when
// alloy.toml declares an [ota] key, integrity-only otherwise. The key RING and
// its size come from generated data, so rotating a key is an alloy.toml edit and
// a rebuild — never a source change here. Same policy for the boot decision AND
// the install, so an unsigned image is refused ON THE WIRE (a NAK the operator
// sees) instead of silently wasting a trial boot.
[[nodiscard]] auto verify(const alloy::ota::slot& s) {
    if constexpr (alloy::ota_key::configured) {
        return alloy::ota::verify_slot(
            s, alloy::ota::signed_verifier{s, &alloy::ota_key::public_keys[0][0],
                                           alloy::ota_key::key_count});
    } else {
        return alloy::ota::verify_slot(s);
    }
}

// The transport's sink: stream into the updater, and on FINISH run the
// verify-then-arm tie (ota::install) so a crash in between leaves pending==none
// and the confirmed firmware still boots.
//
// ANTI-ROLLBACK. `floor` is the highest image_version this device can PROVE it
// already holds, and ota::install refuses anything below it. It starts at the
// running firmware's version and is raised, at HELLO, by whatever verifies in
// the slot about to be overwritten — so a version the device once accepted is
// still a floor after an automatic rollback has moved it back to the older slot.
// The floor lives in the SLOTS, so it survives power loss for the same reason
// the boot-state does, with no new persistent record to tear.
//
// Reading the target slot here (not at boot) is deliberate: the cost is one
// extra full verify, signature included, and it is paid only when an update is
// actually being attempted — never on the boot path.
//
// The honest limit: a device on which NEITHER slot verifies (blank, or bricked)
// has a floor of 0 and accepts anything. That is required — recovery must always
// be possible — so the claim is "a WORKING device refuses a downgrade", not "a
// device refuses a downgrade".
struct arming_sink {
    alloy::ota::updater<flash_hal>& up;
    manager_t& mgr;
    std::uint8_t target;
    std::uint32_t floor;
    using rvoid = alloy::Result<void, alloy::ota::ota_error>;
    [[nodiscard]] rvoid begin() {
        // BEFORE the erase: the outgoing image's version is a floor too.
        if (const auto h = verify(kSlots[target]); h && h->image_version > floor) {
            floor = h->image_version;
        }
        return up.begin();
    }
    [[nodiscard]] rvoid write(std::span<const std::uint8_t> c) { return up.write(c); }
    [[nodiscard]] rvoid finish() {
        const alloy::ota::slot& s = kSlots[target];
        if constexpr (alloy::ota_key::configured) {
            if (auto h = alloy::ota::install(
                    up, mgr, target,
                    alloy::ota::signed_verifier{s, &alloy::ota_key::public_keys[0][0],
                                                alloy::ota_key::key_count},
                    floor);
                !h) {
                return h.error();
            }
        } else if (auto h = alloy::ota::install(up, mgr, target, alloy::ota::no_auth{}, floor);
                   !h) {
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
                  alloy::slots::store_page_b};
    manager_t mgr{store};

    // Decide once per reset (consumes a trial attempt), then verify what we were
    // told to boot; fall back per the anti-brick doctrine.
    const auto plan = mgr.plan_boot();
    int boot = -1;
    std::uint32_t running_version = 0;
    if (const auto h = verify(kSlots[plan.slot])) {
        boot = plan.slot;
        running_version = h->image_version;
    } else {
        if (plan.kind == alloy::ota::boot_kind::trial) {
            (void)mgr.reject_pending();  // never re-try a slot that doesn't verify
        }
        const std::uint8_t other = static_cast<std::uint8_t>(1u - plan.slot);
        if (const auto ho = verify(kSlots[other])) {
            boot = other;
            running_version = ho->image_version;
        }
    }

    const std::uint8_t target = mgr.update_target();
    // Erase stride = the layout's page_size: the chip's erase page on uniform
    // flash, one big sector on sector flash (F7) — never the driver's business.
    alloy::ota::updater<flash_hal> up{flash_hal{}, kSlots[target], alloy::slots::page_size};
    arming_sink sink{up, mgr, target, running_version};
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
            // A trial that HANGS (no crash, no confirm) would otherwise wedge the
            // device forever — plan_boot() only consumes attempts across RESETS.
            // Arm the hardware watchdog before handing over: a healthy app
            // confirms and keeps feeding (see examples/ota_app); a hung or
            // never-feeding one is reset, consuming attempts until the automatic
            // rollback. Confirmed boots are NOT armed here — that's the
            // product's own policy, not the bootloader's.
            if constexpr (board::caps::watchdog) {
                board::watchdog.start(std::chrono::seconds{2});
            }
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
