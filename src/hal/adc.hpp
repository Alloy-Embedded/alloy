#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::adc {

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
using PeripheralId = device::runtime::PeripheralId;

struct Config {
    bool enable_on_configure = true;
    bool start_immediately = false;
};

template <PeripheralId Peripheral>
class handle {
  public:
    using semantic_traits = device::runtime::AdcSemanticTraits<Peripheral>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enable_on_configure) {
            result = enable();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.start_immediately) {
            result = start();
        }
        return result;
    }

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

    [[nodiscard]] auto enable_dma(bool circular = false) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kDmaEnableField.valid) {
            auto result = detail::runtime::modify_field(semantic_traits::kDmaEnableField, 1u);
            if (!result.is_ok()) {
                return result;
            }
            if constexpr (semantic_traits::kDmaModeField.valid) {
                return detail::runtime::modify_field(semantic_traits::kDmaModeField,
                                                     circular ? 1u : 0u);
            }
            return result;
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto disable_dma() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kDmaEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDmaEnableField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& channel, bool circular = false) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX);

        const auto dma_result = channel.configure();
        if (!dma_result.is_ok()) {
            return dma_result;
        }
        return enable_dma(circular);
    }

    [[nodiscard]] static constexpr auto data_register_address() -> std::uintptr_t {
        if constexpr (semantic_traits::kDataRegister.valid) {
            return semantic_traits::kDataRegister.base_address +
                   semantic_traits::kDataRegister.offset_bytes;
        } else if constexpr (semantic_traits::kDataField.valid) {
            return semantic_traits::kDataField.reg.base_address +
                   semantic_traits::kDataField.reg.offset_bytes;
        } else {
            return 0u;
        }
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested ADC is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::adc
