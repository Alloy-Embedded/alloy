// The app side of trial/confirm firmware update. Booted as a TRIAL by the
// bootloader (examples/bootloader_uart), this app runs its health check and then
// promotes itself with boot_manager::confirm() — one atomic record in the same
// power-atomic boot_store the bootloader reads. An app that never confirms (a
// crash, a broken peripheral, this line missing) is rebooted into the previous
// firmware automatically after N trial boots. Confirm on real health — "reached
// main()" is NOT health; here the stand-in check is that the debug UART opened.
#include <alloy/board.hpp>
#include <alloy/ota.hpp>
#include <alloy/slots.hpp>

#include <cstdint>

using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy ota_app ready\r\n");

    // Health check passed (the UART we just printed over) -> confirm the trial.
    // Idempotent: on a normal (already-confirmed) boot this writes nothing.
    using flash_hal = alloy::hal::flash_impl<alloy::dev::flash_t>;
    alloy::ota::boot_store<flash_hal> store{
        flash_hal{}, alloy::slots::store_base,
        alloy::slots::store_base + alloy::slots::page_size};
    alloy::ota::boot_manager mgr{store};
    if (mgr.confirm()) {
        uart.write("ota_app confirmed\r\n");
    } else {
        uart.write("ota_app confirm FAILED\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
