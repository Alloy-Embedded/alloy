#include "boards/nucleo_f401re/board.hpp"
#include "boards/nucleo_f401re/board_uart.hpp"

#include <cstddef>
#include <cstdint>

extern "C" {
volatile std::uint32_t alloy_renode_boot_stage = 0u;
volatile std::uint32_t alloy_renode_boot_marker = 0u;
volatile std::uint32_t alloy_renode_uart_bytes_written = 0u;
}

namespace {

constexpr std::uint32_t kBootMarkerMainReached = 0xA1144001u;
constexpr std::uint32_t kBootMarkerUartConfigFailed = 0xA1144002u;
constexpr auto kBootBanner = "alloy renode stm32f4 boot ok\n";

[[noreturn]] void idle_forever() {
    while (true) {
        __asm volatile("wfi");
    }
}

}  // namespace

int main() {
    alloy_renode_boot_stage = 1u;

    board::init();
    alloy_renode_boot_stage = 2u;

    auto uart = board::make_debug_uart();
    if (const auto configured = uart.configure(); configured.is_err()) {
        alloy_renode_boot_marker = kBootMarkerUartConfigFailed;
        idle_forever();
    }

    alloy_renode_boot_stage = 3u;
    for (const char* cursor = kBootBanner; *cursor != '\0'; ++cursor) {
        if (const auto write = uart.write_byte(static_cast<std::byte>(*cursor)); write.is_err()) {
            alloy_renode_boot_marker = kBootMarkerUartConfigFailed;
            idle_forever();
        }
        alloy_renode_uart_bytes_written = alloy_renode_uart_bytes_written + 1u;
    }

    alloy_renode_boot_stage = 4u;
    alloy_renode_boot_marker = kBootMarkerMainReached;
    idle_forever();
}
