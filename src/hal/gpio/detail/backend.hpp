#pragma once

#include <cstdint>

#include "hal/detail/runtime_lite_ops.hpp"
#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal::gpio::detail {

namespace rt = alloy::hal::detail::runtime_lite;

[[nodiscard]] constexpr auto pull_value(PinPull pull) -> std::uint32_t {
    switch (pull) {
        case PinPull::PullUp:
            return 0x1u;
        case PinPull::PullDown:
            return 0x2u;
        case PinPull::None:
        default:
            return 0x0u;
    }
}

template <typename PinHandle>
auto configure_st_gpio(const GpioConfig& config) -> core::Result<void, core::ErrorCode> {
    const auto otype_result =
        rt::modify_field(PinHandle::output_type_field,
                         config.drive == PinDrive::OpenDrain ? 1u : 0u);
    if (otype_result.is_err()) {
        return otype_result;
    }

    const auto pull_result = rt::modify_field(PinHandle::pull_field, pull_value(config.pull));
    if (pull_result.is_err()) {
        return pull_result;
    }

    if (config.direction == PinDirection::Output) {
        const auto bsrr_bits = rt::field_bits(
            config.initial_state == PinState::High ? PinHandle::output_set_field
                                                   : PinHandle::output_reset_field,
            1u);
        if (bsrr_bits.is_err()) {
            return core::Err(core::ErrorCode{bsrr_bits.unwrap_err()});
        }

        const auto write_result = rt::write_register(PinHandle::output_set_field.reg, bsrr_bits.unwrap());
        if (write_result.is_err()) {
            return write_result;
        }

        return rt::modify_field(PinHandle::mode_field, 0x1u);
    }

    return rt::modify_field(PinHandle::mode_field, 0x0u);
}

template <typename PinHandle>
auto configure_microchip_pio(const GpioConfig& config) -> core::Result<void, core::ErrorCode> {
    const auto per_bits = rt::field_bits(PinHandle::pio_enable_field, 1u);
    if (per_bits.is_err()) {
        return core::Err(core::ErrorCode{per_bits.unwrap_err()});
    }
    const auto per_result = rt::write_register(PinHandle::pio_enable_field.reg, per_bits.unwrap());
    if (per_result.is_err()) {
        return per_result;
    }

    const auto drive_bits =
        rt::field_bits(config.drive == PinDrive::OpenDrain ? PinHandle::pio_drive_enable_field
                                                           : PinHandle::pio_drive_disable_field,
                       1u);
    if (drive_bits.is_err()) {
        return core::Err(core::ErrorCode{drive_bits.unwrap_err()});
    }
    const auto drive_result = rt::write_register(PinHandle::pio_drive_enable_field.reg, drive_bits.unwrap());
    if (drive_result.is_err()) {
        return drive_result;
    }

    const auto pull_up_bits =
        rt::field_bits(config.pull == PinPull::PullUp ? PinHandle::pio_pull_up_enable_field
                                                      : PinHandle::pio_pull_up_disable_field,
                       1u);
    if (pull_up_bits.is_err()) {
        return core::Err(core::ErrorCode{pull_up_bits.unwrap_err()});
    }
    const auto pull_up_result =
        rt::write_register(PinHandle::pio_pull_up_enable_field.reg, pull_up_bits.unwrap());
    if (pull_up_result.is_err()) {
        return pull_up_result;
    }

    const auto pull_down_bits = rt::field_bits(
        config.pull == PinPull::PullDown ? PinHandle::pio_pull_down_enable_field
                                         : PinHandle::pio_pull_down_disable_field,
        1u);
    if (pull_down_bits.is_err()) {
        return core::Err(core::ErrorCode{pull_down_bits.unwrap_err()});
    }
    const auto pull_down_result =
        rt::write_register(PinHandle::pio_pull_down_enable_field.reg, pull_down_bits.unwrap());
    if (pull_down_result.is_err()) {
        return pull_down_result;
    }

    if (config.direction == PinDirection::Output) {
        const auto state_bits = rt::field_bits(
            config.initial_state == PinState::High ? PinHandle::pio_set_field
                                                   : PinHandle::pio_clear_field,
            1u);
        if (state_bits.is_err()) {
            return core::Err(core::ErrorCode{state_bits.unwrap_err()});
        }
        const auto state_result = rt::write_register(PinHandle::pio_set_field.reg, state_bits.unwrap());
        if (state_result.is_err()) {
            return state_result;
        }

        const auto oer_bits = rt::field_bits(PinHandle::pio_output_enable_field, 1u);
        if (oer_bits.is_err()) {
            return core::Err(core::ErrorCode{oer_bits.unwrap_err()});
        }
        return rt::write_register(PinHandle::pio_output_enable_field.reg, oer_bits.unwrap());
    }

    const auto odr_bits = rt::field_bits(PinHandle::pio_output_disable_field, 1u);
    if (odr_bits.is_err()) {
        return core::Err(core::ErrorCode{odr_bits.unwrap_err()});
    }
    return rt::write_register(PinHandle::pio_output_disable_field.reg, odr_bits.unwrap());
}

