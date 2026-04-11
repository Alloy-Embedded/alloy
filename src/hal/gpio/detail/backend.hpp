#pragma once

#include <cstdint>
#include <string_view>

#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/descriptors.hpp"
#include "device/traits.hpp"

namespace alloy::hal::gpio::detail {

[[nodiscard]] constexpr auto backend_as_string(const char* text) -> std::string_view {
    return text == nullptr ? std::string_view{} : std::string_view{text};
}

[[nodiscard]] constexpr auto backend_strings_equal(const char* lhs, std::string_view rhs) -> bool {
    return backend_as_string(lhs) == rhs;
}

[[nodiscard]] inline auto reg32(std::uintptr_t address) -> volatile std::uint32_t& {
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

[[nodiscard]] consteval auto find_mmio_base(std::string_view peripheral_name) -> std::uintptr_t {
    if constexpr (!device::SelectedDescriptors::available) {
        return 0u;
    }

    for (const auto& descriptor : device::descriptors::tables::peripheral_bases) {
        if (backend_strings_equal(descriptor.name, peripheral_name)) {
            return descriptor.address;
        }
    }

    return 0u;
}

[[nodiscard]] constexpr auto gpio_port_index(std::string_view peripheral_name) -> int {
    if (!peripheral_name.starts_with("GPIO") || peripheral_name.size() < 5) {
        return -1;
    }

    const char suffix = peripheral_name[4];
    if (suffix < 'A' || suffix > 'Z') {
        return -1;
    }

    return suffix - 'A';
}

[[nodiscard]] constexpr auto parse_pid(std::string_view signal) -> int {
    const auto marker = signal.find("PID");
    if (marker == std::string_view::npos) {
        return -1;
    }

    int value = 0;
    bool found_digit = false;
    for (std::size_t index = marker + 3; index < signal.size(); ++index) {
        const char ch = signal[index];
        if (ch < '0' || ch > '9') {
            break;
        }
        found_digit = true;
        value = (value * 10) + (ch - '0');
    }

    return found_digit ? value : -1;
}

template <typename PinHandle>
[[nodiscard]] consteval auto is_stm32g0_family() -> bool {
    return device::SelectedDeviceTraits::vendor == std::string_view{"st"} &&
           device::SelectedDeviceTraits::family == std::string_view{"stm32g0"};
}

template <typename PinHandle>
[[nodiscard]] consteval auto is_stm32f4_family() -> bool {
    return device::SelectedDeviceTraits::vendor == std::string_view{"st"} &&
           device::SelectedDeviceTraits::family == std::string_view{"stm32f4"};
}

template <typename PinHandle>
[[nodiscard]] consteval auto is_same70_family() -> bool {
    return device::SelectedDeviceTraits::vendor == std::string_view{"microchip"} &&
           device::SelectedDeviceTraits::family == std::string_view{"same70"};
}

template <typename PinHandle>
[[nodiscard]] auto enable_gpio_clock() -> core::Result<void, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if constexpr (is_stm32g0_family<PinHandle>()) {
        constexpr auto rcc_base = find_mmio_base("RCC");
        constexpr auto port_index = gpio_port_index(PinHandle::peripheral_name);
        static_assert(rcc_base != 0u, "STM32G0 runtime requires RCC base address.");
        static_assert(port_index >= 0, "GPIO port name must map to a valid STM32G0 RCC bit.");

        constexpr std::uint32_t bit_mask = 1u << port_index;
        reg32(rcc_base + 0x34u) |= bit_mask;   // IOPENR
        reg32(rcc_base + 0x24u) &= ~bit_mask;  // IOPRSTR
        return core::Ok();
    }

    if constexpr (is_stm32f4_family<PinHandle>()) {
        constexpr auto rcc_base = find_mmio_base("RCC");
        constexpr auto port_index = gpio_port_index(PinHandle::peripheral_name);
        static_assert(rcc_base != 0u, "STM32F4 runtime requires RCC base address.");
        static_assert(port_index >= 0, "GPIO port name must map to a valid STM32F4 RCC bit.");

        constexpr std::uint32_t bit_mask = 1u << port_index;
        reg32(rcc_base + 0x30u) |= bit_mask;   // AHB1ENR
        reg32(rcc_base + 0x10u) &= ~bit_mask;  // AHB1RSTR
        return core::Ok();
    }

    if constexpr (is_same70_family<PinHandle>()) {
        constexpr auto pmc_base = find_mmio_base("PMC");
        constexpr auto pid =
            PinHandle::rcc_descriptor == nullptr
                ? -1
                : parse_pid(backend_as_string(PinHandle::rcc_descriptor->enable_signal));
        static_assert(pmc_base != 0u, "SAME70 runtime requires PMC base address.");
        static_assert(pid >= 0, "SAME70 runtime requires a valid PMC peripheral identifier.");

        if constexpr (pid < 32) {
            reg32(pmc_base + 0x10u) = (1u << pid);  // PCER0
        } else {
            reg32(pmc_base + 0x100u) = (1u << (pid - 32));  // PCER1
        }

        return core::Ok();
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PinHandle>
auto configure_gpio(const GpioConfig& config) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto clock_result = enable_gpio_clock<PinHandle>();
    if (clock_result.is_err()) {
        return clock_result;
    }

    constexpr auto line = static_cast<std::uint32_t>(PinHandle::line_index);
    constexpr auto mask = (1u << line);
    const auto base = PinHandle::base_address();

    if constexpr (is_stm32g0_family<PinHandle>() || is_stm32f4_family<PinHandle>()) {
        auto& moder = reg32(base + 0x00u);
        auto& otyper = reg32(base + 0x04u);
        auto& pupdr = reg32(base + 0x0Cu);
        const auto shift = line * 2u;

        otyper = config.drive == PinDrive::OpenDrain ? (otyper | mask) : (otyper & ~mask);

        pupdr &= ~(0x3u << shift);
        switch (config.pull) {
            case PinPull::PullUp:
                pupdr |= (0x1u << shift);
                break;
            case PinPull::PullDown:
                pupdr |= (0x2u << shift);
                break;
            case PinPull::None:
            default:
                break;
        }

        if (config.direction == PinDirection::Output) {
            reg32(base + 0x18u) =
                config.initial_state == PinState::High ? mask : (mask << 16u);  // BSRR
            moder = (moder & ~(0x3u << shift)) | (0x1u << shift);
        } else {
            moder &= ~(0x3u << shift);
        }

        return core::Ok();
    }

    if constexpr (is_same70_family<PinHandle>()) {
        reg32(base + 0x00u) = mask;  // PER

        if (config.drive == PinDrive::OpenDrain) {
            reg32(base + 0x50u) = mask;  // MDER
        } else {
            reg32(base + 0x54u) = mask;  // MDDR
        }

        switch (config.pull) {
            case PinPull::PullUp:
                reg32(base + 0x64u) = mask;  // PUER
                reg32(base + 0x90u) = mask;  // PPDDR
                break;
            case PinPull::PullDown:
                reg32(base + 0x60u) = mask;  // PUDR
                reg32(base + 0x94u) = mask;  // PPDER
                break;
            case PinPull::None:
            default:
                reg32(base + 0x60u) = mask;  // PUDR
                reg32(base + 0x90u) = mask;  // PPDDR
                break;
        }

        if (config.direction == PinDirection::Output) {
            reg32(base + 0x30u) = config.initial_state == PinState::High ? mask : 0u;  // SODR
            reg32(base + 0x34u) = config.initial_state == PinState::Low ? mask : 0u;   // CODR
            reg32(base + 0x10u) = mask;                                                // OER
        } else {
            reg32(base + 0x14u) = mask;  // ODR
        }

        return core::Ok();
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PinHandle>
auto write_gpio(const GpioConfig& config, PinState state) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (config.direction != PinDirection::Output) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr auto line = static_cast<std::uint32_t>(PinHandle::line_index);
    constexpr auto mask = (1u << line);
    const auto base = PinHandle::base_address();

    if constexpr (is_stm32g0_family<PinHandle>() || is_stm32f4_family<PinHandle>()) {
        reg32(base + 0x18u) = state == PinState::High ? mask : (mask << 16u);
        return core::Ok();
    }

    if constexpr (is_same70_family<PinHandle>()) {
        reg32(base + 0x30u) = state == PinState::High ? mask : 0u;
        reg32(base + 0x34u) = state == PinState::Low ? mask : 0u;
        return core::Ok();
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PinHandle>
auto toggle_gpio(const GpioConfig& config) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (config.direction != PinDirection::Output) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr auto line = static_cast<std::uint32_t>(PinHandle::line_index);
    constexpr auto mask = (1u << line);
    const auto base = PinHandle::base_address();

    if constexpr (is_stm32g0_family<PinHandle>() || is_stm32f4_family<PinHandle>()) {
        const bool is_high = (reg32(base + 0x14u) & mask) != 0u;  // ODR
        return write_gpio<PinHandle>(config, is_high ? PinState::Low : PinState::High);
    }

    if constexpr (is_same70_family<PinHandle>()) {
        const bool is_high = (reg32(base + 0x38u) & mask) != 0u;  // ODSR
        return write_gpio<PinHandle>(config, is_high ? PinState::Low : PinState::High);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PinHandle>
auto read_gpio() -> core::Result<PinState, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr auto line = static_cast<std::uint32_t>(PinHandle::line_index);
    constexpr auto mask = (1u << line);
    const auto base = PinHandle::base_address();

    if constexpr (is_stm32g0_family<PinHandle>() || is_stm32f4_family<PinHandle>()) {
        const bool high = (reg32(base + 0x10u) & mask) != 0u;  // IDR
        return core::Ok(high ? PinState::High : PinState::Low);
    }

    if constexpr (is_same70_family<PinHandle>()) {
        const bool high = (reg32(base + 0x3Cu) & mask) != 0u;  // PDSR
        return core::Ok(high ? PinState::High : PinState::Low);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

}  // namespace alloy::hal::gpio::detail
