#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "device/selected.hpp"

namespace alloy::device {

namespace detail {

template <auto Value>
[[nodiscard]] consteval auto enum_name() -> std::string_view {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view function = __PRETTY_FUNCTION__;
    constexpr std::string_view needle = "Value = ";
#elif defined(_MSC_VER)
    constexpr std::string_view function = __FUNCSIG__;
    constexpr std::string_view needle = "enum_name<";
#else
    #error Unsupported compiler for compile-time enum name extraction.
#endif

    const auto start = function.find(needle);
    static_assert(start != std::string_view::npos,
                  "Unable to parse compiler function signature for enum name extraction.");

    auto value_start = start + needle.size();
#if defined(_MSC_VER)
    const auto value_end = function.find(">(void)", value_start);
#else
    auto value_end = function.find(';', value_start);
    const auto bracket_end = function.find(']', value_start);
    if (value_end == std::string_view::npos ||
        (bracket_end != std::string_view::npos && bracket_end < value_end)) {
        value_end = bracket_end;
    }
#endif

    auto value = function.substr(value_start, value_end - value_start);
    if (const auto scope = value.rfind("::"); scope != std::string_view::npos) {
        value.remove_prefix(scope + 2u);
    }
    return value;
}

[[nodiscard]] constexpr auto trim_prefix(std::string_view text, std::string_view prefix)
    -> std::string_view {
    return text.starts_with(prefix) ? text.substr(prefix.size()) : text;
}

}  // namespace detail

#if ALLOY_DEVICE_RUNTIME_AVAILABLE
namespace runtime {

namespace family = selected::runtime_family_contract;
namespace device_contract = selected::runtime_device_contract;

using BackendSchemaId = family::BackendSchemaId;
using PeripheralClassId = family::PeripheralClassId;
using SignalId = family::SignalId;
using PortId = family::PortId;
using AccessKindId = family::AccessKindId;
using RouteKindId = family::RouteKindId;
using OperationKindId = family::OperationKindId;
using OperationSubjectKindId = family::OperationSubjectKindId;
using ActiveLevelId = family::ActiveLevelId;

using PeripheralId = device_contract::PeripheralId;
using ClockGateId = device_contract::ClockGateId;
using ResetId = device_contract::ResetId;
using ClockSelectorId = device_contract::ClockSelectorId;
using PinId = device_contract::PinId;
using RegisterId = device_contract::RegisterId;
using FieldId = device_contract::FieldId;
using RouteId = device_contract::RouteId;
using RouteOperation = device_contract::RouteOperation;
using RouteDescriptor = device_contract::RouteDescriptor;
using RuntimeRegisterRef = selected::runtime_driver_contract::RuntimeRegisterRef;
using RuntimeFieldRef = selected::runtime_driver_contract::RuntimeFieldRef;
using RuntimeIndexedFieldRef = selected::runtime_driver_contract::RuntimeIndexedFieldRef;
using KernelClockSource = selected::runtime_driver_contract::KernelClockSource;
using KernelClockSourceOption = selected::runtime_driver_contract::KernelClockSourceOption;
inline constexpr auto invalid_register_ref = selected::runtime_driver_contract::kInvalidRegisterRef;
inline constexpr auto invalid_field_ref = selected::runtime_driver_contract::kInvalidFieldRef;
inline constexpr auto invalid_indexed_field_ref =
    selected::runtime_driver_contract::kInvalidIndexedFieldRef;

inline constexpr auto peripherals = std::span{device_contract::kRuntimePeripherals};
inline constexpr auto pins = std::span{device_contract::kPins};
inline constexpr auto registers = std::span{device_contract::kRegisters};
inline constexpr auto fields = std::span{device_contract::kRegisterFields};
inline constexpr const auto& runtime_peripheral_ids = device_contract::kRuntimePeripherals;
inline constexpr const auto& runtime_pin_ids = device_contract::kPins;
inline constexpr const auto& runtime_register_ids = device_contract::kRegisters;
inline constexpr const auto& runtime_field_ids = device_contract::kRegisterFields;
inline constexpr auto gpio_semantic_pins =
    std::span{selected::runtime_driver_contract::kGpioSemanticPins};
inline constexpr auto uart_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kUartSemanticPeripherals};
inline constexpr auto i2c_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kI2cSemanticPeripherals};
inline constexpr auto spi_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kSpiSemanticPeripherals};
    #if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
inline constexpr auto dma_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kDmaSemanticPeripherals};
    #endif
