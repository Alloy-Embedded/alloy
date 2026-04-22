#pragma once

#include <cstddef>
#include <cstdint>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::can {

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

struct nominal_timing_config {
    std::uint32_t prescaler = 1u;
    std::uint32_t time_seg1 = 1u;
    std::uint32_t time_seg2 = 1u;
    std::uint32_t sync_jump_width = 1u;
};

struct Config {
    bool enter_init_mode = true;
    bool enable_configuration = true;
    bool enable_fd_operation = false;
    bool enable_bit_rate_switch = false;
    bool apply_nominal_timing = false;
    nominal_timing_config nominal_timing{};
};

template <PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::CanSemanticTraits<Peripheral>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enter_init_mode) {
            result = enter_init_mode();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.enable_configuration) {
            result = enable_configuration();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.enable_fd_operation) {
            result = enable_fd_operation(true);
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.enable_bit_rate_switch) {
            result = enable_bit_rate_switch(true);
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.apply_nominal_timing) {
            result = set_nominal_timing(config_.nominal_timing);
        }
        return result;
    }

    [[nodiscard]] auto enter_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto leave_init_mode() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kInitField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInitField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto enable_configuration() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kConfigEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kConfigEnableField, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto enable_fd_operation(bool enabled = true) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kFdOperationEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kFdOperationEnableField,
                                                 enabled ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto enable_bit_rate_switch(bool enabled = true) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kBitRateSwitchEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kBitRateSwitchEnableField,
                                                 enabled ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_nominal_timing(const nominal_timing_config& config) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kNominalPrescalerField.valid &&
                      semantic_traits::kNominalTimeSeg1Field.valid &&
                      semantic_traits::kNominalTimeSeg2Field.valid &&
                      semantic_traits::kNominalSyncJumpWidthField.valid) {
            auto result =
                detail::runtime::modify_field(semantic_traits::kNominalPrescalerField,
                                              config.prescaler);
            if (!result.is_ok()) {
                return result;
            }
            result =
                detail::runtime::modify_field(semantic_traits::kNominalTimeSeg1Field,
                                              config.time_seg1);
            if (!result.is_ok()) {
                return result;
            }
            result =
                detail::runtime::modify_field(semantic_traits::kNominalTimeSeg2Field,
                                              config.time_seg2);
            if (!result.is_ok()) {
                return result;
            }
            return detail::runtime::modify_field(semantic_traits::kNominalSyncJumpWidthField,
                                                 config.sync_jump_width);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto enable_rx_fifo0_interrupt() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kRxFifo0NewInterruptEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kRxFifo0NewInterruptEnableField,
                                                 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto rx_fifo0_fill_level() const
        -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kRxFifo0FillLevelField.valid) {
            return detail::runtime::read_field(semantic_traits::kRxFifo0FillLevelField);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto request_tx(std::size_t buffer_index) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested CAN peripheral is not published for the selected device.");

        if constexpr (semantic_traits::kTxBufferAddRequestPattern.valid) {
            return detail::runtime::modify_indexed_field(
                semantic_traits::kTxBufferAddRequestPattern, buffer_index, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested CAN peripheral is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::can
