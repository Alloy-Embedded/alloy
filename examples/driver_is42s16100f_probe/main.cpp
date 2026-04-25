// examples/driver_is42s16100f_probe/main.cpp
//
// SAME70 Xplained Ultra — IS42S16100F-5B SDRAM chip-descriptor probe.
//
// Target: the ISSI IS42S16100F-5B SDRAM (U6) on the SAME70 Xplained Ultra
// board, wired to the SAME70 SDRAMC. The board pulls CS, the SDRAM data bus
// (D0..D15), the address bus (A0..A10 + BA0), and the command signals (RAS,
// CAS, WE, CKE, CLK, SDCK, NANDWE, NANDOE, NBS0, NBS1) to the matching
// peripheral-A pins on PIOA/PIOC/PIOD as documented in the board schematic.
//
// The `alloy::drivers::memory::is42s16100f` header is a *chip descriptor*:
// constexpr geometry + datasheet ns timings + a `timings_for_bus(hclk_hz)`
// helper that returns the ns values rounded up to bus clocks. No driver
// class, no bus handle — the SDRAMC init flow lives in the application or
// board code and consumes the descriptor. This example bundles the init
// flow inline so the probe is a single, self-contained binary.
//
// What the probe does, in order:
//   1. Initialises the board debug UART (115200-8-N-1).
//   2. Prints the descriptor: capacity, bus width, CAS latency, and the
//      timings for the current MCK — handy for spotting a bad clock preset.
//   3. Enables PMC peripheral clocks for PIOA/PIOC/PIOD and SDRAMC (PID 62).
//   4. Muxes every SDRAM pin to its peripheral assignment.
//   5. Sets MATRIX.CCFG_SMCNFCS.SDRAMEN so the 0x70000000 window routes to
//      SDRAMC.
//   6. Programs SDRAMC.CR + MDR + TR from the constexpr timings, then runs
//      the JEDEC init sequence (NOP → PALL → 8x REFRESH → LMR → NORMAL).
//   7. Walks the first 8 KiB of SDRAM with an address-XOR pattern, reads it
//      back, and verifies byte-for-byte.
//
// Expected UART output on a healthy board:
//
//   is42s16100f probe: ready
//   is42s16100f: capacity = 2097152 bytes (16 Mbit), x16 bus, CAS=3
//   is42s16100f: timings @ MCK = t_rc=9 t_rcd=3 t_rp=3 t_ras=6 t_wr=3 t_rfc=9 refresh=4688
//   is42s16100f: SDRAMC init ok
//   is42s16100f: 8 KiB pattern PASS
//   is42s16100f: PROBE PASS
//
// On failure the line preceding FAIL tells you which step broke:
//
//   FAIL (pattern mismatch @ 0x70000XXX: wrote=YY read=ZZ)
//
// usually means one of the data lines is stuck or a PIO mux bit was missed.
// LED blinks slowly (500 ms) on pass, rapidly (100 ms) on failure.

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "driver_is42s16100f_probe requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER

#include "drivers/memory/is42s16100f/is42s16100f.hpp"
#include "device/runtime.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace {

using namespace alloy::examples::uart_console;

inline volatile std::uint32_t& reg32(std::uintptr_t addr) {
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}
inline volatile std::uint16_t& reg16(std::uintptr_t addr) {
    return *reinterpret_cast<volatile std::uint16_t*>(addr);
}
inline volatile std::uint8_t& reg8(std::uintptr_t addr) {
    return *reinterpret_cast<volatile std::uint8_t*>(addr);
}

// PMC -----------------------------------------------------------------------
// SDRAMC is not yet exposed as a PeripheralId by the runtime contract; once it
// is, the raw PCER1 write below should switch to alloy::clock::enable<SDRAMC>().
constexpr std::uintptr_t kPmcPcer1 = 0x400E0700u;
constexpr std::uint32_t kPidSdramc = 62;  // PCER1 bit 30

// MATRIX (system controller) — not published as a PeripheralId; raw literal.
constexpr std::uintptr_t kMatrixCcfgSmcnfcs = 0x40088124u;
constexpr std::uint32_t kCcfgSdramen = 1u << 4;

// SDRAMC --------------------------------------------------------------------
constexpr std::uintptr_t kSdramcBase = 0x40084000u;
constexpr std::uintptr_t kSdramcMr = kSdramcBase + 0x00;
constexpr std::uintptr_t kSdramcTr = kSdramcBase + 0x04;
constexpr std::uintptr_t kSdramcCr = kSdramcBase + 0x08;
constexpr std::uintptr_t kSdramcMdr = kSdramcBase + 0x24;
constexpr std::uintptr_t kSdramcCfr1 = kSdramcBase + 0x28;

