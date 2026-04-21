#include <cstdint>
#include <string_view>

#include BOARD_HEADER

#include "hal/systick.hpp"

#if !defined(ALLOY_BOARD_SAME70_XPLAINED) && !defined(ALLOY_BOARD_SAME70_XPLD)
#error "manual_uart_probe is SAME70-only"
#endif

namespace {

constexpr std::uintptr_t kPmcBase = 0x400E0600u;
constexpr std::uintptr_t kPioABase = 0x400E0E00u;
constexpr std::uintptr_t kPioBBase = 0x400E1000u;
constexpr std::uintptr_t kUsart1Base = 0x40028000u;

constexpr std::uintptr_t kPmcPcer0 = kPmcBase + 0x0010u;

constexpr std::uintptr_t kPioPdr = 0x0004u;
constexpr std::uintptr_t kPioAbcdsr0 = 0x0070u;
constexpr std::uintptr_t kPioAbcdsr1 = 0x0074u;

constexpr std::uintptr_t kUsCr = 0x0000u;
constexpr std::uintptr_t kUsMr = 0x0004u;
constexpr std::uintptr_t kUsCsr = 0x0014u;
constexpr std::uintptr_t kUsThr = 0x001Cu;
constexpr std::uintptr_t kUsBrgr = 0x0020u;

constexpr std::uint32_t kPidPioa = 10u;
constexpr std::uint32_t kPidPiob = 11u;
constexpr std::uint32_t kPidUsart1 = 14u;

constexpr std::uint32_t kPa21 = 21u;
constexpr std::uint32_t kPb4 = 4u;

constexpr std::uint32_t kUsCrRstrx = 1u << 2u;
constexpr std::uint32_t kUsCrRsttx = 1u << 3u;
constexpr std::uint32_t kUsCrRxen = 1u << 4u;
constexpr std::uint32_t kUsCrRxdis = 1u << 5u;
constexpr std::uint32_t kUsCrTxen = 1u << 6u;
constexpr std::uint32_t kUsCrTxdis = 1u << 7u;
constexpr std::uint32_t kUsCrRststa = 1u << 8u;

constexpr std::uint32_t kUsCsrTxrdy = 1u << 1u;
constexpr std::uint32_t kUsCsrTxempty = 1u << 9u;

constexpr std::uint32_t kUsMrNormal = 0u << 0u;
constexpr std::uint32_t kUsMrMck = 0u << 4u;
constexpr std::uint32_t kUsMr8Bit = 3u << 6u;
constexpr std::uint32_t kUsMrNoParity = 4u << 9u;
constexpr std::uint32_t kUsMr1Stop = 0u << 12u;

constexpr std::uint32_t kPclkHz = board::same70_xplained::ClockConfig::pclk_freq_hz;
constexpr std::uint32_t kBaud = 115200u;
constexpr std::uint32_t kCd = (kPclkHz + ((16u * kBaud) / 2u)) / (16u * kBaud);

inline auto mmio32(std::uintptr_t address) -> volatile std::uint32_t& {
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

void enable_peripheral(std::uint32_t pid) {
    mmio32(kPmcPcer0) = (1u << pid);
}

void configure_pio_selector(std::uintptr_t pio_base, std::uint32_t line, std::uint32_t selector) {
    const auto mask = (1u << line);
    mmio32(pio_base + kPioPdr) = mask;

    auto abcdsr0 = mmio32(pio_base + kPioAbcdsr0);
    auto abcdsr1 = mmio32(pio_base + kPioAbcdsr1);

    if ((selector & 0x1u) != 0u) {
        abcdsr0 |= mask;
    } else {
        abcdsr0 &= ~mask;
    }

    if ((selector & 0x2u) != 0u) {
        abcdsr1 |= mask;
    } else {
        abcdsr1 &= ~mask;
    }

    mmio32(pio_base + kPioAbcdsr0) = abcdsr0;
    mmio32(pio_base + kPioAbcdsr1) = abcdsr1;
}

void usart1_init_8n1() {
    enable_peripheral(kPidPioa);
    enable_peripheral(kPidPiob);
    enable_peripheral(kPidUsart1);

    // SAME70 Xplained Ultra EDBG VCOM:
    // PB4 -> USART1_TXD1, selector D (3)
    // PA21 -> USART1_RXD1, selector A (0)
    configure_pio_selector(kPioBBase, kPb4, 3u);
    configure_pio_selector(kPioABase, kPa21, 0u);

    mmio32(kUsart1Base + kUsCr) = kUsCrRstrx | kUsCrRsttx | kUsCrRxdis | kUsCrTxdis | kUsCrRststa;
    mmio32(kUsart1Base + kUsMr) = kUsMrNormal | kUsMrMck | kUsMr8Bit | kUsMrNoParity | kUsMr1Stop;
    mmio32(kUsart1Base + kUsBrgr) = (kCd == 0u) ? 1u : kCd;
    mmio32(kUsart1Base + kUsCr) = kUsCrRxen | kUsCrTxen;
}

void usart1_write_byte(std::byte byte) {
    while ((mmio32(kUsart1Base + kUsCsr) & kUsCsrTxrdy) == 0u) {
    }
    mmio32(kUsart1Base + kUsThr) = std::to_integer<std::uint8_t>(byte);
}

void usart1_write(std::string_view text) {
    for (char ch : text) {
        usart1_write_byte(static_cast<std::byte>(ch));
    }
}

void usart1_flush() {
    while ((mmio32(kUsart1Base + kUsCsr) & kUsCsrTxempty) == 0u) {
    }
}

}  // namespace

int main() {
    board::init();
    usart1_init_8n1();

    while (true) {
        board::led::toggle();
        usart1_write("manual usart1 pb4/pa21 8n1\r\n");
        usart1_flush();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(250);
    }
}
