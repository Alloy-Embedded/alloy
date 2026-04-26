// examples/filesystem_littlefs/main.cpp
//
// Persistent boot counter stored in LittleFS on a Winbond W25Q128 NOR flash
// (16 MiB) over SPI0 on the SAME70 Xplained Ultra.
//
// Wiring (W25Q128 → EXT1 header):
//   W25Q128 /CS  → PD25  (EXT1 pin 15)   software GPIO CS
//   W25Q128 CLK  → PD22  (EXT1 pin 9)    SPI0_SPCK
//   W25Q128 DO   → PD20  (EXT1 pin 5)    SPI0_MISO
//   W25Q128 DI   → PD21  (EXT1 pin 7)    SPI0_MOSI
//   W25Q128 VCC  → 3V3   (EXT1 pin 20)
//   W25Q128 GND  → GND   (EXT1 pin 19)
//   W25Q128 /WP  → 3V3   (EXT1 pin 20)   tie high
//   W25Q128 /HOLD→ 3V3   (EXT1 pin 20)   tie high
//
// Behaviour:
//   First boot  → formats LittleFS, writes /counter.txt = "1\n"
//   Every reboot → increments the counter and writes it back.
//   Power-loss safe: littlefs guarantees consistency on every boot.

#include <array>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "filesystem_littlefs requires BOARD_UART_HEADER (debug UART) for the target board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "filesystem_littlefs requires BOARD_SPI_HEADER (SPI bus) for the target board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/filesystem/littlefs/littlefs_backend.hpp"
#include "drivers/memory/w25q/w25q_block_device.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/filesystem/filesystem.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"

namespace fs     = alloy::hal::filesystem;
namespace lfs_be = alloy::drivers::filesystem::littlefs;
namespace w25q   = alloy::drivers::memory::w25q;
namespace uart   = alloy::examples::uart_console;

// ── Hardware types ────────────────────────────────────────────────────────────

// W25Q128: 16 MiB = 4096 × 4 KiB sectors.
static constexpr std::size_t kFlashSectors = 4096;

using SpiHandle = board::BoardSpi;

// PD25 = EXT1 pin 15: software GPIO CS for W25Q128.
using CsPin    = alloy::hal::gpio::pin_handle<
    alloy::hal::gpio::pin<alloy::device::PinId::PD25>>;
using CsPolicy = w25q::GpioCsPolicy<CsPin>;
using Flash    = w25q::BlockDevice<SpiHandle, kFlashSectors, CsPolicy>;
using Backend  = lfs_be::LittlefsBackend<Flash, 256, 256, 256, 16>;
using Fso      = fs::Filesystem<Backend>;

// ── Helpers ───────────────────────────────────────────────────────────────────

static auto parse_u32(const char* s) -> std::uint32_t {
    std::uint32_t v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10u + static_cast<std::uint32_t>(*s++ - '0');
    }
    return v;
}

static auto fmt_u32(std::uint32_t v, char* buf, std::size_t sz) -> std::size_t {
    if (sz == 0) return 0;
    char tmp[12]{};
    std::size_t i = 0;
    if (v == 0) { tmp[i++] = '0'; }
    while (v > 0 && i < sizeof(tmp) - 2) {
        tmp[i++] = static_cast<char>('0' + v % 10);
        v /= 10;
    }
    for (std::size_t a = 0, b = i - 1; a < b; ++a, --b) {
        char t = tmp[a]; tmp[a] = tmp[b]; tmp[b] = t;
    }
    tmp[i++] = '\n';
    const std::size_t n = (i < sz - 1) ? i : sz - 1;
    for (std::size_t j = 0; j < n; ++j) buf[j] = tmp[j];
    buf[n] = '\0';
    return n;
}

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) {
        halt_blink(100);
    }
    uart::write_line(debug, "[fs] booting");

    // Configure PD25 as output-high (/CS deasserted) before SPI init.
    CsPin cs_pin{{.direction     = alloy::hal::PinDirection::Output,
                  .initial_state = alloy::hal::PinState::High}};
    if (cs_pin.configure().is_err()) {
        uart::write_line(debug, "[fs] CS pin configure failed");
        halt_blink(100);
    }
    CsPolicy cs{cs_pin};

    auto spi = board::make_spi(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode0,
        8'000'000u,
        alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8,
        board::kBoardSpiPeripheralClockHz,
    });
    if (spi.configure().is_err()) {
        uart::write_line(debug, "[fs] spi configure failed");
        halt_blink(100);
    }

    Flash flash{spi, cs};
    if (flash.init().is_err()) {
        uart::write_line(debug, "[fs] flash init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[fs] flash ok");

    Fso filesystem{Backend{flash}};

    if (filesystem.mount().is_err()) {
        uart::write_line(debug, "[fs] formatting...");
        if (filesystem.format().is_err()) {
            uart::write_line(debug, "[fs] format failed");
            halt_blink(100);
        }
        if (filesystem.mount().is_err()) {
            uart::write_line(debug, "[fs] mount after format failed");
            halt_blink(100);
        }
        uart::write_line(debug, "[fs] formatted and mounted");
    } else {
        uart::write_line(debug, "[fs] mounted");
    }

    // Read existing counter (0 on first boot).
    std::uint32_t counter = 0;
    {
        auto r = filesystem.open("/counter.txt", fs::OpenMode::ReadOnly);
        if (r.is_ok()) {
            auto file = std::move(r).unwrap();
            std::array<char, 16> buf{};
            auto span = std::span<std::byte>{
                reinterpret_cast<std::byte*>(buf.data()), buf.size() - 1};
            if (auto rd = file.read(span); rd.is_ok()) {
                counter = parse_u32(buf.data());
            }
        }
    }

    ++counter;

    // Print counter.
    {
        char num[12]{};
        fmt_u32(counter, num, sizeof(num));
        char msg[32] = "[fs] counter=";
        for (std::size_t i = 0; i < sizeof(num) && num[i] != '\0'; ++i) {
            msg[13 + i] = num[i];
        }
        uart::write_line(debug, msg);
    }

    // Write updated counter.
    {
        auto r = filesystem.open("/counter.txt", fs::OpenMode::Truncate);
        if (r.is_ok()) {
            auto file = std::move(r).unwrap();
            char wbuf[12]{};
            const std::size_t wlen = fmt_u32(counter, wbuf, sizeof(wbuf));
            auto span = std::span<const std::byte>{
                reinterpret_cast<const std::byte*>(wbuf), wlen};
            (void)file.write(span);
        } else {
            uart::write_line(debug, "[fs] open for write failed");
        }
    }

    uart::write_line(debug, "[fs] done");
    halt_blink(500);
}