#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
inline constexpr auto adc_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kAdcSemanticPeripherals};
#endif
#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
inline constexpr auto dac_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kDacSemanticPeripherals};
#endif
#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
inline constexpr auto can_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kCanSemanticPeripherals};
#endif
#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
inline constexpr auto rtc_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kRtcSemanticPeripherals};
#endif
#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
inline constexpr auto watchdog_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kWatchdogSemanticPeripherals};
#endif
#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
inline constexpr auto timer_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kTimerSemanticPeripherals};
#endif
#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
inline constexpr auto pwm_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kPwmSemanticPeripherals};
#endif
inline constexpr const auto& gpio_semantic_pin_ids =
    selected::runtime_driver_contract::kGpioSemanticPins;
inline constexpr const auto& uart_semantic_peripheral_ids =
    selected::runtime_driver_contract::kUartSemanticPeripherals;
inline constexpr const auto& i2c_semantic_peripheral_ids =
    selected::runtime_driver_contract::kI2cSemanticPeripherals;
inline constexpr const auto& spi_semantic_peripheral_ids =
    selected::runtime_driver_contract::kSpiSemanticPeripherals;
    #if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
inline constexpr const auto& dma_semantic_peripheral_ids =
    selected::runtime_driver_contract::kDmaSemanticPeripherals;
    #endif
#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
inline constexpr const auto& adc_semantic_peripheral_ids =
    selected::runtime_driver_contract::kAdcSemanticPeripherals;
#endif
#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
inline constexpr const auto& dac_semantic_peripheral_ids =
    selected::runtime_driver_contract::kDacSemanticPeripherals;
#endif
#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
inline constexpr const auto& can_semantic_peripheral_ids =
    selected::runtime_driver_contract::kCanSemanticPeripherals;
#endif
#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
inline constexpr const auto& rtc_semantic_peripheral_ids =
    selected::runtime_driver_contract::kRtcSemanticPeripherals;
#endif
#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
inline constexpr const auto& watchdog_semantic_peripheral_ids =
    selected::runtime_driver_contract::kWatchdogSemanticPeripherals;
#endif
#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
inline constexpr const auto& timer_semantic_peripheral_ids =
    selected::runtime_driver_contract::kTimerSemanticPeripherals;
#endif
#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
inline constexpr const auto& pwm_semantic_peripheral_ids =
    selected::runtime_driver_contract::kPwmSemanticPeripherals;
#endif

template <PeripheralId Id>
using PeripheralInstanceTraits = device_contract::PeripheralInstanceTraits<Id>;

template <PinId Id>
using PinTraits = device_contract::PinTraits<Id>;

template <RegisterId Id>
using RegisterTraits = device_contract::RegisterTraits<Id>;

template <FieldId Id>
using RegisterFieldTraits = device_contract::RegisterFieldTraits<Id>;

template <ClockGateId Id>
using ClockGateTraits = device_contract::ClockGateTraits<Id>;

template <ResetId Id>
using ResetTraits = device_contract::ResetTraits<Id>;

template <ClockSelectorId Id>
using ClockSelectorTraits = device_contract::ClockSelectorTraits<Id>;

template <PeripheralId Id>
using PeripheralClockBindingTraits = device_contract::PeripheralClockBindingTraits<Id>;

template <PinId Pin, PeripheralId Peripheral, SignalId Signal>
using RouteTraits = device_contract::RouteTraits<Pin, Peripheral, Signal>;

template <PeripheralId Peripheral, SignalId... Signals>
using ConnectionGroupTraits = device_contract::ConnectionGroupTraits<Peripheral, Signals...>;

template <PinId Id>
using GpioSemanticTraits = selected::runtime_driver_contract::GpioSemanticTraits<Id>;

template <PeripheralId Id>
using UartSemanticTraits = selected::runtime_driver_contract::UartSemanticTraits<Id>;

template <PeripheralId Id>
using I2cSemanticTraits = selected::runtime_driver_contract::I2cSemanticTraits<Id>;

template <PeripheralId Id>
using SpiSemanticTraits = selected::runtime_driver_contract::SpiSemanticTraits<Id>;

    #if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
template <PeripheralId Peripheral, SignalId Signal>
using DmaSemanticTraits = selected::runtime_driver_contract::DmaSemanticTraits<Peripheral, Signal>;
    #endif

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using AdcSemanticTraits = selected::runtime_driver_contract::AdcSemanticTraits<Id>;
template <PeripheralId Id>
using AdcChannelOf = selected::runtime_driver_contract::AdcChannelOf<Id>;
template <PeripheralId Id>
using AdcChannel = selected::runtime_driver_contract::AdcChannel<Id>;
#endif

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using DacSemanticTraits = selected::runtime_driver_contract::DacSemanticTraits<Id>;

