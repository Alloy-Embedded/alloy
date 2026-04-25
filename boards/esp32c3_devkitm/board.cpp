#include "board.hpp"

#include <cstdint>

namespace {

// GPIO base: 0x60004000
// IO_MUX base: 0x60009000
inline constexpr std::uint32_t kGpioBase  = 0x60004000u;
inline constexpr std::uint32_t kIoMuxBase = 0x60009000u;

// GPIO registers (offsets from kGpioBase)
inline auto& reg(std::uint32_t offset) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kGpioBase + offset);
}

// IO_MUX_GPIOn_REG = kIoMuxBase + 0x04 + n*4
inline auto& iomux(std::uint32_t pin) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kIoMuxBase + 0x04u + pin * 4u);
}

// GPIO_FUNCn_OUT_SEL_CFG = kGpioBase + 0x554 + n*4
inline auto& func_out_sel(std::uint32_t pin) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kGpioBase + 0x554u + pin * 4u);
}

inline constexpr std::uint32_t kLedPin = 8u;  // GPIO8 (WS2812 data, used as output)

// SIG_GPIO_OUT_IDX = 128 — routes GPIO_OUT register to the pad
inline constexpr std::uint32_t kSigGpioOut = 128u;

void gpio_init_output(std::uint32_t pin) noexcept {
    // IO_MUX: MCU_SEL = 1 (GPIO function) at bits [12:10]
    iomux(pin) = (1u << 10u);
    // Route pad output to GPIO register (software control)
    func_out_sel(pin) = kSigGpioOut;
    // Enable output
    reg(0x24u) = (1u << pin);   // GPIO_ENABLE_W1TS_REG
}

void gpio_set_high(std::uint32_t pin) noexcept {
    reg(0x08u) = (1u << pin);   // GPIO_OUT_W1TS_REG
}

void gpio_set_low(std::uint32_t pin) noexcept {
    reg(0x0Cu) = (1u << pin);   // GPIO_OUT_W1TC_REG
}

static bool s_initialized = false;

}  // namespace

namespace board {

namespace led {

void init() {
    gpio_init_output(kLedPin);
    off();
}

void on()     { gpio_set_high(kLedPin); }
void off()    { gpio_set_low(kLedPin);  }
void toggle() {
    // Read current output state from GPIO_OUT_REG (offset 0x04)
    static bool state = false;
    state ? off() : on();
    state = !state;
}

}  // namespace led

void init() {
    if (s_initialized) { return; }
    led::init();
    s_initialized = true;
}

}  // namespace board