template <typename PinHandle>
auto configure_gpio(const GpioConfig& config) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr auto schema = rt::gpio_schema_for<PinHandle>();
    if constexpr (schema == rt::GpioSchema::st_gpio) {
        const auto enable_result = rt::enable_gpio_port_runtime<PinHandle::peripheral_id>();
        if (enable_result.is_err()) {
            return enable_result;
        }
        return configure_st_gpio<PinHandle>(config);
    }
    if constexpr (schema == rt::GpioSchema::microchip_pio_v) {
        const auto clock_result = rt::enable_gpio_port_runtime<PinHandle::peripheral_id>();
        if (clock_result.is_err()) {
            return clock_result;
        }
        return configure_microchip_pio<PinHandle>(config);
    }
    if constexpr (schema == rt::GpioSchema::nxp_imxrt_gpio_v1) {
        const auto direction_result =
            rt::modify_field(PinHandle::direction_field,
                             config.direction == PinDirection::Output ? 1u : 0u);
        if (direction_result.is_err()) {
            return direction_result;
        }

        if (config.direction == PinDirection::Output) {
            return rt::modify_field(PinHandle::output_value_field,
                                    config.initial_state == PinState::High ? 1u : 0u);
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

    constexpr auto schema = rt::gpio_schema_for<PinHandle>();

    if constexpr (schema == rt::GpioSchema::st_gpio) {
        const auto bits = rt::field_bits(
            state == PinState::High ? PinHandle::output_set_field : PinHandle::output_reset_field,
            1u);
        if (bits.is_err()) {
            return core::Err(core::ErrorCode{bits.unwrap_err()});
        }
        return rt::write_register(PinHandle::output_set_field.reg, bits.unwrap());
    }

    if constexpr (schema == rt::GpioSchema::microchip_pio_v) {
        const auto bits = rt::field_bits(
            state == PinState::High ? PinHandle::pio_set_field : PinHandle::pio_clear_field, 1u);
        if (bits.is_err()) {
            return core::Err(core::ErrorCode{bits.unwrap_err()});
        }
        return rt::write_register(PinHandle::pio_set_field.reg, bits.unwrap());
    }

    if constexpr (schema == rt::GpioSchema::nxp_imxrt_gpio_v1) {
        return rt::modify_field(PinHandle::output_value_field,
                                state == PinState::High ? 1u : 0u);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PinHandle>
auto read_gpio() -> core::Result<PinState, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr auto schema = rt::gpio_schema_for<PinHandle>();

    if constexpr (schema == rt::GpioSchema::st_gpio) {
        const auto value = rt::read_field(PinHandle::input_field);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        return core::Ok(value.unwrap() != 0u ? PinState::High : PinState::Low);
    }

    if constexpr (schema == rt::GpioSchema::microchip_pio_v) {
        const auto value = rt::read_field(PinHandle::pio_input_state_field);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        return core::Ok(value.unwrap() != 0u ? PinState::High : PinState::Low);
    }

    if constexpr (schema == rt::GpioSchema::nxp_imxrt_gpio_v1) {
        const auto value = rt::read_field(PinHandle::input_field);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        return core::Ok(value.unwrap() != 0u ? PinState::High : PinState::Low);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PinHandle>
auto toggle_gpio(const GpioConfig& config) -> core::Result<void, core::ErrorCode>;

template <typename PinHandle>
auto toggle_gpio(const GpioConfig& config) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PinHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (config.direction != PinDirection::Output) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto current = read_gpio<PinHandle>();
    if (current.is_err()) {
        return core::Err(core::ErrorCode{current.unwrap_err()});
    }
    return write_gpio<PinHandle>(config,
                                 current.unwrap() == PinState::High ? PinState::Low
                                                                    : PinState::High);
}

}  // namespace alloy::hal::gpio::detail
