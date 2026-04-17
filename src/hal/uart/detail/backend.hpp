#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

#include "hal/detail/runtime_ops.hpp"
#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal::uart::detail {

namespace rt = alloy::hal::detail::runtime;

struct FieldWrite {
    rt::FieldRef field{};
    std::uint32_t value = 0u;
};

template <std::size_t N>
[[nodiscard]] auto build_register_value(const std::array<FieldWrite, N>& fields)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    auto value = std::uint32_t{0u};
    for (const auto& field : fields) {
        if (!field.field.valid) {
            continue;
        }
        const auto bits = rt::field_bits(field.field, field.value);
        if (bits.is_err()) {
            return core::Err(core::ErrorCode{bits.unwrap_err()});
        }
        value |= bits.unwrap();
    }
    return core::Ok(static_cast<std::uint32_t>(value));
}

template <typename PortHandle, std::size_t... Index>
[[nodiscard]] constexpr auto has_signal_impl(std::string_view signal_name,
                                             std::index_sequence<Index...>) -> bool {
    using connector_type = typename PortHandle::connector_type;
    return (((connector_type::template binding_type<Index>::signal_type::name) == signal_name) ||
            ...);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto has_signal(std::string_view signal_name) -> bool {
    using connector_type = typename PortHandle::connector_type;
    return has_signal_impl<PortHandle>(signal_name,
                                       std::make_index_sequence<connector_type::binding_count>{});
}

[[nodiscard]] constexpr auto baud_value(Baudrate baudrate) -> std::uint32_t {
    return static_cast<std::uint32_t>(baudrate);
}

[[nodiscard]] inline auto wait_for_field(const rt::FieldRef& field)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        const auto value = rt::read_field(field);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() != 0u) {
            return core::Ok();
        }
    }
    return core::Err(core::ErrorCode::Timeout);
}

