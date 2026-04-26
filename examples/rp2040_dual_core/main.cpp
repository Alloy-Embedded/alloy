// RP2040 dual-core demo.
//
// Core 0 (Cortex-M0+ #0): blinks GP25 (onboard LED) at 1 Hz and forwards
//                          messages from CrossCoreChannel to UART0.
//
// Core 1 (Cortex-M0+ #1): pushes an incrementing counter every ~500 ms.
//
// CrossCoreChannel<uint32_t, 64> carries values core1 → core0 using
// acquire/release atomics — safe across both RP2040 cores.
//
// UART0: GP0=TX, 115200-8-N-1. The RP2040 bootloader does NOT pre-configure
// UART, so we initialise it from scratch (see uart0_init()).

#include <cstdint>

#include "boards/raspberry_pi_pico/board_multicore.hpp"
#include "runtime/cross_core_channel.hpp"

// ---- raw MMIO helpers -------------------------------------------------------

static auto& mmio(std::uint32_t addr) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}

// ---- UART0 raw MMIO (GP0=TX) ------------------------------------------------
// Base address: 0x40034000

static constexpr std::uint32_t kUart0Base   = 0x40034000u;
static constexpr std::uint32_t kUartDR      = kUart0Base + 0x000u;  // data
static constexpr std::uint32_t kUartIBRD    = kUart0Base + 0x024u;  // integer baud
static constexpr std::uint32_t kUartFBRD    = kUart0Base + 0x028u;  // fractional baud
static constexpr std::uint32_t kUartLCRH    = kUart0Base + 0x02Cu;  // line control
static constexpr std::uint32_t kUartCR      = kUart0Base + 0x030u;  // control
static constexpr std::uint32_t kUartFR      = kUart0Base + 0x018u;  // flags
static constexpr std::uint32_t kUartFRTxFull = 1u << 5;

// IO_BANK0: GP0 function select = 2 (UART0 TX)
static constexpr std::uint32_t kIoBank0Base  = 0x40014000u;
static constexpr std::uint32_t kGp0Ctrl      = kIoBank0Base + 0x004u;

// RESETS: bit 22 = UART0
static constexpr std::uint32_t kResetsBase   = 0x4000C000u;
static constexpr std::uint32_t kResetsReset  = kResetsBase + 0x000u;
static constexpr std::uint32_t kResetsDone   = kResetsBase + 0x008u;
static constexpr std::uint32_t kUart0Bit     = 1u << 22;

static void uart0_init() noexcept {
    // Release UART0 from reset
    mmio(kResetsReset) |= kUart0Bit;
    mmio(kResetsReset) &= ~kUart0Bit;
    while (!(mmio(kResetsDone) & kUart0Bit)) {}

    // Baud 115200 @ 125 MHz: IBR=67, FBR=53
    mmio(kUartIBRD) = 67u;
    mmio(kUartFBRD) = 53u;
    // 8 data bits, no parity, 1 stop, FIFO enabled (WLEN=0b11, FEN=1)
    mmio(kUartLCRH) = (0b11u << 5) | (1u << 4);
    // Enable UART: UARTEN | TXE
    mmio(kUartCR)   = (1u << 0) | (1u << 8);

    // Route GP0 to UART0 TX via IO bank0 function select = 2
    mmio(kGp0Ctrl) = 2u;
}

static void uart_putc(char c) noexcept {
    while (mmio(kUartFR) & kUartFRTxFull) {}
    mmio(kUartDR) = static_cast<std::uint32_t>(static_cast<unsigned char>(c));
}

static void uart_puts(const char* s) noexcept {
    while (*s) uart_putc(*s++);
}

static void uart_put_u32(std::uint32_t v) noexcept {
    if (v == 0u) { uart_putc('0'); return; }
    char buf[10];
    int n = 0;
    while (v) { buf[n++] = '0' + static_cast<char>(v % 10u); v /= 10u; }
    for (int i = n - 1; i >= 0; --i) uart_putc(buf[i]);
}

// ---- GPIO GP25 (onboard LED) ------------------------------------------------
// SIO GPIO output via IO_BANK0

static constexpr std::uint32_t kSioBase      = 0xD0000000u;
static constexpr std::uint32_t kSioGpioOe    = kSioBase + 0x020u;  // output enable set
static constexpr std::uint32_t kSioGpioOut   = kSioBase + 0x010u;  // output set
static constexpr std::uint32_t kSioGpioOutClr= kSioBase + 0x018u;  // output clear
static constexpr std::uint32_t kGp25Ctrl     = kIoBank0Base + (25u * 8u + 4u);
static constexpr std::uint32_t kGp25Bit      = 1u << 25u;

// PADS_BANK0 GP25 — enable output drive
static constexpr std::uint32_t kPadsBank0Base = 0x4001C000u;
static constexpr std::uint32_t kPadsGp25      = kPadsBank0Base + (1u + 25u) * 4u;

static void led_init() noexcept {
    // SIO function = 5
    mmio(kGp25Ctrl)  = 5u;
    // Enable output
    mmio(kSioGpioOe) = kGp25Bit;
}

static void led_on()  noexcept { mmio(kSioGpioOut)    = kGp25Bit; }
static void led_off() noexcept { mmio(kSioGpioOutClr)  = kGp25Bit; }

// ---- shared channel ---------------------------------------------------------

static alloy::tasks::CrossCoreChannel<std::uint32_t, 64> g_channel;

// ---- busy-wait (~1 ms @ 125 MHz) --------------------------------------------

static void busy_ms(std::uint32_t ms) noexcept {
    for (std::uint32_t i = 0; i < ms * 125'000u; ++i) {
        __asm volatile("nop");
    }
}

// ---- core 1 entry -----------------------------------------------------------

static void core1_main() noexcept {
    std::uint32_t counter = 0u;
    while (true) {
        (void)g_channel.try_push(counter++);
        busy_ms(500u);
    }
}

// ---- core 0 entry -----------------------------------------------------------

extern "C" [[noreturn]] void alloy_main() {
    uart0_init();
    led_init();

    uart_puts("[core0] starting\r\n");

    board::launch_core1([] { core1_main(); });
    uart_puts("[core0] core1 launched\r\n");

    bool led_state = false;
    std::uint32_t blink_timer = 0u;

    while (true) {
        while (auto val = g_channel.try_pop()) {
            uart_puts("[core0] recv: ");
            uart_put_u32(*val);
            uart_puts("\r\n");
        }

        busy_ms(1u);
        if (++blink_timer >= 500u) {
            blink_timer = 0u;
            led_state ? led_off() : led_on();
            led_state = !led_state;
        }
    }
}
