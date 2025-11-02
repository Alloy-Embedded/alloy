/// RP2040 RESETS Peripheral Control
///
/// The RESETS peripheral controls reset state for all peripherals.
/// Before using any peripheral, it must be released from reset.

#ifndef ALLOY_HAL_RP2040_RESETS_HPP
#define ALLOY_HAL_RP2040_RESETS_HPP

#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040 {

/// RESETS register structure (RP2040 datasheet section 2.14)
struct RESETS_Registers {
    volatile uint32_t RESET;          // 0x00: Reset control
    volatile uint32_t WDSEL;          // 0x04: Watchdog select
    volatile uint32_t RESET_DONE;     // 0x08: Reset done status
};

/// RESETS peripheral base address
static constexpr uint32_t RESETS_BASE = 0x4000C000;

/// Get RESETS peripheral
inline RESETS_Registers* get_resets() {
    return reinterpret_cast<RESETS_Registers*>(RESETS_BASE);
}

/// Reset bits for each peripheral
namespace ResetBits {
    constexpr uint32_t ADC        = (1 << 0);
    constexpr uint32_t BUSCTRL    = (1 << 1);
    constexpr uint32_t DMA        = (1 << 2);
    constexpr uint32_t I2C0       = (1 << 3);
    constexpr uint32_t I2C1       = (1 << 4);
    constexpr uint32_t IO_BANK0   = (1 << 5);
    constexpr uint32_t IO_QSPI    = (1 << 6);
    constexpr uint32_t JTAG       = (1 << 7);
    constexpr uint32_t PADS_BANK0 = (1 << 8);
    constexpr uint32_t PADS_QSPI  = (1 << 9);
    constexpr uint32_t PIO0       = (1 << 10);
    constexpr uint32_t PIO1       = (1 << 11);
    constexpr uint32_t PLL_SYS    = (1 << 12);
    constexpr uint32_t PLL_USB    = (1 << 13);
    constexpr uint32_t PWM        = (1 << 14);
    constexpr uint32_t RTC        = (1 << 15);
    constexpr uint32_t SPI0       = (1 << 16);
    constexpr uint32_t SPI1       = (1 << 17);
    constexpr uint32_t SYSCFG     = (1 << 18);
    constexpr uint32_t SYSINFO    = (1 << 19);
    constexpr uint32_t TBMAN      = (1 << 20);
    constexpr uint32_t TIMER      = (1 << 21);
    constexpr uint32_t UART0      = (1 << 22);
    constexpr uint32_t UART1      = (1 << 23);
    constexpr uint32_t USBCTRL    = (1 << 24);
}

/// Reset peripheral (put it into reset)
inline void reset_peripheral(uint32_t bits) {
    auto* resets = get_resets();
    resets->RESET |= bits;
}

/// Unreset peripheral (release from reset)
inline void unreset_peripheral(uint32_t bits) {
    auto* resets = get_resets();
    resets->RESET &= ~bits;
}

/// Wait for peripheral to complete reset
inline void wait_for_reset_done(uint32_t bits) {
    auto* resets = get_resets();
    while ((resets->RESET_DONE & bits) != bits) {
        // Wait for reset to complete
    }
}

/// Reset and unreset peripheral (complete reset cycle)
inline void reset_peripheral_blocking(uint32_t bits) {
    reset_peripheral(bits);
    unreset_peripheral(bits);
    wait_for_reset_done(bits);
}

} // namespace alloy::hal::raspberrypi::rp2040

#endif // ALLOY_HAL_RP2040_RESETS_HPP