template <PeripheralId Id, std::size_t Channel>
using DacChannelSemanticTraits =
    selected::runtime_driver_contract::DacChannelSemanticTraits<Id, Channel>;
#endif

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using CanSemanticTraits = selected::runtime_driver_contract::CanSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using RtcSemanticTraits = selected::runtime_driver_contract::RtcSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using WatchdogSemanticTraits = selected::runtime_driver_contract::WatchdogSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using TimerSemanticTraits = selected::runtime_driver_contract::TimerSemanticTraits<Id>;

template <PeripheralId Id, std::size_t Channel>
using TimerChannelSemanticTraits =
    selected::runtime_driver_contract::TimerChannelSemanticTraits<Id, Channel>;
#endif

#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using PwmSemanticTraits = selected::runtime_driver_contract::PwmSemanticTraits<Id>;

template <PeripheralId Id, std::size_t Channel>
using PwmChannelSemanticTraits =
    selected::runtime_driver_contract::PwmChannelSemanticTraits<Id, Channel>;
#endif

#if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using QspiSemanticTraits = selected::runtime_driver_contract::QspiSemanticTraits<Id>;
inline constexpr auto qspi_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kQspiSemanticPeripherals};
inline constexpr const auto& qspi_semantic_peripheral_ids =
    selected::runtime_driver_contract::kQspiSemanticPeripherals;
#endif

#if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using SdmmcSemanticTraits = selected::runtime_driver_contract::SdmmcSemanticTraits<Id>;
inline constexpr auto sdmmc_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kSdmmcSemanticPeripherals};
inline constexpr const auto& sdmmc_semantic_peripheral_ids =
    selected::runtime_driver_contract::kSdmmcSemanticPeripherals;
#endif

#if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using EthSemanticTraits = selected::runtime_driver_contract::EthSemanticTraits<Id>;
inline constexpr auto eth_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kEthSemanticPeripherals};
inline constexpr const auto& eth_semantic_peripheral_ids =
    selected::runtime_driver_contract::kEthSemanticPeripherals;
#endif

#if ALLOY_DEVICE_USB_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using UsbSemanticTraits = selected::runtime_driver_contract::UsbSemanticTraits<Id>;
inline constexpr auto usb_semantic_peripherals =
    std::span{selected::runtime_driver_contract::kUsbSemanticPeripherals};
inline constexpr const auto& usb_semantic_peripheral_ids =
    selected::runtime_driver_contract::kUsbSemanticPeripherals;
#endif

}  // namespace runtime

using BackendSchemaId = runtime::BackendSchemaId;
using PeripheralClassId = runtime::PeripheralClassId;
using SignalId = runtime::SignalId;
using PortId = runtime::PortId;
using AccessKindId = runtime::AccessKindId;
using RouteKindId = runtime::RouteKindId;
using OperationKindId = runtime::OperationKindId;
using OperationSubjectKindId = runtime::OperationSubjectKindId;
using ActiveLevelId = runtime::ActiveLevelId;

using PeripheralId = runtime::PeripheralId;
using ClockGateId = runtime::ClockGateId;
using ResetId = runtime::ResetId;
using ClockSelectorId = runtime::ClockSelectorId;
using PinId = runtime::PinId;
using RegisterId = runtime::RegisterId;
using FieldId = runtime::FieldId;
using RouteId = runtime::RouteId;
using RouteOperation = runtime::RouteOperation;
using RouteDescriptor = runtime::RouteDescriptor;
using RuntimeRegisterRef = runtime::RuntimeRegisterRef;
using RuntimeFieldRef = runtime::RuntimeFieldRef;
using RuntimeIndexedFieldRef = runtime::RuntimeIndexedFieldRef;
using KernelClockSource = runtime::KernelClockSource;
using KernelClockSourceOption = runtime::KernelClockSourceOption;

inline constexpr auto invalid_register_ref = runtime::invalid_register_ref;
inline constexpr auto invalid_field_ref = runtime::invalid_field_ref;
inline constexpr auto invalid_indexed_field_ref = runtime::invalid_indexed_field_ref;

inline constexpr auto peripherals = runtime::peripherals;
inline constexpr auto pins = runtime::pins;
inline constexpr auto registers = runtime::registers;
inline constexpr auto fields = runtime::fields;

template <PeripheralId Id>
using PeripheralInstanceTraits = runtime::PeripheralInstanceTraits<Id>;

template <PinId Id>
using PinTraits = runtime::PinTraits<Id>;

template <RegisterId Id>
using RegisterTraits = runtime::RegisterTraits<Id>;

template <FieldId Id>
using RegisterFieldTraits = runtime::RegisterFieldTraits<Id>;