template <typename PortHandle>
auto configure_st_uart(const UartConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.peripheral_clock_hz == 0u || baud_value(config.baudrate) == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (config.flow_control != FlowControl::None) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    if (!has_signal<PortHandle>("tx") && !has_signal<PortHandle>("rx")) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    auto m0_value = std::uint32_t{0u};
    auto m1_value = std::uint32_t{0u};
    switch (config.data_bits) {
        case DataBits::Seven:
            if (!PortHandle::m1_field.valid) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            m1_value = 1u;
            break;
        case DataBits::Eight:
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    const auto enable_value = build_register_value(std::array{
        FieldWrite{PortHandle::ue_field, 0u},
        FieldWrite{PortHandle::re_field, has_signal<PortHandle>("rx") ? 1u : 0u},
        FieldWrite{PortHandle::te_field, has_signal<PortHandle>("tx") ? 1u : 0u},
        FieldWrite{PortHandle::pce_field, config.parity == Parity::None ? 0u : 1u},
        FieldWrite{PortHandle::ps_field, config.parity == Parity::Odd ? 1u : 0u},
        FieldWrite{PortHandle::m1_field.valid ? PortHandle::m0_field : PortHandle::m_field,
                   m0_value},
        FieldWrite{PortHandle::m1_field, m1_value},
    });
    if (enable_value.is_err()) {
        return core::Err(core::ErrorCode{enable_value.unwrap_err()});
    }

    if (const auto cr1_result = rt::write_register(PortHandle::cr1_reg, enable_value.unwrap());
        cr1_result.is_err()) {
        return cr1_result;
    }

    auto stop_value = std::uint32_t{0u};
    switch (config.stop_bits) {
        case StopBits::One:
            stop_value = 0u;
            break;
        case StopBits::Two:
            stop_value = 0x2u;
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    if (const auto stop_result = rt::modify_field(PortHandle::stop_field, stop_value);
        stop_result.is_err()) {
        return stop_result;
    }

    const auto divisor = (config.peripheral_clock_hz + (baud_value(config.baudrate) / 2u)) /
                         baud_value(config.baudrate);
    if (const auto brr_result = rt::write_register(PortHandle::brr_reg, divisor);
        brr_result.is_err()) {
        return brr_result;
    }

    return rt::modify_field(PortHandle::ue_field, 1u);
}

template <typename PortHandle>
auto configure_microchip_uart_r(const UartConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.peripheral_clock_hz == 0u || baud_value(config.baudrate) == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (config.flow_control != FlowControl::None || config.data_bits != DataBits::Eight ||
        config.stop_bits != StopBits::One) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    if (!has_signal<PortHandle>("tx") && !has_signal<PortHandle>("rx")) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto reset_mask = build_register_value(std::array{
        FieldWrite{PortHandle::rstrx_field, 1u},
        FieldWrite{PortHandle::rsttx_field, 1u},
        FieldWrite{PortHandle::rxdis_field, 1u},
        FieldWrite{PortHandle::txdis_field, 1u},
        FieldWrite{PortHandle::rststa_field, 1u},
    });
    if (reset_mask.is_err()) {
        return core::Err(core::ErrorCode{reset_mask.unwrap_err()});
    }
    if (const auto reset_result = rt::write_register(PortHandle::cr_reg, reset_mask.unwrap());
        reset_result.is_err()) {
        return reset_result;
    }

    std::uint32_t parity = 0u;
    switch (config.parity) {
        case Parity::Even:
            parity = 0u;
            break;
        case Parity::Odd:
            parity = 1u;
            break;
        case Parity::None:
            parity = 4u;
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    const auto mr_value = build_register_value(std::array{
        FieldWrite{PortHandle::par_field, parity},
        FieldWrite{PortHandle::chmode_field, 0u},
    });
    if (mr_value.is_err()) {
        return core::Err(core::ErrorCode{mr_value.unwrap_err()});
    }
    if (const auto mr_result = rt::write_register(PortHandle::mr_reg, mr_value.unwrap());
        mr_result.is_err()) {
        return mr_result;
    }

    auto cd = (config.peripheral_clock_hz + ((16u * baud_value(config.baudrate)) / 2u)) /
              (16u * baud_value(config.baudrate));
    if (cd == 0u) {
        cd = 1u;
    }

    const auto brgr_value = build_register_value(std::array{
        FieldWrite{PortHandle::cd_field, cd},
    });
    if (brgr_value.is_err()) {
        return core::Err(core::ErrorCode{brgr_value.unwrap_err()});
    }
    if (const auto brgr_result = rt::write_register(PortHandle::brgr_reg, brgr_value.unwrap());
        brgr_result.is_err()) {
        return brgr_result;
    }

    const auto enable_mask = build_register_value(std::array{
        FieldWrite{PortHandle::rxen_field, has_signal<PortHandle>("rx") ? 1u : 0u},
        FieldWrite{PortHandle::txen_field, has_signal<PortHandle>("tx") ? 1u : 0u},
    });
    if (enable_mask.is_err()) {
        return core::Err(core::ErrorCode{enable_mask.unwrap_err()});
    }

    return rt::write_register(PortHandle::cr_reg, enable_mask.unwrap());
}

template <typename PortHandle>
auto configure_microchip_usart_zw(const UartConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.peripheral_clock_hz == 0u || baud_value(config.baudrate) == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (config.flow_control != FlowControl::None || config.data_bits != DataBits::Eight ||
        config.stop_bits != StopBits::One || config.parity != Parity::None) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    if (!has_signal<PortHandle>("tx") && !has_signal<PortHandle>("rx")) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto reset_mask = build_register_value(std::array{
        FieldWrite{PortHandle::us_rstrx_field, 1u},
        FieldWrite{PortHandle::us_rsttx_field, 1u},
        FieldWrite{PortHandle::us_rxdis_field, 1u},
        FieldWrite{PortHandle::us_txdis_field, 1u},
        FieldWrite{PortHandle::us_rststa_field, 1u},
    });
    if (reset_mask.is_err()) {
        return core::Err(core::ErrorCode{reset_mask.unwrap_err()});
    }
    if (const auto reset_result = rt::write_register(PortHandle::us_cr_reg, reset_mask.unwrap());
        reset_result.is_err()) {
        return reset_result;
    }

    const auto mr_value = build_register_value(std::array{
        FieldWrite{PortHandle::us_usart_mode_field, 0u},
        FieldWrite{PortHandle::us_usclks_field, 0u},
        FieldWrite{PortHandle::us_chrl_field, 3u},
    });
    if (mr_value.is_err()) {
        return core::Err(core::ErrorCode{mr_value.unwrap_err()});
    }
    if (const auto mr_result = rt::write_register(PortHandle::us_mr_reg, mr_value.unwrap());
        mr_result.is_err()) {
        return mr_result;
    }

    auto cd = (config.peripheral_clock_hz + ((16u * baud_value(config.baudrate)) / 2u)) /
              (16u * baud_value(config.baudrate));
    if (cd == 0u) {
        cd = 1u;
    }

    const auto brgr_value = build_register_value(std::array{
        FieldWrite{PortHandle::us_cd_field, cd},
    });
    if (brgr_value.is_err()) {
        return core::Err(core::ErrorCode{brgr_value.unwrap_err()});
    }
    if (const auto brgr_result = rt::write_register(PortHandle::us_brgr_reg, brgr_value.unwrap());
        brgr_result.is_err()) {
        return brgr_result;
    }

    const auto enable_mask = build_register_value(std::array{
        FieldWrite{PortHandle::us_rxen_field, has_signal<PortHandle>("rx") ? 1u : 0u},
        FieldWrite{PortHandle::us_txen_field, has_signal<PortHandle>("tx") ? 1u : 0u},
    });
    if (enable_mask.is_err()) {
        return core::Err(core::ErrorCode{enable_mask.unwrap_err()});
    }

    return rt::write_register(PortHandle::us_cr_reg, enable_mask.unwrap());
}

template <typename PortHandle>
auto configure_uart(const PortHandle& handle) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (const auto operations_result = rt::apply_route_operations(PortHandle::operations());
        operations_result.is_err()) {
        return operations_result;
    }

    if constexpr (PortHandle::is_st_style) {
        return configure_st_uart<PortHandle>(handle.config());
    }
    if constexpr (PortHandle::is_microchip_uart_r) {
        return configure_microchip_uart_r<PortHandle>(handle.config());
    }
    if constexpr (PortHandle::is_microchip_usart_zw) {
        return configure_microchip_usart_zw<PortHandle>(handle.config());
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto write_uart_byte(const PortHandle&, std::byte value) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if constexpr (PortHandle::is_st_modern_style) {
        if (const auto ready = wait_for_field(PortHandle::txe_isr_field); ready.is_err()) {
            return ready;
        }
        return rt::modify_field(PortHandle::tdr_field,
                                static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value)));
    }
    if constexpr (PortHandle::is_st_legacy_style) {
        if (const auto ready = wait_for_field(PortHandle::txe_sr_field); ready.is_err()) {
            return ready;
        }
        return rt::modify_field(PortHandle::dr_field,
                                static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value)));
    }
    if constexpr (PortHandle::is_microchip_uart_r) {
        if (const auto ready = wait_for_field(PortHandle::txrdy_field); ready.is_err()) {
            return ready;
        }
        const auto tx_value = build_register_value(std::array{
            FieldWrite{PortHandle::txchr_field,
                       static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value))},
        });
        if (tx_value.is_err()) {
            return core::Err(core::ErrorCode{tx_value.unwrap_err()});
        }
        return rt::write_register(PortHandle::thr_reg, tx_value.unwrap());
    }
    if constexpr (PortHandle::is_microchip_usart_zw) {
        if (const auto ready = wait_for_field(PortHandle::us_txrdy_field); ready.is_err()) {
            return ready;
        }
        const auto tx_value = build_register_value(std::array{
            FieldWrite{PortHandle::us_txchr_field,
                       static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value))},
        });
        if (tx_value.is_err()) {
            return core::Err(core::ErrorCode{tx_value.unwrap_err()});
        }
        return rt::write_register(PortHandle::us_thr_reg, tx_value.unwrap());
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto write_uart(const PortHandle& handle, std::span<const std::byte> buffer)
    -> core::Result<std::size_t, core::ErrorCode> {
    auto written = std::size_t{0};
    for (const auto value : buffer) {
        const auto result = write_uart_byte(handle, value);
        if (result.is_err()) {
            if (written == 0u) {
                return core::Err(core::ErrorCode{result.err()});
            }
            return core::Ok(static_cast<std::size_t>(written));
        }
        ++written;
    }
    return core::Ok(static_cast<std::size_t>(written));
}

