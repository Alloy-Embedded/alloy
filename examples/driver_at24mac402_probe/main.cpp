// examples/driver_at24mac402_probe/main.cpp
//
// SAME70 Xplained Ultra — AT24MAC402 driver probe.
//
// Target: the Microchip AT24MAC402 (U9) soldered on the SAME70 Xplained Ultra
// board, wired to TWIHS0 (PA3 = TWD0, PA4 = TWCK0) with A2/A1/A0 strapped to
// VCC. That strap puts the EEPROM block at I2C address 0x57 and the protected
// block (EUI-48 / EUI-64 / 128-bit serial) at 0x5F. If your carrier uses a
// different strap, override the defaults via the Config struct below.
//
// What the probe does, in order:
//   1. Initialises the board debug UART (115200-8-N-1 on PB4/PB0 per board).
//   2. Brings up TWIHS0 via `board::make_i2c()`.
//   3. Calls `Device::init()` to probe both logical addresses.
//   4. Reads the factory-programmed EUI-48 (6 bytes) and serial number (16 bytes)
//      and prints them as hex over UART.
//   5. Writes a 32-byte test pattern crossing a page boundary to EEPROM
//      offset 0x08, reads it back, compares. Repeats with an inverted pattern
//      to leave the EEPROM in a distinguishable state.
//   6. Prints PASS / FAIL and toggles the user LED slowly on pass, rapidly on
//      fail.
//
// Expected UART output on a working board:
//
//   at24mac402 probe: ready
//   at24mac402: init ok
//   at24mac402: EUI-48 = FC:C2:3D:XX:XX:XX
//   at24mac402: serial = 00 FC C2 3D XX XX XX XX XX XX XX XX XX XX XX XX
//   at24mac402: write + readback PASS
//   at24mac402: PROBE PASS
//
// On failure the line preceding FAIL tells you which step broke. The AT24MAC
// write cycle needs ~5 ms between page writes; the probe inserts a 10 ms
// SysTick delay after each write for margin.

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "driver_at24mac402_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
    #error "driver_at24mac402_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/memory/at24mac402/at24mac402.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"
#include "device/runtime.hpp"

namespace {

using namespace alloy::examples::uart_console;

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

[[noreturn]] void blink_ok() {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}

template <typename Uart>
void print_hex_run(const Uart& uart, std::span<const std::uint8_t> bytes, char sep) {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        constexpr auto kHex = "0123456789ABCDEF";
        const char buf[3] = {kHex[(bytes[i] >> 4) & 0x0F], kHex[bytes[i] & 0x0F], '\0'};
        write_text(uart, std::string_view{buf, 2});
        if (i + 1 != bytes.size() && sep != '\0') {
            char sbuf[2] = {sep, '\0'};
            write_text(uart, std::string_view{sbuf, 1});
        }
    }
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (uart.configure().is_err()) {
        blink_error(100);
    }
    write_line(uart, "at24mac402 probe: ready");

    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        write_line(uart, "at24mac402: FAIL (i2c configure)");
        blink_error(100);
    }

    // SAME70 Xplained Ultra strap: A2/A1/A0 all pulled high.
    alloy::drivers::memory::at24mac402::Device eeprom{bus,
                                                     {.eeprom_address = 0x57,
                                                      .protected_address = 0x5F}};

    if (eeprom.init().is_err()) {
        write_line(uart, "at24mac402: FAIL (init / device not ACKing)");
        blink_error(100);
    }
    write_line(uart, "at24mac402: init ok");

    std::array<std::uint8_t, 6> eui48{};
    if (eeprom.read_eui48(eui48).is_err()) {
        write_line(uart, "at24mac402: FAIL (read_eui48)");
        blink_error(100);
    }
    write_text(uart, "at24mac402: EUI-48 = ");
    print_hex_run(uart, eui48, ':');
    write_text(uart, "\r\n");

    std::array<std::uint8_t,
               alloy::drivers::memory::at24mac402::kSerialNumberLengthBytes>
        serial{};
    if (eeprom.read_serial_number(serial).is_err()) {
        write_line(uart, "at24mac402: FAIL (read_serial_number)");
        blink_error(100);
    }
    write_text(uart, "at24mac402: serial = ");
    print_hex_run(uart, serial, ' ');
    write_text(uart, "\r\n");

    // Write a 32-byte pattern starting at offset 0x08 so it crosses the
    // 16-byte page boundary (0x08..0x27 covers pages 0 and 1). The driver
    // must split the write into per-page transactions internally.
    std::array<std::uint8_t, 32> pattern{};
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        pattern[i] = static_cast<std::uint8_t>(0xA0 + i);
    }
    if (auto wr = eeprom.write(0x08, pattern); wr.is_err()) {
        write_text(uart, "at24mac402: FAIL (write) err=0x");
        write_hex_byte(uart, static_cast<std::uint8_t>(wr.error()));
        write_text(uart, "\r\n");
        blink_error(100);
    }
    // Give the part the datasheet write cycle (~5 ms) with margin.
    alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(10);

    std::array<std::uint8_t, 32> readback{};
    if (eeprom.read(0x08, readback).is_err()) {
        write_line(uart, "at24mac402: FAIL (readback)");
        blink_error(100);
    }

    bool match = true;
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (readback[i] != pattern[i]) {
            match = false;
            break;
        }
    }

    if (!match) {
        write_line(uart, "at24mac402: FAIL (pattern mismatch)");
        blink_error(100);
    }
    write_line(uart, "at24mac402: write + readback PASS");
    write_line(uart, "at24mac402: PROBE PASS");
    blink_ok();
}
