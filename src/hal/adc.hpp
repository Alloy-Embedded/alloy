#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::adc {

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
using PeripheralId = device::runtime::PeripheralId;

template <PeripheralId Peripheral>
class handle {
  public:
    using semantic_traits = device::runtime::AdcSemanticTraits<Peripheral>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    [[nodiscard]] auto enable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 1u);
        }
        return core::Ok();
    }

    [[nodiscard]] auto start() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kStartField.valid) {
            return detail::runtime::modify_field(semantic_traits::kStartField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto ready() const -> bool {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kReadyField.valid) {
            const auto state = detail::runtime::read_field(semantic_traits::kReadyField);
            return state.is_ok() && state.unwrap() != 0u;
        }
        return false;
    }

    [[nodiscard]] auto read() const -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kDataField.valid) {
            return detail::runtime::read_field(semantic_traits::kDataField);
        }
        if constexpr (semantic_traits::kDataRegister.valid) {
            return detail::runtime::read_register(semantic_traits::kDataRegister);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open() -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested ADC is not published for the selected device.");
    return {};
}
#endif

}  // namespace alloy::hal::adc
