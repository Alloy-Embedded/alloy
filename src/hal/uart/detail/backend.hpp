#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

#include "hal/detail/runtime_backend.hpp"
#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal::uart::detail {

namespace rt = alloy::hal::detail::runtime;

struct FieldWrite {
    std::string_view field{};
    std::uint32_t value = 0u;
};

template <std::size_t N>
[[nodiscard]] auto build_register_value(std::string_view peripheral_name,
                                        std::string_view register_name,
                                        const std::array<FieldWrite, N>& fields)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    auto value = std::uint32_t{0u};
    for (const auto& field : fields) {
        if (field.field.empty()) {
            continue;
        }
        const auto bits =
            rt::field_bits(peripheral_name, register_name, field.field, field.value);
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

[[nodiscard]] inline auto wait_for_field(std::string_view peripheral_name,
                                         std::string_view register_name,
                                         std::string_view field_name)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        const auto value = rt::read_field(peripheral_name, register_name, field_name);
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
auto configure_st_uart(const UartConfig& config, bool has_m1) -> core::Result<void, core::ErrorCode> {
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
            if (!has_m1) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            m1_value = 1u;
            break;
        case DataBits::Eight:
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    const auto enable_value = build_register_value(
        PortHandle::peripheral_name, "CR1",
        std::array{
            FieldWrite{"UE", 0u},
            FieldWrite{"RE", has_signal<PortHandle>("rx") ? 1u : 0u},
            FieldWrite{"TE", has_signal<PortHandle>("tx") ? 1u : 0u},
            FieldWrite{"PCE", config.parity == Parity::None ? 0u : 1u},
            FieldWrite{"PS", config.parity == Parity::Odd ? 1u : 0u},
            FieldWrite{has_m1 ? "M0" : "M", m0_value},
            FieldWrite{has_m1 ? "M1" : "", m1_value},
        });
    if (enable_value.is_err()) {
        return core::Err(core::ErrorCode{enable_value.unwrap_err()});
    }

    const auto cr1_result =
        rt::write_register(PortHandle::peripheral_name, "CR1", enable_value.unwrap());
    if (cr1_result.is_err()) {
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

    const auto stop_result =
        rt::modify_field(PortHandle::peripheral_name, "CR2", "STOP", stop_value);
    if (stop_result.is_err()) {
        return stop_result;
    }

    const auto divisor = (config.peripheral_clock_hz + (baud_value(config.baudrate) / 2u)) /
                         baud_value(config.baudrate);
    const auto brr_result =
        rt::write_register(PortHandle::peripheral_name, "BRR", divisor);
    if (brr_result.is_err()) {
        return brr_result;
    }

    return rt::modify_field(PortHandle::peripheral_name, "CR1", "UE", 1u);
}

template <typename PortHandle>
auto configure_microchip_uart_r(const UartConfig& config)
    -> core::Result<void, core::ErrorCode> {
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

    const auto reset_mask = build_register_value(
        PortHandle::peripheral_name, "CR",
        std::array{
            FieldWrite{"RSTRX", 1u},
            FieldWrite{"RSTTX", 1u},
            FieldWrite{"RXDIS", 1u},
            FieldWrite{"TXDIS", 1u},
            FieldWrite{"RSTSTA", 1u},
        });
    if (reset_mask.is_err()) {
        return core::Err(core::ErrorCode{reset_mask.unwrap_err()});
    }
    const auto reset_result =
        rt::write_register(PortHandle::peripheral_name, "CR", reset_mask.unwrap());
    if (reset_result.is_err()) {
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

    const auto mr_value = build_register_value(
        PortHandle::peripheral_name, "MR",
        std::array{
            FieldWrite{"PAR", parity},
            FieldWrite{"CHMODE", 0u},
        });
    if (mr_value.is_err()) {
        return core::Err(core::ErrorCode{mr_value.unwrap_err()});
    }
    const auto mr_result =
        rt::write_register(PortHandle::peripheral_name, "MR", mr_value.unwrap());
    if (mr_result.is_err()) {
        return mr_result;
    }

    auto cd = (config.peripheral_clock_hz + ((16u * baud_value(config.baudrate)) / 2u)) /
              (16u * baud_value(config.baudrate));
    if (cd == 0u) {
        cd = 1u;
    }
    const auto brgr_value =
        build_register_value(PortHandle::peripheral_name, "BRGR",
                             std::array{FieldWrite{"CD", cd}});
    if (brgr_value.is_err()) {
        return core::Err(core::ErrorCode{brgr_value.unwrap_err()});
    }
    const auto brgr_result =
        rt::write_register(PortHandle::peripheral_name, "BRGR", brgr_value.unwrap());
    if (brgr_result.is_err()) {
        return brgr_result;
    }

    const auto enable_mask = build_register_value(
        PortHandle::peripheral_name, "CR",
        std::array{
            FieldWrite{"RXEN", has_signal<PortHandle>("rx") ? 1u : 0u},
            FieldWrite{"TXEN", has_signal<PortHandle>("tx") ? 1u : 0u},
        });
    if (enable_mask.is_err()) {
        return core::Err(core::ErrorCode{enable_mask.unwrap_err()});
    }
    return rt::write_register(PortHandle::peripheral_name, "CR", enable_mask.unwrap());
}

template <typename PortHandle>
auto configure_microchip_usart_zw(const UartConfig& config)
    -> core::Result<void, core::ErrorCode> {
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

    const auto reset_mask = build_register_value(
        PortHandle::peripheral_name, "US_CR_LIN_MODE",
        std::array{
            FieldWrite{"RSTRX", 1u},
            FieldWrite{"RSTTX", 1u},
            FieldWrite{"RXDIS", 1u},
            FieldWrite{"TXDIS", 1u},
            FieldWrite{"RSTSTA", 1u},
        });
    if (reset_mask.is_err()) {
        return core::Err(core::ErrorCode{reset_mask.unwrap_err()});
    }
    const auto reset_result =
        rt::write_register(PortHandle::peripheral_name, "US_CR_LIN_MODE", reset_mask.unwrap());
    if (reset_result.is_err()) {
        return reset_result;
    }

    const auto mr_value = build_register_value(
        PortHandle::peripheral_name, "US_MR_SPI_MODE",
        std::array{
            FieldWrite{"USART_MODE", 0u},
            FieldWrite{"USCLKS", 0u},
            FieldWrite{"CHRL", 3u},
        });
    if (mr_value.is_err()) {
        return core::Err(core::ErrorCode{mr_value.unwrap_err()});
    }
    const auto mr_result =
        rt::write_register(PortHandle::peripheral_name, "US_MR_SPI_MODE", mr_value.unwrap());
    if (mr_result.is_err()) {
        return mr_result;
    }

    auto cd = (config.peripheral_clock_hz + ((16u * baud_value(config.baudrate)) / 2u)) /
              (16u * baud_value(config.baudrate));
    if (cd == 0u) {
        cd = 1u;
    }
    const auto brgr_value =
        build_register_value(PortHandle::peripheral_name, "US_BRGR",
                             std::array{FieldWrite{"CD", cd}});
    if (brgr_value.is_err()) {
        return core::Err(core::ErrorCode{brgr_value.unwrap_err()});
    }
    const auto brgr_result =
        rt::write_register(PortHandle::peripheral_name, "US_BRGR", brgr_value.unwrap());
    if (brgr_result.is_err()) {
        return brgr_result;
    }

    const auto enable_mask = build_register_value(
        PortHandle::peripheral_name, "US_CR_LIN_MODE",
        std::array{
            FieldWrite{"RXEN", has_signal<PortHandle>("rx") ? 1u : 0u},
            FieldWrite{"TXEN", has_signal<PortHandle>("tx") ? 1u : 0u},
        });
    if (enable_mask.is_err()) {
        return core::Err(core::ErrorCode{enable_mask.unwrap_err()});
    }
    return rt::write_register(PortHandle::peripheral_name, "US_CR_LIN_MODE",
                              enable_mask.unwrap());
}

template <typename PortHandle>
auto configure_uart(const PortHandle& handle) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto operations_result = rt::apply_route_operations(PortHandle::operations());
    if (operations_result.is_err()) {
        return operations_result;
    }

    constexpr auto schema = rt::uart_schema_for<PortHandle>();
    if constexpr (schema == rt::UartSchema::st_sci3_v2_1_cube) {
        return configure_st_uart<PortHandle>(handle.config(), true);
    }
    if constexpr (schema == rt::UartSchema::st_sci2_v1_2_cube) {
        return configure_st_uart<PortHandle>(handle.config(), false);
    }
    if constexpr (schema == rt::UartSchema::microchip_uart_r) {
        return configure_microchip_uart_r<PortHandle>(handle.config());
    }
    if constexpr (schema == rt::UartSchema::microchip_usart_zw) {
        return configure_microchip_usart_zw<PortHandle>(handle.config());
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto write_uart_byte(const PortHandle&, std::byte value) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr auto schema = rt::uart_schema_for<PortHandle>();
    if constexpr (schema == rt::UartSchema::st_sci3_v2_1_cube) {
        const auto ready = wait_for_field(PortHandle::peripheral_name, "ISR", "TXE");
        if (ready.is_err()) {
            return ready;
        }
        return rt::modify_field(PortHandle::peripheral_name, "TDR", "TDR",
                                static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value)));
    }
    if constexpr (schema == rt::UartSchema::st_sci2_v1_2_cube) {
        const auto ready = wait_for_field(PortHandle::peripheral_name, "SR", "TXE");
        if (ready.is_err()) {
            return ready;
        }
        return rt::modify_field(PortHandle::peripheral_name, "DR", "DR",
                                static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value)));
    }
    if constexpr (schema == rt::UartSchema::microchip_uart_r) {
        const auto ready = wait_for_field(PortHandle::peripheral_name, "SR", "TXRDY");
        if (ready.is_err()) {
            return ready;
        }
        const auto tx_value = build_register_value(
            PortHandle::peripheral_name, "THR",
            std::array{FieldWrite{"TXCHR", static_cast<std::uint32_t>(
                                               std::to_integer<std::uint8_t>(value))}});
        if (tx_value.is_err()) {
            return core::Err(core::ErrorCode{tx_value.unwrap_err()});
        }
        return rt::write_register(PortHandle::peripheral_name, "THR", tx_value.unwrap());
    }
    if constexpr (schema == rt::UartSchema::microchip_usart_zw) {
        const auto ready = wait_for_field(PortHandle::peripheral_name, "US_CSR_LIN_MODE", "TXRDY");
        if (ready.is_err()) {
            return ready;
        }
        const auto tx_value = build_register_value(
            PortHandle::peripheral_name, "US_THR",
            std::array{FieldWrite{"TXCHR", static_cast<std::uint32_t>(
                                               std::to_integer<std::uint8_t>(value))}});
        if (tx_value.is_err()) {
            return core::Err(core::ErrorCode{tx_value.unwrap_err()});
        }
        return rt::write_register(PortHandle::peripheral_name, "US_THR", tx_value.unwrap());
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
    constexpr auto schema = rt::uart_schema_for<PortHandle>();
    for (; read < buffer.size(); ++read) {
        core::Result<void, core::ErrorCode> ready = core::Err(core::ErrorCode::NotSupported);
        core::Result<std::uint32_t, core::ErrorCode> data =
            core::Err(core::ErrorCode::NotSupported);

        if constexpr (schema == rt::UartSchema::st_sci3_v2_1_cube) {
            ready = wait_for_field(PortHandle::peripheral_name, "ISR", "RXNE");
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::peripheral_name, "RDR", "RDR");
            }
        } else if constexpr (schema == rt::UartSchema::st_sci2_v1_2_cube) {
            ready = wait_for_field(PortHandle::peripheral_name, "SR", "RXNE");
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::peripheral_name, "DR", "DR");
            }
        } else if constexpr (schema == rt::UartSchema::microchip_uart_r) {
            ready = wait_for_field(PortHandle::peripheral_name, "SR", "RXRDY");
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::peripheral_name, "RHR", "RXCHR");
            }
        } else if constexpr (schema == rt::UartSchema::microchip_usart_zw) {
            ready = wait_for_field(PortHandle::peripheral_name, "US_CSR_LIN_MODE", "RXRDY");
            if (ready.is_ok()) {
                data = rt::read_field(PortHandle::peripheral_name, "US_RHR", "RXCHR");
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

    constexpr auto schema = rt::uart_schema_for<PortHandle>();
    if constexpr (schema == rt::UartSchema::st_sci3_v2_1_cube) {
        return wait_for_field(PortHandle::peripheral_name, "ISR", "TC");
    }
    if constexpr (schema == rt::UartSchema::st_sci2_v1_2_cube) {
        return wait_for_field(PortHandle::peripheral_name, "SR", "TC");
    }
    if constexpr (schema == rt::UartSchema::microchip_uart_r) {
        return wait_for_field(PortHandle::peripheral_name, "SR", "TXEMPTY");
    }
    if constexpr (schema == rt::UartSchema::microchip_usart_zw) {
        return wait_for_field(PortHandle::peripheral_name, "US_CSR_LIN_MODE", "TXEMPTY");
    }

    return core::Err(core::ErrorCode::NotSupported);
}

}  // namespace alloy::hal::uart::detail