constexpr std::uint32_t kSdramcModeNormal = 0;
constexpr std::uint32_t kSdramcModeNop = 1;
constexpr std::uint32_t kSdramcModeAllBanks = 2;
constexpr std::uint32_t kSdramcModeLmr = 3;
constexpr std::uint32_t kSdramcModeAutoRefresh = 4;

constexpr std::uintptr_t kSdramcWindow = 0x70000000u;

// PIO ------------------------------------------------------------------------
constexpr std::uintptr_t kPioaBase =
    alloy::device::base<alloy::device::PeripheralId::GPIOA>();
constexpr std::uintptr_t kPiocBase =
    alloy::device::base<alloy::device::PeripheralId::GPIOC>();
constexpr std::uintptr_t kPiodBase =
    alloy::device::base<alloy::device::PeripheralId::GPIOD>();
constexpr std::uintptr_t kPioeBase =
    alloy::device::base<alloy::device::PeripheralId::GPIOE>();
inline std::uintptr_t pio_pdr(std::uintptr_t base)     { return base + 0x04; }
inline std::uintptr_t pio_abcdsr1(std::uintptr_t base) { return base + 0x70; }
inline std::uintptr_t pio_abcdsr2(std::uintptr_t base) { return base + 0x74; }

// SAME70 Xplained Ultra SDRAM pin mux (per ASF board file — mix of periph A + C):
//   Peripheral A pads:
//     PIOA: PA15 = D14, PA16 = D15
//     PIOC: PC0..PC7 = D0..D7, PC15 = SDCS, PC18 = NBS0,
//           PC20..PC29 = A2..A9 / SDA0..SDA7 / A10 / A11
//     PIOE: PE0..PE5 = D8..D13
//   Peripheral C pads:
//     PIOA: PA20 = BA0
//     PIOD: PD13 = SDA10, PD14 = SDCKE, PD15 = NBS1, PD16 = RAS, PD17 = CAS,
//           PD23 = SDCK, PD29 = SDWE
// Selector encoding (per-pin bit pair across ABCDSR1/ABCDSR2):
//   A = ABCDSR1=0, ABCDSR2=0   C = ABCDSR1=0, ABCDSR2=1
// Previous probe revisions muxed everything to peripheral A, which left the
// upper data byte (D8..D15) floating and routed PIOD command pins to their
// GMAC/USART alt functions — writes silently NACKed on odd byte lanes and
// every refresh command landed on the wrong pad.

constexpr std::uint32_t kPioaSdramMaskA = (1u << 15) | (1u << 16);          // D14, D15
constexpr std::uint32_t kPioaSdramMaskC = (1u << 20);                       // BA0
constexpr std::uint32_t kPiocSdramMaskA = 0x3FFFFFFFu;                      // PC0..PC29
constexpr std::uint32_t kPiodSdramMaskC = (1u << 13) | (1u << 14) | (1u << 15) |
                                          (1u << 16) | (1u << 17) |
                                          (1u << 23) | (1u << 29);
constexpr std::uint32_t kPioeSdramMaskA = 0x3Fu;                            // PE0..PE5 = D8..D13

void mux_sdram_pins() {
    // PIOA: PA15, PA16 → periph A; PA20 → periph C.
    const std::uint32_t piao_mask = kPioaSdramMaskA | kPioaSdramMaskC;
    std::uint32_t a1 = reg32(pio_abcdsr1(kPioaBase));
    std::uint32_t a2 = reg32(pio_abcdsr2(kPioaBase));
    a1 &= ~piao_mask;                                  // clear ABCDSR1 for both A and C
    a2 = (a2 & ~piao_mask) | kPioaSdramMaskC;          // ABCDSR2=1 for periph-C pins
    reg32(pio_abcdsr1(kPioaBase)) = a1;
    reg32(pio_abcdsr2(kPioaBase)) = a2;
    reg32(pio_pdr(kPioaBase)) = piao_mask;

    // PIOC: all periph A.
    reg32(pio_abcdsr1(kPiocBase)) &= ~kPiocSdramMaskA;
    reg32(pio_abcdsr2(kPiocBase)) &= ~kPiocSdramMaskA;
    reg32(pio_pdr(kPiocBase)) = kPiocSdramMaskA;

    // PIOD: all periph C.
    reg32(pio_abcdsr1(kPiodBase)) &= ~kPiodSdramMaskC;
    std::uint32_t d2 = reg32(pio_abcdsr2(kPiodBase));
    reg32(pio_abcdsr2(kPiodBase)) = (d2 & ~kPiodSdramMaskC) | kPiodSdramMaskC;
    reg32(pio_pdr(kPiodBase)) = kPiodSdramMaskC;

    // PIOE: all periph A.
    reg32(pio_abcdsr1(kPioeBase)) &= ~kPioeSdramMaskA;
    reg32(pio_abcdsr2(kPioeBase)) &= ~kPioeSdramMaskA;
    reg32(pio_pdr(kPioeBase)) = kPioeSdramMaskA;
}