template <typename PortHandle>
auto read_uart(const PortHandle&, std::span<std::byte> buffer)
    -> core::Result<std::size_t, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    auto read = std::size_t{0};
    for (; read < buffer.size(); ++read) {
        core::Result<void, core::ErrorCode> ready = core::Err(core::ErrorCode::NotSupported);
        core::Result<std::uint32_t, core::ErrorCode> data =
            core::Err(core::ErrorCode::NotSupported);

        if constexpr (PortHandle::is_st_modern_style) {
            ready = wait_for_field(PortHandle::rxne_isr_field);
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::rdr_field);
            }
        } else if constexpr (PortHandle::is_st_legacy_style) {
            ready = wait_for_field(PortHandle::rxne_sr_field);
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::dr_field);
            }
        } else if constexpr (PortHandle::is_microchip_uart_r) {
            ready = wait_for_field(PortHandle::rxrdy_field);
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::rxchr_field);
            }
        } else if constexpr (PortHandle::is_microchip_usart_zw) {
            ready = wait_for_field(PortHandle::us_rxrdy_field);
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::us_rxchr_field);
            }
        }

        if (ready.is_err()) {
            if (read == 0u) {
                return core::Err(core::ErrorCode{ready.err()});
            }
            return core::Ok(static_cast<std::size_t>(read));
        }
        if (data.is_err()) {
            if (read == 0u) {
                return core::Err(core::ErrorCode{data.unwrap_err()});
            }
            return core::Ok(static_cast<std::size_t>(read));
        }

        buffer[read] = static_cast<std::byte>(data.unwrap() & 0xFFu);
    }

    return core::Ok(static_cast<std::size_t>(read));
}

template <typename PortHandle>
auto flush_uart(const PortHandle&) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if constexpr (PortHandle::is_st_modern_style) {
        return wait_for_field(PortHandle::tc_isr_field);
    }
    if constexpr (PortHandle::is_st_legacy_style) {
        return wait_for_field(PortHandle::tc_sr_field);
    }
    if constexpr (PortHandle::is_microchip_uart_r) {
        return wait_for_field(PortHandle::txempty_field);
    }
    if constexpr (PortHandle::is_microchip_usart_zw) {
        return wait_for_field(PortHandle::us_txempty_field);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

}  // namespace alloy::hal::uart::detail
