#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/spi/types.hpp"

namespace alloy::hal::spi::detail {

namespace rt = alloy::hal::detail::runtime;
using BitOrder = SpiBitOrder;
using DataSize = SpiDataSize;
using Mode = SpiMode;

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

[[nodiscard]] constexpr auto mode_cpol(Mode mode) -> std::uint32_t {
    return mode == Mode::Mode2 || mode == Mode::Mode3 ? 1u : 0u;
}

[[nodiscard]] constexpr auto mode_cpha(Mode mode) -> std::uint32_t {
    return mode == Mode::Mode1 || mode == Mode::Mode3 ? 1u : 0u;
}

[[nodiscard]] constexpr auto mode_ncpha(Mode mode) -> std::uint32_t {
    return mode_cpha(mode) == 0u ? 1u : 0u;
}

[[nodiscard]] constexpr auto st_spi_prescaler_bits(std::uint32_t peripheral_clock_hz,
                                                   std::uint32_t target_clock_hz)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (peripheral_clock_hz == 0u || target_clock_hz == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr std::array divisors{2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u};
    const auto ratio = (peripheral_clock_hz + target_clock_hz - 1u) / target_clock_hz;
    for (std::size_t index = 0; index < divisors.size(); ++index) {
        if (ratio <= divisors[index]) {
            return core::Ok(static_cast<std::uint32_t>(index));
        }
    }
    return core::Ok(static_cast<std::uint32_t>(7u));
}

[[nodiscard]] constexpr auto microchip_spi_scbr(std::uint32_t peripheral_clock_hz,
                                                std::uint32_t target_clock_hz)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (peripheral_clock_hz == 0u || target_clock_hz == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    auto divider = (peripheral_clock_hz + target_clock_hz - 1u) / target_clock_hz;
    if (divider == 0u) {
        divider = 1u;
    }
    if (divider > 255u) {
        divider = 255u;
    }
    return core::Ok(static_cast<std::uint32_t>(divider));
}

[[nodiscard]] constexpr auto microchip_spi_bits(DataSize data_size)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    switch (data_size) {
        case DataSize::Bits8:
            return core::Ok(static_cast<std::uint32_t>(0u));
        case DataSize::Bits16:
            return core::Ok(static_cast<std::uint32_t>(8u));
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

[[nodiscard]] inline auto wait_for_field_set(const rt::FieldRef& field)
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

[[nodiscard]] inline auto wait_for_field_clear(const rt::FieldRef& field)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        const auto value = rt::read_field(field);
        if (value.is_err()) {
            return core::Err(core::ErrorCode{value.unwrap_err()});
        }
        if (value.unwrap() == 0u) {
            return core::Ok();
        }
    }
    return core::Err(core::ErrorCode::Timeout);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_cr1_reg() -> rt::RegisterRef {
    return PortHandle::cr1_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_cr2_reg() -> rt::RegisterRef {
    return PortHandle::cr2_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_sr_reg() -> rt::RegisterRef {
    return PortHandle::sr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_dr_reg() -> rt::RegisterRef {
    return PortHandle::dr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_cpha_field() -> rt::FieldRef {
    return PortHandle::cpha_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_cpol_field() -> rt::FieldRef {
    return PortHandle::cpol_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_mstr_field() -> rt::FieldRef {
    return PortHandle::mstr_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_br_field() -> rt::FieldRef {
    return PortHandle::br_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_spe_field() -> rt::FieldRef {
    return PortHandle::spe_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_lsbfirst_field() -> rt::FieldRef {
    return PortHandle::lsbfirst_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_ssi_field() -> rt::FieldRef {
    return PortHandle::ssi_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_ssm_field() -> rt::FieldRef {
    return PortHandle::ssm_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_dff_field() -> rt::FieldRef {
    return PortHandle::dff_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_ds_field() -> rt::FieldRef {
    return PortHandle::ds_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_frxth_field() -> rt::FieldRef {
    return PortHandle::frxth_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_txe_field() -> rt::FieldRef {
    return PortHandle::txe_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_rxne_field() -> rt::FieldRef {
    return PortHandle::rxne_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_bsy_field() -> rt::FieldRef {
    return PortHandle::bsy_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_dr_field() -> rt::FieldRef {
    return PortHandle::dr_data_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_cr_reg() -> rt::RegisterRef {
    return PortHandle::cr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_mr_reg() -> rt::RegisterRef {
    return PortHandle::mr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_csr_reg() -> rt::RegisterRef {
    return PortHandle::csr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_sr_reg() -> rt::RegisterRef {
    return PortHandle::sr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_tdr_reg() -> rt::RegisterRef {
    return PortHandle::tdr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_rdr_reg() -> rt::RegisterRef {
    return PortHandle::rdr_reg;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_spien_field() -> rt::FieldRef {
    return PortHandle::spien_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_spidis_field() -> rt::FieldRef {
    return PortHandle::spidis_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_swrst_field() -> rt::FieldRef {
    return PortHandle::swrst_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_mstr_field() -> rt::FieldRef {
    return PortHandle::mstr_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_ps_field() -> rt::FieldRef {
    return PortHandle::ps_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_pcsdec_field() -> rt::FieldRef {
    return PortHandle::pcsdec_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_modfdis_field() -> rt::FieldRef {
    return PortHandle::modfdis_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_pcs_field() -> rt::FieldRef {
    return PortHandle::pcs_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_dlybcs_field() -> rt::FieldRef {
    return PortHandle::dlybcs_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_cpol_field() -> rt::FieldRef {
    return PortHandle::cpol_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_ncpha_field() -> rt::FieldRef {
    return PortHandle::ncpha_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_bits_field() -> rt::FieldRef {
    return PortHandle::bits_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_scbr_field() -> rt::FieldRef {
    return PortHandle::scbr_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_dlybs_field() -> rt::FieldRef {
    return PortHandle::dlybs_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_dlybct_field() -> rt::FieldRef {
    return PortHandle::dlybct_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_tdre_field() -> rt::FieldRef {
    return PortHandle::tdre_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_rdrf_field() -> rt::FieldRef {
    return PortHandle::rdrf_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_txempty_field() -> rt::FieldRef {
    return PortHandle::txempty_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_td_field() -> rt::FieldRef {
    return PortHandle::td_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_tdr_pcs_field() -> rt::FieldRef {
    return PortHandle::tdr_pcs_field;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto mc_rd_field() -> rt::FieldRef {
    return PortHandle::rd_field;
}

template <typename PortHandle>
auto configure_st_spi(const SpiConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.peripheral_clock_hz == 0u || config.clock_speed == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto br = st_spi_prescaler_bits(config.peripheral_clock_hz, config.clock_speed);
    if (br.is_err()) {
        return core::Err(core::ErrorCode{br.unwrap_err()});
    }

    constexpr auto cr1_reg = st_cr1_reg<PortHandle>();
    constexpr auto spe = st_spe_field<PortHandle>();
    constexpr auto ds = st_ds_field<PortHandle>();
    constexpr auto dff = st_dff_field<PortHandle>();
    constexpr auto frxth = st_frxth_field<PortHandle>();

    if (!cr1_reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (spe.valid) {
        const auto disable_result = rt::modify_field(spe, 0u);
        if (disable_result.is_err()) {
            return disable_result;
        }
    }

    const auto cr1_value = build_register_value(std::array<FieldWrite, 8>{
        FieldWrite{st_cpha_field<PortHandle>(), mode_cpha(config.mode)},
        FieldWrite{st_cpol_field<PortHandle>(), mode_cpol(config.mode)},
        FieldWrite{st_mstr_field<PortHandle>(), 1u},
        FieldWrite{st_br_field<PortHandle>(), br.unwrap()},
        FieldWrite{st_lsbfirst_field<PortHandle>(),
                   config.bit_order == BitOrder::LsbFirst ? 1u : 0u},
        FieldWrite{st_ssi_field<PortHandle>(), 1u},
        FieldWrite{st_ssm_field<PortHandle>(), 1u},
        FieldWrite{st_spe_field<PortHandle>(), 0u},
    });
    if (cr1_value.is_err()) {
        return core::Err(core::ErrorCode{cr1_value.unwrap_err()});
    }
    if (const auto cr1_result = rt::write_register(cr1_reg, cr1_value.unwrap());
        cr1_result.is_err()) {
        return cr1_result;
    }

    if (ds.valid) {
        const auto ds_value = config.data_size == DataSize::Bits16 ? 15u : 7u;
        const auto ds_result = rt::modify_field(ds, ds_value);
        if (ds_result.is_err()) {
            return ds_result;
        }
        if (frxth.valid) {
            const auto frxth_result =
                rt::modify_field(frxth, config.data_size == DataSize::Bits8 ? 1u : 0u);
            if (frxth_result.is_err()) {
                return frxth_result;
            }
        }
    } else if (dff.valid) {
        const auto dff_result =
            rt::modify_field(dff, config.data_size == DataSize::Bits16 ? 1u : 0u);
        if (dff_result.is_err()) {
            return dff_result;
        }
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }

    return rt::modify_field(spe, 1u);
}

template <typename PortHandle>
auto configure_microchip_spi(const SpiConfig& config) -> core::Result<void, core::ErrorCode> {
    if (config.peripheral_clock_hz == 0u || config.clock_speed == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto scbr = microchip_spi_scbr(config.peripheral_clock_hz, config.clock_speed);
    if (scbr.is_err()) {
        return core::Err(core::ErrorCode{scbr.unwrap_err()});
    }

    const auto bits = microchip_spi_bits(config.data_size);
    if (bits.is_err()) {
        return core::Err(core::ErrorCode{bits.unwrap_err()});
    }

    constexpr auto cr_reg = mc_cr_reg<PortHandle>();
    constexpr auto mr_reg = mc_mr_reg<PortHandle>();
    constexpr auto csr_reg = mc_csr_reg<PortHandle>();
    if (!cr_reg.valid || !mr_reg.valid || !csr_reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto reset_mask = build_register_value(std::array<FieldWrite, 2>{
        FieldWrite{mc_spidis_field<PortHandle>(), 1u},
        FieldWrite{mc_swrst_field<PortHandle>(), 1u},
    });
    if (reset_mask.is_err()) {
        return core::Err(core::ErrorCode{reset_mask.unwrap_err()});
    }
    if (const auto reset_result = rt::write_register(cr_reg, reset_mask.unwrap());
        reset_result.is_err()) {
        return reset_result;
    }

    const auto mr_value = build_register_value(std::array<FieldWrite, 6>{
        FieldWrite{mc_mstr_field<PortHandle>(), 1u},
        FieldWrite{mc_ps_field<PortHandle>(), 1u},
        FieldWrite{mc_pcsdec_field<PortHandle>(), 0u},
        FieldWrite{mc_modfdis_field<PortHandle>(), 1u},
        FieldWrite{mc_pcs_field<PortHandle>(), 0xFu},
        FieldWrite{mc_dlybcs_field<PortHandle>(), 0u},
    });
    if (mr_value.is_err()) {
        return core::Err(core::ErrorCode{mr_value.unwrap_err()});
    }
    if (const auto mr_result = rt::write_register(mr_reg, mr_value.unwrap());
        mr_result.is_err()) {
        return mr_result;
    }

    const auto csr_value = build_register_value(std::array<FieldWrite, 6>{
        FieldWrite{mc_cpol_field<PortHandle>(), mode_cpol(config.mode)},
        FieldWrite{mc_ncpha_field<PortHandle>(), mode_ncpha(config.mode)},
        FieldWrite{mc_bits_field<PortHandle>(), bits.unwrap()},
        FieldWrite{mc_scbr_field<PortHandle>(), scbr.unwrap()},
        FieldWrite{mc_dlybs_field<PortHandle>(), 0u},
        FieldWrite{mc_dlybct_field<PortHandle>(), 0u},
    });
    if (csr_value.is_err()) {
        return core::Err(core::ErrorCode{csr_value.unwrap_err()});
    }
    if (const auto csr_result = rt::write_register(csr_reg, csr_value.unwrap());
        csr_result.is_err()) {
        return csr_result;
    }

    const auto enable_mask = build_register_value(std::array<FieldWrite, 1>{
        FieldWrite{mc_spien_field<PortHandle>(), 1u},
    });
    if (enable_mask.is_err()) {
        return core::Err(core::ErrorCode{enable_mask.unwrap_err()});
    }
    return rt::write_register(cr_reg, enable_mask.unwrap());
}

template <typename PortHandle>
auto configure_spi(const PortHandle& port) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (const auto operations_result = rt::apply_route_operations(PortHandle::operations());
        operations_result.is_err()) {
        return operations_result;
    }

    switch (rt::spi_schema_for<PortHandle>()) {
        case rt::SpiSchema::st_spi2s1_v3_3_cube:
        case rt::SpiSchema::st_spi2s1_v2_2_cube:
            return configure_st_spi<PortHandle>(port.config());
        case rt::SpiSchema::microchip_spi_zm:
            return configure_microchip_spi<PortHandle>(port.config());
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename PortHandle>
[[nodiscard]] auto transfer_st_spi(const PortHandle& port, std::span<const std::uint8_t> tx_buffer,
                                   std::span<std::uint8_t> rx_buffer)
    -> core::Result<void, core::ErrorCode> {
    if (port.config().data_size != DataSize::Bits8) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    constexpr auto txe = st_txe_field<PortHandle>();
    constexpr auto rxne = st_rxne_field<PortHandle>();
    constexpr auto bsy = st_bsy_field<PortHandle>();
    constexpr auto dr = st_dr_field<PortHandle>();
    if (!txe.valid || !rxne.valid || !dr.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto count = tx_buffer.size() < rx_buffer.size() ? tx_buffer.size() : rx_buffer.size();
    for (std::size_t index = 0; index < count; ++index) {
        const auto tx_ready = wait_for_field_set(txe);
        if (tx_ready.is_err()) {
            return tx_ready;
        }
        const auto write_result = rt::modify_field(dr, tx_buffer[index]);
        if (write_result.is_err()) {
            return write_result;
        }

        const auto rx_ready = wait_for_field_set(rxne);
        if (rx_ready.is_err()) {
            return rx_ready;
        }
        const auto read_value = rt::read_field(dr);
        if (read_value.is_err()) {
            return core::Err(core::ErrorCode{read_value.unwrap_err()});
        }
        rx_buffer[index] = static_cast<std::uint8_t>(read_value.unwrap());
    }

    if (bsy.valid) {
        return wait_for_field_clear(bsy);
    }
    return core::Ok();
}

template <typename PortHandle>
[[nodiscard]] auto transfer_microchip_spi(const PortHandle& port,
                                          std::span<const std::uint8_t> tx_buffer,
                                          std::span<std::uint8_t> rx_buffer)
    -> core::Result<void, core::ErrorCode> {
    if (port.config().data_size != DataSize::Bits8) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    constexpr auto tdre = mc_tdre_field<PortHandle>();
    constexpr auto rdrf = mc_rdrf_field<PortHandle>();
    constexpr auto txempty = mc_txempty_field<PortHandle>();
    constexpr auto tdr_reg = mc_tdr_reg<PortHandle>();
    constexpr auto rd = mc_rd_field<PortHandle>();
    constexpr auto td = mc_td_field<PortHandle>();
    constexpr auto pcs = mc_tdr_pcs_field<PortHandle>();
    if (!tdre.valid || !rdrf.valid || !txempty.valid || !td.valid || !pcs.valid || !rd.valid ||
        !tdr_reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto count = tx_buffer.size() < rx_buffer.size() ? tx_buffer.size() : rx_buffer.size();
    for (std::size_t index = 0; index < count; ++index) {
        const auto tx_ready = wait_for_field_set(tdre);
        if (tx_ready.is_err()) {
            return tx_ready;
        }

        const auto tdr_value = build_register_value(
            std::array<FieldWrite, 2>{FieldWrite{td, tx_buffer[index]}, FieldWrite{pcs, 0xFu}});
        if (tdr_value.is_err()) {
            return core::Err(core::ErrorCode{tdr_value.unwrap_err()});
        }
        if (const auto write_result = rt::write_register(tdr_reg, tdr_value.unwrap());
            write_result.is_err()) {
            return write_result;
        }

        const auto rx_ready = wait_for_field_set(rdrf);
        if (rx_ready.is_err()) {
            return rx_ready;
        }
        const auto read_value = rt::read_field(rd);
        if (read_value.is_err()) {
            return core::Err(core::ErrorCode{read_value.unwrap_err()});
        }
        rx_buffer[index] = static_cast<std::uint8_t>(read_value.unwrap());
    }

    return wait_for_field_set(txempty);
}

template <typename PortHandle>
auto transfer_spi(const PortHandle& port, std::span<const std::uint8_t> tx_buffer,
                  std::span<std::uint8_t> rx_buffer) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (tx_buffer.empty() || rx_buffer.empty()) {
        return core::Ok();
    }

    switch (rt::spi_schema_for<PortHandle>()) {
        case rt::SpiSchema::st_spi2s1_v3_3_cube:
        case rt::SpiSchema::st_spi2s1_v2_2_cube:
            return transfer_st_spi(port, tx_buffer, rx_buffer);
        case rt::SpiSchema::microchip_spi_zm:
            return transfer_microchip_spi(port, tx_buffer, rx_buffer);
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

template <typename PortHandle>
auto transmit_spi(const PortHandle& port, std::span<const std::uint8_t> tx_buffer)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    std::array<std::uint8_t, 32> discard{};
    auto remaining = tx_buffer;
    while (!remaining.empty()) {
        const auto chunk_size =
            remaining.size() < discard.size() ? remaining.size() : discard.size();
        const auto result = transfer_spi(port, remaining.first(chunk_size),
                                         std::span<std::uint8_t>{discard.data(), chunk_size});
        if (result.is_err()) {
            return result;
        }
        remaining = remaining.subspan(chunk_size);
    }
    return core::Ok();
}

template <typename PortHandle>
auto receive_spi(const PortHandle& port, std::span<std::uint8_t> rx_buffer)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    std::array<std::uint8_t, 32> dummy{};
    dummy.fill(0xFFu);
    auto remaining = rx_buffer;
    while (!remaining.empty()) {
        const auto chunk_size = remaining.size() < dummy.size() ? remaining.size() : dummy.size();
        const auto result =
            transfer_spi(port, std::span<const std::uint8_t>{dummy.data(), chunk_size},
                         remaining.first(chunk_size));
        if (result.is_err()) {
            return result;
        }
        remaining = remaining.subspan(chunk_size);
    }
    return core::Ok();
}

template <typename PortHandle>
[[nodiscard]] auto spi_is_busy(const PortHandle&) -> bool {
    if constexpr (!PortHandle::valid) {
        return false;
    }

    switch (rt::spi_schema_for<PortHandle>()) {
        case rt::SpiSchema::st_spi2s1_v3_3_cube:
        case rt::SpiSchema::st_spi2s1_v2_2_cube: {
            constexpr auto bsy = st_bsy_field<PortHandle>();
            if (!bsy.valid) {
                return false;
            }
            const auto value = rt::read_field(bsy);
            return value.is_ok() && value.unwrap() != 0u;
        }
        case rt::SpiSchema::microchip_spi_zm: {
            constexpr auto txempty = mc_txempty_field<PortHandle>();
            if (!txempty.valid) {
                return false;
            }
            const auto value = rt::read_field(txempty);
            return value.is_ok() && value.unwrap() == 0u;
        }
        default:
            return false;
    }
}

}  // namespace alloy::hal::spi::detail