void enable_pmc_clocks() {
    using PId = alloy::device::PeripheralId;
    alloy::clock::enable<PId::GPIOA>();
    alloy::clock::enable<PId::GPIOC>();
    alloy::clock::enable<PId::GPIOD>();
    alloy::clock::enable<PId::GPIOE>();
    // SDRAMC has no PeripheralId yet; raw PCER1 write until codegen exposes it.
    reg32(kPmcPcer1) = (1u << (kPidSdramc - 32));
}

// SDRAMC command issue: write MODE, then touch any SDRAM address.
void sdramc_issue(std::uint32_t mode) {
    reg32(kSdramcMr) = mode;
    // Touch SDRAM — value is don't-care for non-NORMAL modes except LMR where
    // the address bits carry the mode register content and are supplied via
    // the SDRAMC itself.
    reg16(kSdramcWindow) = 0;
    (void)reg16(kSdramcWindow);
}

void busy_wait_us(std::uint32_t us) {
    // ~1 cycle per iteration at -O0; overshoots at higher opt, which is fine
    // for datasheet delays measured in hundreds of microseconds.
    const std::uint32_t cycles =
        (board::same70_xplained::ClockConfig::cpu_freq_hz / 1'000'000u) * us;
    for (volatile std::uint32_t i = 0; i < cycles; ++i) {
        __asm__ volatile("nop");
    }
}

// Pack SDRAMC_CR from the constexpr descriptor + runtime-computed timings.
[[nodiscard]] std::uint32_t build_cr(
    const alloy::drivers::memory::is42s16100f::TimingsCycles& t) {
    constexpr std::uint32_t nc_8bit = 0;     // 256 columns
    constexpr std::uint32_t nr_11bit = 0;    // 2048 rows
    constexpr std::uint32_t nb_2 = 0;        // 2 banks
    constexpr std::uint32_t cas_3 = 3u << 5; // CAS=3
    constexpr std::uint32_t dbw_16 = 1u << 7;

    const std::uint32_t twr = (t.t_wr & 0xFu) << 8;
    const std::uint32_t trc = (t.t_rc & 0xFu) << 12;
    const std::uint32_t trp = (t.t_rp & 0xFu) << 16;
    const std::uint32_t trcd = (t.t_rcd & 0xFu) << 20;
    const std::uint32_t tras = (t.t_ras & 0xFu) << 24;
    const std::uint32_t txsr = (t.t_rc & 0xFu) << 28;  // conservative: t_xsr ~= t_rc
    return nc_8bit | nr_11bit | nb_2 | cas_3 | dbw_16 |
           twr | trc | trp | trcd | tras | txsr;
}

void sdramc_init(const alloy::drivers::memory::is42s16100f::TimingsCycles& t) {
    reg32(kSdramcCr) = build_cr(t);
    reg32(kSdramcMdr) = 0;      // 0 = SDRAM (not low-power SDRAM)
    reg32(kSdramcCfr1) = (1u << 8) | t.t_rfc;  // UNAL=1 + TMRD field low byte

    // JEDEC init sequence for a 2-bank x16 SDRAM (datasheet §3.1):
    //   1) CKE high + stable clocks for >= 200 us  (board POR provides this;
    //      we wait again to be safe).
    busy_wait_us(250);

    sdramc_issue(kSdramcModeNop);
    busy_wait_us(10);
    sdramc_issue(kSdramcModeAllBanks);      // PALL
    busy_wait_us(10);

    for (int i = 0; i < 8; ++i) {           // 8x AUTO REFRESH
        sdramc_issue(kSdramcModeAutoRefresh);
        busy_wait_us(10);
    }

    sdramc_issue(kSdramcModeLmr);           // LOAD MODE REG
    busy_wait_us(10);

    // Refresh timer: refresh command once per t.refresh_period bus cycles.
    reg32(kSdramcTr) = t.refresh_period & 0xFFFu;

    sdramc_issue(kSdramcModeNormal);
    busy_wait_us(10);
}

