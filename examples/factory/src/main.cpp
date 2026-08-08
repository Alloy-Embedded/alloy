// LINE-TEST firmware — the board that comes off the production line runs this
// once, before the product firmware is loaded, and either passes or does not.
//
// It answers exactly one question that nothing else on the line can answer:
// does the DEVICE agree with the WORK ORDER? `alloy provision write` verifies
// its own write through the probe, which proves the bytes are in flash; this
// proves the firmware can find them, parse them and act on them — that the
// address the linker baked in, the record format the host encoder wrote, and
// the parser that ships in the product are the same three things.
//
// It is deliberately a SLOT-A app (`alloy build --slot a`, packed with
// `alloy image`), because that is where the product firmware goes: the line
// test boots through the real bootloader, out of the real slot, so if the A/B
// layout or the bootloader is wrong the line finds out here rather than the
// customer finding out later.
//
// The last line is the verdict, and it is the only thing a line script should
// grep for:
//
//     LINE TEST: PASS
//     LINE TEST: FAIL <reason>
//
// A blank identity page prints FAIL unprovisioned — that is the negative
// control for the whole mechanism (tests/emulation/provision.robot runs it).
#include <alloy/board.hpp>
#include <alloy/provision.hpp>
#include <alloy/slots.hpp>
#include <alloy/time.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {

template <class Uart>
void put_hex_byte(const Uart& uart, std::uint8_t v) {
    constexpr char digits[] = "0123456789abcdef";
    uart.write(static_cast<std::uint8_t>(digits[v >> 4]));
    uart.write(static_cast<std::uint8_t>(digits[v & 0xF]));
}

template <class Uart>
void put_u32(const Uart& uart, std::uint32_t v) {
    char buf[10];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + (v % 10u));
        v /= 10u;
    } while (v != 0u);
    while (n-- > 0) {
        uart.write(static_cast<std::uint8_t>(buf[n]));
    }
}

// Why the device refused its own identity — the line operator needs to know
// whether to re-provision this board or to scrap the batch's programmer.
constexpr const char* reason(alloy::provision::prov_error e) {
    switch (e) {
    case alloy::provision::prov_error::blank:
        return "unprovisioned (identity page erased) — `alloy provision write` "
               "was never run on this board";
    case alloy::provision::prov_error::bad_magic:
        return "no identity record at provision_base — wrong address, or "
               "something overwrote the page";
    case alloy::provision::prov_error::bad_crc:
        return "identity record corrupt (torn write / partial erase) — "
               "re-provision this board";
    case alloy::provision::prov_error::unsupported_version:
        return "identity record is a newer format than this firmware parses";
    case alloy::provision::prov_error::truncated:
        return "identity record truncated";
    }
    return "unknown";
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy factory line-test\r\n");

    // Print the address the LINKER gave us, from the same generated header the
    // host verb reads the address out of. If these two ever disagree the whole
    // mechanism is broken, and this line is how it shows.
    uart.write("identity page: 0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        const auto nib = static_cast<std::uint8_t>(
            (static_cast<std::uint32_t>(alloy::slots::provision_base) >> shift) & 0xFu);
        uart.write(static_cast<std::uint8_t>(nib < 10 ? '0' + nib : 'a' + nib - 10));
    }
    uart.write("\r\n");

    const auto id = alloy::provision::read(alloy::slots::provision_base);
    if (!id) {
        uart.write("LINE TEST: FAIL ");
        uart.write(reason(id.error()));
        uart.write("\r\n");
        for (;;) {
            alloy::sleep_for(1s);
        }
    }

    // Byte by byte, not as a C string: a serial that fills all 16 bytes has no
    // NUL terminator, and serial_view() is the bounded view that says so.
    uart.write("serial: ");
    for (char c : id->serial_view()) {
        uart.write(static_cast<std::uint8_t>(c));
    }
    uart.write("\r\nmac: ");
    for (std::size_t i = 0; i < alloy::provision::mac_size; ++i) {
        if (i != 0) uart.write(static_cast<std::uint8_t>(':'));
        put_hex_byte(uart, id->mac[i]);
    }
    uart.write("\r\nhw_rev: ");
    put_u32(uart, id->hw_revision);
    uart.write("\r\nbatch: ");
    put_u32(uart, id->batch);
    uart.write("\r\n");

    // A serial that parses but is empty means the record was written by
    // something that is not `alloy provision` — the host verb refuses an empty
    // serial. Catch it here too: the line must never ship a nameless board.
    if (id->serial_view().empty()) {
        uart.write("LINE TEST: FAIL identity has an empty serial number\r\n");
    } else {
        uart.write("LINE TEST: PASS\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
