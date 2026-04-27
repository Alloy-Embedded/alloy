#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "hal/connect/tags.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/dma/detail/backend.hpp"
#include "hal/dma/types.hpp"
#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "event.hpp"

// ============================================================
// Extended UART enumerations — defined here so backend.hpp can use them
// before uart.hpp re-exports them into the alloy::hal::uart namespace.
// ============================================================
namespace alloy::hal::uart {

enum class Oversampling : std::uint8_t { X16, X8 };

enum class FifoTrigger : std::uint8_t {
    Empty         = 0,
    Quarter       = 1,
    Half          = 2,
    ThreeQuarters = 3,
    Full          = 4,
};

enum class InterruptKind : std::uint8_t {
    Tc,
    Txe,
    Rxne,
    IdleLine,
    LinBreak,
    Cts,
    Error,
    RxFifoThreshold,
    TxFifoThreshold,
};

enum class AddressLength : std::uint8_t { Bits4, Bits7 };

enum class WakeupTrigger : std::uint8_t { AddressMatch, RxneNonEmpty, StartBit };

}  // namespace alloy::hal::uart

namespace alloy::hal::uart::detail {

namespace rt = alloy::hal::detail::runtime;
namespace conn = alloy::hal::connection;

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

template <conn::detail::role_id RoleId, typename Binding>
[[nodiscard]] consteval auto binding_matches_role() -> bool {
    if constexpr (requires { Binding::role_id; }) {
        return Binding::role_id == RoleId;
    } else {
        return false;
    }
}

template <typename PortHandle, conn::detail::role_id RoleId, std::size_t... Index>
[[nodiscard]] constexpr auto has_signal_impl(std::index_sequence<Index...>) -> bool {
    using connector_type = typename PortHandle::connector_type;
    return (binding_matches_role<RoleId, typename connector_type::template binding_type<Index>>() ||
            ...);
}

template <typename PortHandle, conn::detail::role_id RoleId>
[[nodiscard]] constexpr auto has_signal() -> bool {
    using connector_type = typename PortHandle::connector_type;
    return has_signal_impl<PortHandle, RoleId>(
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
    if (!has_signal<PortHandle, conn::detail::role_id::tx>() &&
        !has_signal<PortHandle, conn::detail::role_id::rx>()) {
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
        FieldWrite{PortHandle::re_field,
               has_signal<PortHandle, conn::detail::role_id::rx>() ? 1u : 0u},
        FieldWrite{PortHandle::te_field,
               has_signal<PortHandle, conn::detail::role_id::tx>() ? 1u : 0u},
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
    if (!has_signal<PortHandle, conn::detail::role_id::tx>() &&
        !has_signal<PortHandle, conn::detail::role_id::rx>()) {
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
        FieldWrite{PortHandle::rxen_field,
               has_signal<PortHandle, conn::detail::role_id::rx>() ? 1u : 0u},
        FieldWrite{PortHandle::txen_field,
               has_signal<PortHandle, conn::detail::role_id::tx>() ? 1u : 0u},
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
    if (!has_signal<PortHandle, conn::detail::role_id::tx>() &&
        !has_signal<PortHandle, conn::detail::role_id::rx>()) {
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

    auto mr_value = build_register_value(std::array{
        FieldWrite{PortHandle::us_usart_mode_field, 0u},
        FieldWrite{PortHandle::us_usclks_field, 0u},
        FieldWrite{PortHandle::us_chrl_field, 3u},
    });
    if (mr_value.is_err()) {
        return core::Err(core::ErrorCode{mr_value.unwrap_err()});
    }
    auto mr_register_value = mr_value.unwrap();
    // The runtime contract does not publish US_MR parity/stop fields yet, but
    // the USART ZW schema requires 8N1 for the public Alloy UART contract.
    mr_register_value |= (0x4u << 9u);  // PAR = no parity

    if (const auto mr_result = rt::write_register(PortHandle::us_mr_reg, mr_register_value);
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
        FieldWrite{PortHandle::us_rxen_field,
               has_signal<PortHandle, conn::detail::role_id::rx>() ? 1u : 0u},
        FieldWrite{PortHandle::us_txen_field,
               has_signal<PortHandle, conn::detail::role_id::tx>() ? 1u : 0u},
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

    if constexpr (requires { PortHandle::peripheral_id; }) {
        if constexpr (PortHandle::peripheral_id != alloy::device::PeripheralId::none) {
            if (const auto enable_result =
                    rt::enable_peripheral_runtime_typed<PortHandle::peripheral_id>();
                enable_result.is_err()) {
                return enable_result;
            }
        }
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

template <typename PortHandle>
[[nodiscard]] inline auto st_tx_dma_field() -> rt::FieldRef {
    if constexpr (!PortHandle::is_st_style || PortHandle::peripheral_id == device::PeripheralId::none) {
        return rt::kInvalidFieldRef;
    } else {
#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE) || defined(ALLOY_BOARD_NUCLEO_F401RE)
        if constexpr (PortHandle::peripheral_id == device::PeripheralId::USART1) {
            return rt::field_ref<device::FieldId::field_usart1_cr3_dmat>();
        }
        if constexpr (PortHandle::peripheral_id == device::PeripheralId::USART2) {
            return rt::field_ref<device::FieldId::field_usart2_cr3_dmat>();
        }
#endif
        constexpr auto cr3_reg = rt::find_runtime_register_ref_by_suffix(PortHandle::peripheral_id, "cr3");
        if constexpr (!cr3_reg.valid) {
            return rt::kInvalidFieldRef;
        } else {
            return rt::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, 7u);
        }
    }
}

template <typename PortHandle>
[[nodiscard]] inline auto st_rx_dma_field() -> rt::FieldRef {
    if constexpr (!PortHandle::is_st_style || PortHandle::peripheral_id == device::PeripheralId::none) {
        return rt::kInvalidFieldRef;
    } else {
#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE) || defined(ALLOY_BOARD_NUCLEO_F401RE)
        if constexpr (PortHandle::peripheral_id == device::PeripheralId::USART1) {
            return rt::field_ref<device::FieldId::field_usart1_cr3_dmar>();
        }
        if constexpr (PortHandle::peripheral_id == device::PeripheralId::USART2) {
            return rt::field_ref<device::FieldId::field_usart2_cr3_dmar>();
        }
#endif
        constexpr auto cr3_reg = rt::find_runtime_register_ref_by_suffix(PortHandle::peripheral_id, "cr3");
        if constexpr (!cr3_reg.valid) {
            return rt::kInvalidFieldRef;
        } else {
            return rt::find_runtime_field_ref_by_register_and_offset(cr3_reg.register_id, 6u);
        }
    }
}

template <typename PortHandle, typename DmaChannel>
auto write_uart_dma(const PortHandle&, const DmaChannel& channel, std::span<const std::byte> buffer)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (buffer.empty()) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (channel.config().direction != dma::Direction::memory_to_peripheral) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    using Completion = alloy::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>;

    if constexpr (PortHandle::is_st_style) {
        const auto tx_dma_field = st_tx_dma_field<PortHandle>();
        if (!tx_dma_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        Completion::reset();
        // Ensure DMAT is clear before DMA setup so the USART does not generate
        // requests against an unconfigured or busy channel.
        static_cast<void>(rt::modify_field(tx_dma_field, 0u));
        const auto start_result = dma::detail::start_transfer(
            channel, PortHandle::tx_data_register_address(),
            reinterpret_cast<std::uintptr_t>(buffer.data()), buffer.size_bytes());
        if (start_result.is_err()) {
            return start_result;
        }
        return rt::modify_field(tx_dma_field, 1u);
    }

    // Microchip USART (SAME70): XDMAC transfers are triggered by hardware PERID routing;
    // no explicit DMA-enable bit in the USART is needed.
    if constexpr (PortHandle::is_microchip_usart_zw) {
        Completion::reset();
        return dma::detail::start_transfer(
            channel, PortHandle::tx_data_register_address(),
            reinterpret_cast<std::uintptr_t>(buffer.data()), buffer.size_bytes());
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle, typename DmaChannel>
auto read_uart_dma(const PortHandle&, const DmaChannel& channel, std::span<std::byte> buffer)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (buffer.empty()) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (channel.config().direction != dma::Direction::peripheral_to_memory) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    using Completion = alloy::dma_event::token<DmaChannel::peripheral_id, DmaChannel::signal_id>;

    if constexpr (PortHandle::is_st_style) {
        const auto rx_dma_field = st_rx_dma_field<PortHandle>();
        if (!rx_dma_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        Completion::reset();
        if (const auto dmar_result = rt::modify_field(rx_dma_field, 1u); dmar_result.is_err()) {
            return dmar_result;
        }
        return dma::detail::start_transfer(
            channel, PortHandle::rx_data_register_address(),
            reinterpret_cast<std::uintptr_t>(buffer.data()), buffer.size_bytes());
    }

    if constexpr (PortHandle::is_microchip_usart_zw) {
        Completion::reset();
        return dma::detail::start_transfer(
            channel, PortHandle::rx_data_register_address(),
            reinterpret_cast<std::uintptr_t>(buffer.data()), buffer.size_bytes());
    }

    return core::Err(core::ErrorCode::NotSupported);
}

// ============================================================
// Extended UART Coverage — phases 1-3 (host-testable)
// All ST-style implementations now use pre-computed static constexpr field members
// from port_handle (via st_uart_register_bank) rather than calling field-discovery
// functions at runtime. This eliminates the flash-overflow caused by thousands of
// unique binary-search template instantiations at -O0.
//
// Register layout for ST SCI3 (USART modern — STM32G0/G4/H7/WB):
//   CR1: offset 0x00  CR2: offset 0x04  CR3: offset 0x08  BRR: offset 0x0C
//   ISR: offset 0x1C  ICR: offset 0x20  RDR: offset 0x24  TDR: offset 0x28
// ============================================================

// ---------- Simple field helper ----------

// Read a FieldRef and return true when valid and non-zero (single-bit flag check).
[[nodiscard]] inline auto read_field_bool(rt::FieldRef field) noexcept -> bool {
    if (!field.valid) { return false; }
    const auto v = rt::read_field(field);
    return v.is_ok() && v.unwrap() != 0u;
}

// Write 1 to a FieldRef (ICR-style clear-by-write-1). Returns NotSupported when invalid.
[[nodiscard]] inline auto clear_flag_field_impl(rt::FieldRef field)
    -> core::Result<void, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return rt::modify_field(field, 1u);
}

// ---------- Phase 1: Baudrate / oversampling ----------

// set_baudrate: computes BRR from current kernel clock + validates ±2%.
// Assumes 16x oversampling (OVER8=0). Returns InvalidParameter when:
//   - clock or bps is zero
//   - BRR overflows 16 bits
//   - realised baud rate falls outside ±2% of requested rate
template <typename PortHandle>
auto set_baudrate_impl(const PortHandle& handle, std::uint32_t bps)
    -> core::Result<void, core::ErrorCode> {
    const auto clock_hz = handle.config().peripheral_clock_hz;
    if (clock_hz == 0u || bps == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if constexpr (PortHandle::is_st_style) {
        // Rounded integer BRR = fCLK / baud (16x oversampling)
        const auto divisor = (clock_hz + bps / 2u) / bps;
        if (divisor == 0u || divisor > 0xFFFFu) {
            return core::Err(core::ErrorCode::OutOfRange);
        }
        const auto realised = clock_hz / divisor;
        const auto diff = (realised > bps) ? (realised - bps) : (bps - realised);
        if (diff * 100u > bps * 2u) {
            return core::Err(core::ErrorCode::OutOfRange);
        }
        return rt::write_register(PortHandle::brr_reg, divisor);
    }
    if constexpr (PortHandle::is_microchip_uart_r || PortHandle::is_microchip_usart_zw) {
        // CD divisor for Microchip: fCLK / (16 * baud)
        const auto cd = (clock_hz + 8u * bps) / (16u * bps);
        if (cd == 0u || cd > 0xFFFFu) {
            return core::Err(core::ErrorCode::OutOfRange);
        }
        const auto realised = clock_hz / (16u * cd);
        const auto diff = (realised > bps) ? (realised - bps) : (bps - realised);
        if (diff * 100u > bps * 2u) {
            return core::Err(core::ErrorCode::OutOfRange);
        }
        const auto brgr_field = PortHandle::is_microchip_uart_r ? PortHandle::cd_field
                                                                 : PortHandle::us_cd_field;
        const auto reg = PortHandle::is_microchip_uart_r ? PortHandle::brgr_reg
                                                         : PortHandle::us_brgr_reg;
        const auto v = build_register_value(std::array{FieldWrite{brgr_field, cd}});
        if (v.is_err()) {
            return core::Err(core::ErrorCode{v.unwrap_err()});
        }
        return rt::write_register(reg, v.unwrap());
    }
    return core::Err(core::ErrorCode::NotSupported);
}

// set_oversampling: OVER8 bit in CR1 (ST-style only).
// STM32G0/F4/H7: CR1 bit 3 is TE; OVER8 is bit 15 per RM.
// UART must be disabled (UE=0) while OVER8 is changed; re-enabled after.
template <typename PortHandle>
auto set_oversampling_impl(const PortHandle&, Oversampling mode)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // OVER8 = CR1 bit 15 on all ST SCI3 / SCI2 families
        if (!PortHandle::cr1_over8_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        static_cast<void>(rt::modify_field(PortHandle::ue_field, 0u));
        const auto r = rt::modify_field(PortHandle::cr1_over8_field,
                                        (mode == Oversampling::X8) ? 1u : 0u);
        static_cast<void>(rt::modify_field(PortHandle::ue_field, 1u));
        return r;
    }
    return core::Err(core::ErrorCode::NotSupported);
}

// ---------- Phase 2: Status flags + interrupts ----------

// enable/disable a single interrupt-enable bit.
// Returns NotSupported when the field is absent on this peripheral.
template <typename PortHandle>
auto set_interrupt_enable_impl(const PortHandle&, InterruptKind kind, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::is_st_style) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        const auto value = enable ? 1u : 0u;
        rt::FieldRef field = rt::kInvalidFieldRef;

        switch (kind) {
            // CR1 interrupt enables
            case InterruptKind::IdleLine:
                field = PortHandle::cr1_idleie_field;   // IDLEIE bit 4
                break;
            case InterruptKind::Rxne:
                field = PortHandle::cr1_rxneie_field;   // RXNEIE / RXFNEIE bit 5
                break;
            case InterruptKind::Tc:
                field = PortHandle::cr1_tcie_field;     // TCIE bit 6
                break;
            case InterruptKind::Txe:
                field = PortHandle::cr1_txeie_field;    // TXEIE / TXFNFIE bit 7
                break;
            // CR2 interrupt enables
            case InterruptKind::LinBreak:
                field = PortHandle::cr2_lbdie_field;    // LBDIE bit 6
                break;
            // CR3 interrupt enables
            case InterruptKind::Error:
                field = PortHandle::cr3_eie_field;      // EIE bit 0 (ORE/FE/NE)
                break;
            case InterruptKind::Cts:
                field = PortHandle::cr3_ctsie_field;    // CTSIE bit 10
                break;
            case InterruptKind::TxFifoThreshold:
                field = PortHandle::cr3_txftie_field;   // TXFTIE bit 23
                break;
            case InterruptKind::RxFifoThreshold:
                // RXFTIE: CR3 bit 27 (pre-computed in cr3_rxftie_field).
                // Absent on families without FIFO (e.g. STM32F4) → NotSupported.
                field = PortHandle::cr3_rxftie_field;
                break;
        }

        if (!field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(field, value);
    }
}

// ---------- Phase 2: FIFO control ----------

template <typename PortHandle>
auto enable_fifo_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // FIFOEN: CR1 bit 29 (STM32G0/G4/H7; absent on F1/F4 → NotSupported)
        if (!PortHandle::cr1_fifoen_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr1_fifoen_field, enable ? 1u : 0u);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto set_tx_threshold_impl(const PortHandle&, FifoTrigger trigger)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // TXFTCFG: CR3 bits [31:29], 3-bit field
        // Trigger enum → register encoding:
        //   Empty=0(1/8), Quarter=1(1/4), Half=2(1/2), ThreeQuarters=3(3/4), Full=4(7/8)
        if (!PortHandle::cr3_txftcfg_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr3_txftcfg_field, static_cast<std::uint32_t>(trigger));
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto set_rx_threshold_impl(const PortHandle&, FifoTrigger trigger)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // RXFTCFG: CR3 bits [26:24], 3-bit field
        if (!PortHandle::cr3_rxftcfg_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr3_rxftcfg_field, static_cast<std::uint32_t>(trigger));
    }
    return core::Err(core::ErrorCode::NotSupported);
}

// ---------- Phase 3: LIN / RS-485 DE / half-duplex / smartcard / IrDA ----------

template <typename PortHandle>
auto enable_lin_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // LINEN: CR2 bit 14
        if (!PortHandle::cr2_linen_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr2_linen_field, enable ? 1u : 0u);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

// Send a LIN break character by setting SBKRQ in the USART RQR register (bit 1).
// Only valid on ST SCI3 / SCI2 families (STM32G0/F4/H7/WB); returns NotSupported
// if the RQR register is absent from the runtime database (e.g. Microchip USART).
template <typename PortHandle>
auto send_lin_break_impl(const PortHandle&)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // SBKRQ: RQR bit 1 (pre-computed in rqr_sbkrq_field) — write-only, self-clearing.
        if (!PortHandle::rqr_sbkrq_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::rqr_sbkrq_field, 1u);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

// Returns true when the LIN break detection flag (LBDF) is set.
// ISR bit 8 on modern ST; SR bit 8 on legacy ST.
// (Kept for any callers that go through this function; uart.hpp methods call read_field_bool directly.)
template <typename PortHandle>
[[nodiscard]] constexpr auto lin_break_detected_impl() noexcept -> bool {
    if constexpr (PortHandle::is_st_modern_style)
        return read_field_bool(PortHandle::isr_lbdf_field);
    else if constexpr (PortHandle::is_st_legacy_style)
        return read_field_bool(PortHandle::sr_lbdf_field);
    return false;
}

// Clear the LIN break detection flag (LBDCF: ICR bit 8 on modern ST).
// Returns NotSupported on legacy ST (flags cleared by read of DR).
// (Kept for any callers that go through this function; uart.hpp methods call clear_flag_field_impl directly.)
template <typename PortHandle>
auto clear_lin_break_flag_impl()
    -> core::Result<void, core::ErrorCode> {
    return clear_flag_field_impl(PortHandle::icr_lbdcf_field);
}

template <typename PortHandle>
auto set_half_duplex_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // HDSEL: CR3 bit 3
        if (!PortHandle::cr3_hdsel_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr3_hdsel_field, enable ? 1u : 0u);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto enable_de_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // DEM: CR3 bit 14
        if (!PortHandle::cr3_dem_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr3_dem_field, enable ? 1u : 0u);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto set_de_assertion_time_impl(const PortHandle&, std::uint8_t sample_times)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // DEAT[4:0]: CR1 bits [25:21], 5-bit field. Clamp to [0,31].
        if (!PortHandle::cr1_deat_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr1_deat_field, std::uint32_t{sample_times} & 0x1Fu);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto set_de_deassertion_time_impl(const PortHandle&, std::uint8_t sample_times)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // DEDT[4:0]: CR1 bits [20:16], 5-bit field. Clamp to [0,31].
        if (!PortHandle::cr1_dedt_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr1_dedt_field, std::uint32_t{sample_times} & 0x1Fu);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto set_smartcard_mode_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // SCEN: CR3 bit 5
        if (!PortHandle::cr3_scen_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr3_scen_field, enable ? 1u : 0u);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto set_irda_mode_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (PortHandle::is_st_style) {
        // IREN: CR3 bit 1
        if (!PortHandle::cr3_iren_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(PortHandle::cr3_iren_field, enable ? 1u : 0u);
    }
    return core::Err(core::ErrorCode::NotSupported);
}

}  // namespace alloy::hal::uart::detail
