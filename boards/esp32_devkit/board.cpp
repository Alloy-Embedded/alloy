#include "board.hpp"

#include <cstdint>

namespace {

// Watchdog unlock key (same for TIMG and RTC WDTs)
inline constexpr std::uint32_t kWdtUnlockKey = 0x50D83AA1u;

inline auto& reg32(std::uint32_t addr) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}

// Disable TIMG0 and TIMG1 main watchdogs + RTC watchdog.
// The ESP-IDF v5 bootloader leaves TIMG0 WDT running (~1s timeout).
// Without this, the chip resets before the second loop iteration.
void disable_watchdogs() noexcept {
    // TIMG0 WDT: WDTPROTECT=0x3FF5F064, WDTCONFIG0=0x3FF5F048 (EN=bit31)
    reg32(0x3FF5F064u) = kWdtUnlockKey;
    reg32(0x3FF5F048u) &= ~(1u << 31u);
    reg32(0x3FF5F064u) = 0u;

    // TIMG1 WDT: WDTPROTECT=0x3FF60064, WDTCONFIG0=0x3FF60048
    reg32(0x3FF60064u) = kWdtUnlockKey;
    reg32(0x3FF60048u) &= ~(1u << 31u);
    reg32(0x3FF60064u) = 0u;

    // RTC WDT: WDTPROTECT=0x3FF480A8, WDTCONFIG0=0x3FF48094 (EN=bit31)
    reg32(0x3FF480A8u) = kWdtUnlockKey;
    reg32(0x3FF48094u) &= ~(1u << 31u);
    reg32(0x3FF480A8u) = 0u;
}

// GPIO base: 0x3FF44000
// IO_MUX base: 0x3FF49000
inline constexpr std::uint32_t kGpioBase  = 0x3FF44000u;
inline constexpr std::uint32_t kIoMuxBase = 0x3FF49000u;

inline auto& gpio_reg(std::uint32_t offset) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kGpioBase + offset);
}

// IO_MUX_GPIOn_REG = kIoMuxBase + 0x04 + n*4 (ESP32 IO_MUX table)
inline auto& iomux_reg(std::uint32_t pin) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kIoMuxBase + 0x04u + pin * 4u);
}

// GPIO_FUNCn_OUT_SEL_CFG_REG = kGpioBase + 0x530 + n*4
inline auto& func_out_sel(std::uint32_t pin) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kGpioBase + 0x530u + pin * 4u);
}

inline constexpr std::uint32_t kLedPin    = 2u;    // GPIO2 built-in LED
inline constexpr std::uint32_t kMcuSelGpio = 2u;   // GPIO function in IO_MUX MCU_SEL
inline constexpr std::uint32_t kSigGpioOut = 256u; // SIG_GPIO_OUT_IDX

void gpio_output_init(std::uint32_t pin) noexcept {
    // IO_MUX: set MCU_SEL = 2 (GPIO function) at bits [12:10]
    iomux_reg(pin) = (kMcuSelGpio << 10u);
    // Route to GPIO matrix software control
    func_out_sel(pin) = kSigGpioOut;
    // Enable output
    gpio_reg(0x24u) = (1u << pin);   // GPIO_ENABLE_W1TS_REG
}

void gpio_high(std::uint32_t pin) noexcept { gpio_reg(0x08u) = (1u << pin); }
void gpio_low (std::uint32_t pin) noexcept { gpio_reg(0x0Cu) = (1u << pin); }

static bool s_initialized = false;

}  // namespace

namespace board {

namespace led {

void init()   { gpio_output_init(kLedPin); off(); }
void on()     { gpio_high(kLedPin); }
void off()    { gpio_low(kLedPin);  }
void toggle() {
    static bool state = false;
    state ? off() : on();
    state = !state;
}

}  // namespace led

void init() {
    if (s_initialized) { return; }
    disable_watchdogs();
    led::init();
    s_initialized = true;
}

}  // namespace board
