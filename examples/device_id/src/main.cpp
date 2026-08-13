// Who am I, and can this chip check its own flash?
//
// Two blocks that are three registers each and feed two features alloy already
// ships:
//
//   board::uid   the factory-programmed device identifier — the companion to
//                `alloy provision`, from the other direction. Provisioning
//                writes the serial on the label; this IS the die.
//   board::crc   the hardware CRC-32, which computes exactly the checksum the
//                OTA image format and the bootloader already use in software.
//
// The second claim is the one worth a demo rather than a paragraph, so this
// program does not assert it — it MEASURES it. The same bytes go through
// `board::crc` and through `alloy::ota::crc::crc32_of`, and both numbers are
// printed side by side with a verdict. On the Nucleo-G0B1RE the first comes out
// of silicon and the second out of the shift loop in ota/crc32.hpp; on a board
// with no CRC block `board::crc.open()` hands back that same software class and
// the two lines agree trivially. `board::caps::crc` says which happened.
//
// Nothing here has been run on silicon. If you have the board, this program is
// the witness: flash it, open the UART at 115200, and the AGREE/DISAGREE line
// is the answer to "is alloy's CRC configuration right".
//
// Zero preprocessor, and it builds on every board in the matrix: the chips with
// no CRC unit and no UID get the stand-ins, which are values and not errors.
#include <alloy/board.hpp>
#include <alloy/crc.hpp>
#include <alloy/ota/crc32.hpp>
#include <alloy/uid.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

using namespace alloy::literals;

namespace {

// The standard CRC check vector: the nine ASCII digits "123456789". Every
// catalogued CRC publishes its value over exactly this string, which is what
// makes it worth using here — CRC-32/ISO-HDLC's is 0xCBF43926, so a human with
// the datasheet can grade the output without trusting anything in this repo.
constexpr std::uint8_t kCheck[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

template <class Uart>
void put_hex32(const Uart& uart, std::uint32_t v) {
    for (int nib = 7; nib >= 0; --nib) {
        const auto d = static_cast<unsigned>((v >> (nib * 4)) & 0xFu);
        uart.write(static_cast<std::uint8_t>(d < 10u ? '0' + d : 'A' + (d - 10u)));
    }
}

template <class Uart>
void put_line(const Uart& uart, const char* label, std::uint32_t v) {
    uart.write(label);
    put_hex32(uart, v);
    uart.write("\r\n");
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy device_id: factory UID + hardware CRC-32\r\n");

    // ── the identity ────────────────────────────────────────────────────
    const auto id = board::uid.read();
    uart.write("uid       : ");
    // `hex_buffer` and not `char[hex_chars]`, and this is the line the
    // eight-board matrix exists to catch: on a chip with no UID block the
    // identifier is zero words, so the buffer is zero CHARS, and a C array of
    // zero is not a type. The array is — it just has nothing in it, which is
    // the right answer, and `to_hex` writes nothing into it. No branch is
    // needed to make this COMPILE anywhere; the branch below only decides what
    // a human reads when the line would otherwise be empty.
    decltype(id)::hex_buffer text;
    const std::size_t n = id.to_hex(text);
    for (std::size_t i = 0; i < n; ++i) {
        uart.write(static_cast<std::uint8_t>(text[i]));
    }
    if (!board::caps::uid) {
        uart.write("(this chip has no factory device ID)");
    }
    uart.write("\r\n");
    put_line(uart, "uid fold32: 0x", id.fold32());

    // ── the checksum, both ways ─────────────────────────────────────────
    uart.write(board::caps::crc ? "crc engine: hardware\r\n"
                                : "crc engine: software (no CRC block on this chip)\r\n");

    auto hw = board::crc.open();
    const std::span<const std::uint8_t> check{kCheck, sizeof kCheck};
    const std::uint32_t from_engine = hw.checksum(check);
    const std::uint32_t from_software = alloy::ota::crc::crc32_of(check);

    put_line(uart, "\"123456789\" via board::crc     : 0x", from_engine);
    put_line(uart, "\"123456789\" via ota::crc32_of  : 0x", from_software);
    uart.write(from_engine == from_software ? "verdict   : AGREE\r\n"
                                            : "verdict   : DISAGREE\r\n");

    // The same question over something long and unaligned, because a tail of
    // 1-3 bytes and a buffer that does not start on a word boundary are the two
    // ways a hardware CRC driver goes wrong while still looking plausible.
    static const std::uint8_t blob[131] = {};
    bool all_agree = true;
    for (std::size_t len = 0; len <= sizeof blob; len += 7) {
        for (std::size_t off = 0; off < 4u; ++off) {
            if (off + len > sizeof blob) {
                continue;
            }
            const std::span<const std::uint8_t> s{blob + off, len};
            if (hw.checksum(s) != alloy::ota::crc::crc32_of(s)) {
                all_agree = false;
            }
        }
    }
    uart.write(all_agree ? "lengths/offsets: all AGREE\r\n"
                         : "lengths/offsets: a DISAGREEMENT was found\r\n");

    while (true) {
        board::led.toggle();
        alloy::sleep_for(500ms);
    }
}