template <ClockGateId Id>
using ClockGateTraits = runtime::ClockGateTraits<Id>;

template <ResetId Id>
using ResetTraits = runtime::ResetTraits<Id>;

template <ClockSelectorId Id>
using ClockSelectorTraits = runtime::ClockSelectorTraits<Id>;

template <PeripheralId Id>
using PeripheralClockBindingTraits = runtime::PeripheralClockBindingTraits<Id>;

template <PinId Pin, PeripheralId Peripheral, SignalId Signal>
using RouteTraits = runtime::RouteTraits<Pin, Peripheral, Signal>;

template <PeripheralId Peripheral, SignalId... Signals>
using ConnectionGroupTraits = runtime::ConnectionGroupTraits<Peripheral, Signals...>;

template <PinId Id>
using GpioSemanticTraits = runtime::GpioSemanticTraits<Id>;

template <PeripheralId Id>
using UartSemanticTraits = runtime::UartSemanticTraits<Id>;

template <PeripheralId Id>
using I2cSemanticTraits = runtime::I2cSemanticTraits<Id>;

template <PeripheralId Id>
using SpiSemanticTraits = runtime::SpiSemanticTraits<Id>;

    #if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
template <PeripheralId Peripheral, SignalId Signal>
using DmaSemanticTraits = runtime::DmaSemanticTraits<Peripheral, Signal>;
    #endif

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using AdcSemanticTraits = runtime::AdcSemanticTraits<Id>;
template <PeripheralId Id>
using AdcChannelOf = runtime::AdcChannelOf<Id>;
template <PeripheralId Id>
using AdcChannel = runtime::AdcChannel<Id>;
#endif

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using DacSemanticTraits = runtime::DacSemanticTraits<Id>;

template <PeripheralId Id, std::size_t Channel>
using DacChannelSemanticTraits = runtime::DacChannelSemanticTraits<Id, Channel>;
#endif

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using CanSemanticTraits = runtime::CanSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using RtcSemanticTraits = runtime::RtcSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using WatchdogSemanticTraits = runtime::WatchdogSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using TimerSemanticTraits = runtime::TimerSemanticTraits<Id>;

template <PeripheralId Id, std::size_t Channel>
using TimerChannelSemanticTraits = runtime::TimerChannelSemanticTraits<Id, Channel>;
#endif

#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using PwmSemanticTraits = runtime::PwmSemanticTraits<Id>;

template <PeripheralId Id, std::size_t Channel>
using PwmChannelSemanticTraits = runtime::PwmChannelSemanticTraits<Id, Channel>;
#endif

#if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using QspiSemanticTraits = runtime::QspiSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using SdmmcSemanticTraits = runtime::SdmmcSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using EthSemanticTraits = runtime::EthSemanticTraits<Id>;
#endif

#if ALLOY_DEVICE_USB_SEMANTICS_AVAILABLE
template <PeripheralId Id>
using UsbSemanticTraits = runtime::UsbSemanticTraits<Id>;
#endif

template <PinId Id>
struct pin {
    static constexpr auto id = Id;
    static constexpr auto name = detail::enum_name<Id>();
};

template <PeripheralId Id>
struct peripheral {
    static constexpr auto id = Id;
    static constexpr auto name = detail::enum_name<Id>();
};

template <SignalId Id>
struct signal {
    static constexpr auto id = Id;
    static constexpr auto name = detail::trim_prefix(detail::enum_name<Id>(), "signal_");
};
#endif

struct SelectedRuntimeDescriptors {
    static constexpr bool available = selected::runtime_available;
};

#if ALLOY_DEVICE_RUNTIME_AVAILABLE
template <PeripheralId Id>
[[nodiscard]] constexpr auto base() noexcept -> std::uintptr_t {
    return selected::runtime_device_contract::peripheral_base<Id>();
}
#endif

}  // namespace alloy::device

#if ALLOY_DEVICE_RUNTIME_AVAILABLE
namespace alloy::clock {

template <alloy::device::PeripheralId Id>
inline auto enable() noexcept -> void {
    alloy::device::selected::runtime_device_contract::clock_enable<Id>();
}

template <alloy::device::PeripheralId Id>
inline auto disable() noexcept -> void {
    alloy::device::selected::runtime_device_contract::clock_disable<Id>();
}

}  // namespace alloy::clock

namespace alloy::pinmux {

template <alloy::device::PinId Pin,
          alloy::device::PeripheralId Peripheral,
          alloy::device::SignalId Signal>
inline auto route() noexcept -> void {
    alloy::device::selected::runtime_device_contract::apply_route<Pin, Peripheral, Signal>();
}

}  // namespace alloy::pinmux
#endif

#include "device/dev.hpp"
