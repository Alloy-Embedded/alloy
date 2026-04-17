#pragma once

#include <span>

#include "device/selected.hpp"

namespace alloy::device {

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

}  // namespace runtime
#endif

struct SelectedRuntimeDescriptors {
    static constexpr bool available = selected::runtime_available;
};

}  // namespace alloy::device