// Pattern walk over [0x70000000, 0x70000000 + size). Writes byte-XOR of the
// address low bytes; reads back and compares.
[[nodiscard]] bool ram_pattern_check(std::size_t size,
                                     std::uintptr_t& first_bad_addr,
                                     std::uint8_t& wrote,
                                     std::uint8_t& got) {
    auto* p = reinterpret_cast<volatile std::uint8_t*>(kSdramcWindow);
    for (std::size_t i = 0; i < size; ++i) {
        p[i] = static_cast<std::uint8_t>((i ^ (i >> 8)) & 0xFFu);
    }
    for (std::size_t i = 0; i < size; ++i) {
        const auto expected = static_cast<std::uint8_t>((i ^ (i >> 8)) & 0xFFu);
        const std::uint8_t actual = p[i];
        if (actual != expected) {
            first_bad_addr = kSdramcWindow + i;
            wrote = expected;
            got = actual;
            return false;
        }
    }
    return true;
}

template <typename Uart>
void print_hex32(const Uart& uart, std::uint32_t v) {
    constexpr auto kHex = "0123456789ABCDEF";
    char buf[9];
    for (int i = 0; i < 8; ++i) buf[i] = kHex[(v >> ((7 - i) * 4)) & 0xF];
    buf[8] = '\0';
    write_text(uart, std::string_view{buf, 8});
}

template <typename Uart>
void print_hex8(const Uart& uart, std::uint8_t v) {
    constexpr auto kHex = "0123456789ABCDEF";
    char buf[3] = {kHex[(v >> 4) & 0xF], kHex[v & 0xF], '\0'};
    write_text(uart, std::string_view{buf, 2});
}

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

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (uart.configure().is_err()) {
        blink_error(100);
    }
    write_line(uart, "is42s16100f probe: ready");

    namespace desc = alloy::drivers::memory::is42s16100f;
    constexpr auto capacity_bytes = (desc::kCapacityBits / 8u);
    write_text(uart, "is42s16100f: capacity = ");
    write_unsigned(uart, capacity_bytes);
    write_text(uart, " bytes (16 Mbit), x16 bus, CAS=");
    write_unsigned(uart, desc::kCasLatencyCycles);
    write_text(uart, "\r\n");

    const auto t = desc::timings_for_bus(
        board::same70_xplained::ClockConfig::pclk_freq_hz);
    write_text(uart, "is42s16100f: timings @ MCK = t_rc=");
    write_unsigned(uart, t.t_rc);
    write_text(uart, " t_rcd=");
    write_unsigned(uart, t.t_rcd);
    write_text(uart, " t_rp=");
    write_unsigned(uart, t.t_rp);
    write_text(uart, " t_ras=");
    write_unsigned(uart, t.t_ras);
    write_text(uart, " t_wr=");
    write_unsigned(uart, t.t_wr);
    write_text(uart, " t_rfc=");
    write_unsigned(uart, t.t_rfc);
    write_text(uart, " refresh=");
    write_unsigned(uart, t.refresh_period);
    write_text(uart, "\r\n");

    enable_pmc_clocks();
    mux_sdram_pins();
    reg32(kMatrixCcfgSmcnfcs) |= kCcfgSdramen;

    sdramc_init(t);
    write_line(uart, "is42s16100f: SDRAMC init ok");

    constexpr std::size_t kTestBytes = 8u * 1024u;
    std::uintptr_t bad_addr = 0;
    std::uint8_t wrote = 0;
    std::uint8_t got = 0;
    if (!ram_pattern_check(kTestBytes, bad_addr, wrote, got)) {
        write_text(uart, "is42s16100f: FAIL (pattern mismatch @ 0x");
        print_hex32(uart, static_cast<std::uint32_t>(bad_addr));
        write_text(uart, ": wrote=");
        print_hex8(uart, wrote);
        write_text(uart, " read=");
        print_hex8(uart, got);
        write_line(uart, ")");
        blink_error(100);
    }

    write_line(uart, "is42s16100f: 8 KiB pattern PASS");
    write_line(uart, "is42s16100f: PROBE PASS");
    blink_ok();
}
