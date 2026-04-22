#include <cstdint>

#if !defined(ALLOY_BOARD_NUCLEO_G071RB)
    #error "clock_boot_probe is currently only supported on nucleo_g071rb"
#endif

#include "device/clock_config.hpp"

namespace {

constexpr std::uintptr_t kRccBase = 0x40021000u;
constexpr std::uintptr_t kGpioABase = 0x50000000u;

constexpr std::uintptr_t kRccIopenr = kRccBase + 0x34u;
constexpr std::uintptr_t kGpioModer = kGpioABase + 0x00u;
constexpr std::uintptr_t kGpioBsrr = kGpioABase + 0x18u;

constexpr std::uint32_t kPin5 = 5u;
constexpr std::uint32_t kGpioAEnable = 1u << 0u;
constexpr std::uint32_t kPin5ModeMask = 0x3u << (kPin5 * 2u);
constexpr std::uint32_t kPin5OutputMode = 0x1u << (kPin5 * 2u);
constexpr std::uint32_t kPin5Set = 1u << kPin5;
constexpr std::uint32_t kPin5Reset = 1u << (kPin5 + 16u);

inline auto mmio32(std::uintptr_t address) -> volatile std::uint32_t& {
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

inline void busy_delay(std::uint32_t cycles) {
    for (volatile std::uint32_t i = 0; i < cycles; ++i) {
    }
}

inline void enable_gpioa() {
    mmio32(kRccIopenr) |= kGpioAEnable;
}

inline void configure_pa5_output() {
    auto moder = mmio32(kGpioModer);
    moder &= ~kPin5ModeMask;
    moder |= kPin5OutputMode;
    mmio32(kGpioModer) = moder;
}

inline void led_on() {
    mmio32(kGpioBsrr) = kPin5Set;
}

inline void led_off() {
    mmio32(kGpioBsrr) = kPin5Reset;
}

}  // namespace

int main() {
    const auto clock_ok = alloy::device::clock_config::apply_default();

    enable_gpioa();
    configure_pa5_output();

    if (!clock_ok) {
        while (true) {
            led_on();
            busy_delay(2'000'000u);
            led_off();
            busy_delay(2'000'000u);
        }
    }

    while (true) {
        led_on();
        busy_delay(12'000'000u);
        led_off();
        busy_delay(12'000'000u);
    }
}
