// examples/filesystem_fatfs_sdcard/main.cpp
//
// FatFS on an SD card (SPI mode) on the SAME70 Xplained Ultra.
//
// Wiring (SD card → SPI0 EXT1 header):
//   SD /CS   → PD26  (EXT1 pin 14)   software GPIO CS
//   SD CLK   → PD22  (EXT1 pin 9)    SPI0_SPCK
//   SD MISO  → PD20  (EXT1 pin 5)    SPI0_MISO
//   SD MOSI  → PD21  (EXT1 pin 7)    SPI0_MOSI
//
// Note: PD25 (EXT1 pin 15) is reserved for the W25Q128 /CS in the
// filesystem_littlefs example — both can be wired on the same SPI0 bus.
//
// Behaviour:
//   1. Initialises the SD card (SPI mode, SDHC/SDXC only).
//   2. Mounts FAT32 volume (formats if mount fails and a blank card is detected).
//   3. Appends one timestamped line to /log.txt on each boot.
//   4. Reads and prints the last entry back over UART.
//
// UART output (115200, 8N1):
//   [sd] booting
//   [sd] card ok
//   [sd] mounted
//   [sd] wrote: boot=1 tick=0
//   [sd] last log: boot=1 tick=0
//   [sd] done

#include <array>
#include <cstdint>
#include <cstdlib>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "filesystem_fatfs_sdcard requires BOARD_UART_HEADER for the target board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "filesystem_fatfs_sdcard requires BOARD_SPI_HEADER for the target board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/filesystem/fatfs/fatfs_backend.hpp"
#include "drivers/memory/sdcard/sdcard.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/filesystem/filesystem.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"

namespace fs    = alloy::hal::filesystem;
namespace uart  = alloy::examples::uart_console;
namespace sd    = alloy::drivers::memory::sdcard;
namespace fatfs = alloy::drivers::filesystem::fatfs;

// ── Hardware types ────────────────────────────────────────────────────────────

using SpiHandle = board::BoardSpi;

// PD26 = EXT1 pin 14: software GPIO CS for SD card.
using CsGpioPin = alloy::hal::gpio::pin_handle<
    alloy::hal::gpio::pin<alloy::device::PinId::PD26>>;

using CsPolicy = sd::GpioCsPolicy<CsGpioPin>;
using Card     = sd::SdCard<SpiHandle, CsPolicy>;
using Backend  = fatfs::FatfsBackend<Card>;
using Fso      = fs::Filesystem<Backend>;

// ── Helpers ───────────────────────────────────────────────────────────────────

