// Filesystem demo: a real littlefs on a reserved flash region. Where the nvm
// demo keeps one integer in a flat key/value slot, this keeps NAMED FILES in
// DIRECTORIES that survive power-off — a boot counter in /boot.txt, a growing
// /logs/boot.log, then a directory listing with sizes. Zero #ifdefs: board::fs
// exists on every board (a no-op stub where absent), and the demo is guarded by
// `if constexpr (board::caps::fs)`.
#include <alloy/board.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

using namespace alloy::literals;

namespace {
template <class Uart>
void print_u32(const Uart& uart, std::uint32_t v) {
    char buf[10];
    int i = 10;
    do {
        buf[--i] = static_cast<char>('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    while (i < 10) {
        uart.write(static_cast<std::uint8_t>(buf[i++]));
    }
}

// Render `v` as decimal ASCII into `buf`; returns the byte count written.
std::size_t format_u32(std::uint32_t v, std::span<std::uint8_t> buf) {
    std::uint8_t tmp[10];
    int i = 10;
    do {
        tmp[--i] = static_cast<std::uint8_t>('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    std::size_t n = 0;
    while (i < 10 && n < buf.size()) {
        buf[n++] = tmp[i++];
    }
    return n;
}

std::uint32_t parse_u32(std::span<const std::uint8_t> s) {
    std::uint32_t v = 0;
    for (std::uint8_t c : s) {
        if (c < '0' || c > '9') break;
        v = v * 10u + static_cast<std::uint32_t>(c - '0');
    }
    return v;
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy fs (littlefs) demo\r\n");

    if constexpr (board::caps::fs) {
        if (!board::fs.mount_or_format()) {
            uart.write("mount/format FAILED\r\n");
        } else {
            // 1) Boot counter kept in a NAMED FILE (not a raw KV slot).
            std::uint8_t rd[16];
            const int got = board::fs.read_file("/boot.txt", rd);
            const std::uint32_t prev =
                got > 0 ? parse_u32(std::span<const std::uint8_t>(rd, static_cast<std::size_t>(got)))
                        : 0u;
            const std::uint32_t boots = prev + 1u;

            std::uint8_t wr[16];
            const std::size_t len = format_u32(boots, wr);
            const bool saved =
                board::fs.write_file("/boot.txt", std::span<const std::uint8_t>(wr, len));

            // 2) A growing log under a subdirectory: "boot N\n" appended each run.
            static_cast<void>(board::fs.mkdir("/logs"));
            std::uint8_t line[16];
            std::size_t li = 0;
            for (char c : {'b', 'o', 'o', 't', ' '}) {
                line[li++] = static_cast<std::uint8_t>(c);
            }
            li += format_u32(boots, std::span<std::uint8_t>(line + li, sizeof(line) - li));
            line[li++] = static_cast<std::uint8_t>('\n');
            static_cast<void>(
                board::fs.append_file("/logs/boot.log", std::span<const std::uint8_t>(line, li)));

            uart.write("boot #");
            print_u32(uart, boots);
            uart.write(saved ? " (saved to /boot.txt)\r\n" : " (SAVE FAILED)\r\n");

            // 3) Directory listing with sizes — walk root and /logs.
            uart.write("files:\r\n");
            auto list = [&](const char* dir) {
                static_cast<void>(board::fs.for_each(
                    dir, [&](const char* name, std::uint32_t size, bool is_dir) {
                        // Skip the "." and ".." pseudo-entries.
                        if (name[0] == '.' &&
                            (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
                            return;
                        }
                        uart.write("  ");
                        uart.write(dir);
                        if (dir[1] != '\0') uart.write("/");  // "/" root vs "/logs"
                        uart.write(name);
                        if (is_dir) {
                            uart.write("/\r\n");
                        } else {
                            uart.write(" (");
                            print_u32(uart, size);
                            uart.write(" B)\r\n");
                        }
                    }));
            };
            list("/");
            list("/logs");
            uart.write("reset -> the count keeps rising, the log keeps growing\r\n");
        }
    } else {
        uart.write("this board declares no fs role\r\n");
    }

    while (true) {
        alloy::sleep_for(1s);
    }
}
