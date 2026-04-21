#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/watchdog.hpp"

namespace rt = alloy::hal::detail::runtime;

namespace {

using FieldId = alloy::device::runtime::FieldId;

constexpr auto kWdtMrAddress = std::uintptr_t{0x400E1854u};
constexpr auto kGpiocSodrAddress = std::uintptr_t{0x400E1230u};
constexpr auto kUsart0UsMrAddress = std::uintptr_t{0x40024004u};
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
