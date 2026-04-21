#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/pwm.hpp"
#include "hal/timer.hpp"
#include "hal/watchdog.hpp"

namespace rt = alloy::hal::detail::runtime;

namespace {

using FieldId = alloy::device::runtime::FieldId;

constexpr auto kWdtMrAddress = std::uintptr_t{0x400E1854u};
constexpr auto kGpiocSodrAddress = std::uintptr_t{0x400E1230u};
constexpr auto kUsart0UsMrAddress = std::uintptr_t{0x40024004u};
constexpr auto kTc0RcAddress = std::uintptr_t{0x4000C01Cu};
constexpr auto kPwm0Cdty0Address = std::uintptr_t{0x40020204u};
constexpr auto kPwm0Cprd0Address = std::uintptr_t{0x4002020Cu};
constexpr auto kPioLine8Mask = std::uint32_t{1u << 8u};
constexpr auto kUsartChrlEightBits = std::uint32_t{3u};
constexpr auto kUsartChrlMask = std::uint32_t{0x3u << 6u};

}  // namespace

extern "C" __attribute__((noinline, used)) void manual_disable_same70_watchdog() {
    *reinterpret_cast<volatile std::uint32_t*>(kWdtMrAddress) = 0x0FFF8000u;
}

extern "C" __attribute__((noinline, used)) void runtime_disable_same70_watchdog() {
    static_cast<void>(
        alloy::hal::watchdog::open<alloy::device::runtime::PeripheralId::WDT>().disable());
}

extern "C" __attribute__((noinline, used)) void manual_set_same70_pioc8_high() {
    *reinterpret_cast<volatile std::uint32_t*>(kGpiocSodrAddress) = kPioLine8Mask;
}

extern "C" __attribute__((noinline, used)) void runtime_set_same70_pioc8_high() {
    constexpr auto set_field = rt::field_ref<FieldId::field_gpioc_sodr_p8>();
    const auto set_bits = rt::field_bits(set_field, 1u).unwrap();
    static_cast<void>(rt::write_register(set_field.reg, set_bits));
}

extern "C" __attribute__((noinline, used)) void manual_set_same70_usart0_chrl() {
    auto& reg = *reinterpret_cast<volatile std::uint32_t*>(kUsart0UsMrAddress);
    auto current = reg;
    current &= ~kUsartChrlMask;
    current |= (kUsartChrlEightBits << 6u);
    reg = current;
}

extern "C" __attribute__((noinline, used)) void runtime_set_same70_usart0_chrl() {
    constexpr auto chrl_field = rt::field_ref<FieldId::field_usart0_us_mr_spi_mode_chrl>();
    static_cast<void>(rt::modify_field(chrl_field, kUsartChrlEightBits));
}

extern "C" __attribute__((noinline, used)) void manual_set_same70_tc0_period() {
    *reinterpret_cast<volatile std::uint32_t*>(kTc0RcAddress) = 0x1234u;
}

extern "C" __attribute__((noinline, used)) void runtime_set_same70_tc0_period() {
    static_cast<void>(
        alloy::hal::timer::open<alloy::device::runtime::PeripheralId::TC0>().set_period(0x1234u));
}

extern "C" __attribute__((noinline, used)) void manual_set_same70_pwm0_period() {
    *reinterpret_cast<volatile std::uint32_t*>(kPwm0Cprd0Address) = 0x0200u;
}

extern "C" __attribute__((noinline, used)) void runtime_set_same70_pwm0_period() {
    static_cast<void>(
        alloy::hal::pwm::open<alloy::device::runtime::PeripheralId::PWM0, 0u>().set_period(0x0200u));
}

extern "C" __attribute__((noinline, used)) void manual_set_same70_pwm0_duty() {
    *reinterpret_cast<volatile std::uint32_t*>(kPwm0Cdty0Address) = 0x0080u;
}

extern "C" __attribute__((noinline, used)) void runtime_set_same70_pwm0_duty() {
    static_cast<void>(alloy::hal::pwm::open<alloy::device::runtime::PeripheralId::PWM0, 0u>()
                          .set_duty_cycle(0x0080u));
}