static auto fmt_u32(std::uint32_t v, char* buf, std::size_t sz) -> std::size_t {
    if (sz == 0) return 0;
    char tmp[12]{};
    std::size_t i = 0;
    if (v == 0) { tmp[i++] = '0'; }
    while (v > 0 && i < sizeof(tmp) - 1) {
        tmp[i++] = static_cast<char>('0' + v % 10);
        v /= 10;
    }
    for (std::size_t a = 0, b = i - 1; a < b; ++a, --b) {
        char t = tmp[a]; tmp[a] = tmp[b]; tmp[b] = t;
    }
    tmp[i] = '\0';
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

// Build a log line: "boot=N tick=T\n"
static auto build_log_line(std::uint32_t boot_count, std::uint32_t tick,
                            char* buf, std::size_t sz) -> std::size_t {
    // "boot=" + N + " tick=" + T + "\n"
    std::size_t pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos + 1 < sz) buf[pos++] = *s++;
    };
    char tmp[12]{};
    append("boot=");
    fmt_u32(boot_count, tmp, sizeof(tmp));
    append(tmp);
    append(" tick=");
    fmt_u32(tick, tmp, sizeof(tmp));
    append(tmp);
    if (pos + 1 < sz) buf[pos++] = '\n';
    buf[pos] = '\0';
    return pos;
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) {
        halt_blink(100);
    }
    uart::write_line(debug, "[sd] booting");

    // CS pin: output, starts deasserted (high).
    CsGpioPin cs_gpio{{.direction     = alloy::hal::PinDirection::Output,
                        .initial_state = alloy::hal::PinState::High}};
    if (cs_gpio.configure().is_err()) {
        uart::write_line(debug, "[sd] cs pin init failed");
        halt_blink(100);
    }

    // SPI at 400 kHz for card init; the card will accept higher speeds after init.
    auto spi = board::make_spi(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode0,
        400'000u,
        alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8,
        board::kBoardSpiPeripheralClockHz,
    });
    if (spi.configure().is_err()) {
        uart::write_line(debug, "[sd] spi configure failed");
        halt_blink(100);
    }

    CsPolicy cs{cs_gpio};
    Card card{spi, cs};

    if (card.init().is_err()) {
        uart::write_line(debug, "[sd] card init failed (insert SDHC/SDXC card)");
        halt_blink(100);
    }
    uart::write_line(debug, "[sd] card ok");

    Fso filesystem{Backend{card}};

    if (filesystem.mount().is_err()) {
        uart::write_line(debug, "[sd] mount failed (format FAT32 on host first)");
        halt_blink(100);
    }
    uart::write_line(debug, "[sd] mounted");

    // Read existing boot count from /boot.txt (0 if absent).
    std::uint32_t boot_count = 0;
    {
        auto r = filesystem.open("/boot.txt", fs::OpenMode::ReadOnly);
        if (r.is_ok()) {
            auto file = std::move(r).unwrap();
            std::array<char, 16> buf{};
            auto span = std::span<std::byte>{
                reinterpret_cast<std::byte*>(buf.data()), buf.size() - 1};
            if (auto rd = file.read(span); rd.is_ok()) {
                for (const char* p = buf.data(); *p >= '0' && *p <= '9'; ++p) {
                    boot_count = boot_count * 10u + static_cast<std::uint32_t>(*p - '0');
                }
            }
        }
    }
    ++boot_count;

    // Persist new boot count.
    {
        auto r = filesystem.open("/boot.txt", fs::OpenMode::Truncate);
        if (r.is_ok()) {
            auto file = std::move(r).unwrap();
            char num[12]{};
            const std::size_t n = fmt_u32(boot_count, num, sizeof(num));
            auto span = std::span<const std::byte>{
                reinterpret_cast<const std::byte*>(num), n};
            (void)file.write(span);
        }
    }

    // Append one log entry to /log.txt.
    const std::uint32_t tick =
        alloy::hal::SysTickTimer::millis<board::BoardSysTick>();

    char log_line[64]{};
    const std::size_t log_len = build_log_line(boot_count, tick, log_line, sizeof(log_line));

    {
        auto r = filesystem.open("/log.txt", fs::OpenMode::Append);
        if (r.is_ok()) {
            auto file = std::move(r).unwrap();
            auto span = std::span<const std::byte>{
                reinterpret_cast<const std::byte*>(log_line), log_len};
            (void)file.write(span);
        } else {
            uart::write_line(debug, "[sd] open log failed");
            halt_blink(100);
        }
    }

    uart::write_text(debug, "[sd] wrote: ");
    uart::write_text(debug, log_line);

    // Read the last line back for confirmation.
    {
        auto r = filesystem.open("/log.txt", fs::OpenMode::ReadOnly);
        if (r.is_ok()) {
            auto file = std::move(r).unwrap();
            // Seek to the last log_len bytes.
            (void)file.seek(-static_cast<std::int64_t>(log_len),
                            fs::SeekOrigin::End);
            std::array<char, 64> rbuf{};
            auto span = std::span<std::byte>{
                reinterpret_cast<std::byte*>(rbuf.data()), log_len};
            if (auto rd = file.read(span); rd.is_ok()) {
                rbuf[log_len] = '\0';
                uart::write_text(debug, "[sd] last log: ");
                uart::write_text(debug, rbuf.data());
            }
        }
    }

    uart::write_line(debug, "[sd] done");
    halt_blink(500);
}
