#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/spi/types.hpp"

// ============================================================
// Extended SPI enumerations — defined here so backend.hpp can use them
// before spi.hpp re-exports them into the alloy::hal::spi namespace.
// ============================================================
namespace alloy::hal::spi {

enum class FrameFormat : std::uint8_t { Motorola, TI };

enum class BiDir : std::uint8_t { Receive, Transmit };

enum class NssManagement : std::uint8_t { Software, HardwareInput, HardwareOutput };

enum class InterruptKind : std::uint8_t {
    Txe,
    Rxne,
    Error,
    ModeFault,
    CrcError,
    FrameError,
};

}  // namespace alloy::hal::spi

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

    if constexpr (requires { PortHandle::peripheral_id; }) {
        if constexpr (PortHandle::peripheral_id != alloy::device::PeripheralId::none) {
            if (const auto enable_result =
                    rt::enable_peripheral_runtime_typed<PortHandle::peripheral_id>();
                enable_result.is_err()) {
                return enable_result;
            }
        }
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

        // PCS=0xE selects NPCS0 internally so the SPI generates a clock.
        // PCS=0xF ("no peripheral") suppresses the clock on Microchip SPI.
        const auto tdr_value = build_register_value(
            std::array<FieldWrite, 2>{FieldWrite{td, tx_buffer[index]}, FieldWrite{pcs, 0xEu}});
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

// ============================================================
// Extended SPI HAL helpers (extend-spi-coverage)
// ============================================================

// Returns true when valid and non-zero (single-bit flag check).
[[nodiscard]] inline auto read_field_bool(rt::FieldRef field) noexcept -> bool {
    if (!field.valid) {
        return false;
    }
    const auto v = rt::read_field(field);
    return v.is_ok() && v.unwrap() != 0u;
}

// Build a synthetic FieldRef anchored at an existing register's MMIO address.
// MMIO accesses only need base_address + offset_bytes; FieldId/RegisterId are
// not consulted by runtime_ops, so this works on backends whose descriptor
// publishes RegisterId::none for some registers (e.g. STM32G0 SPI CR2/SR/DR).
[[nodiscard]] constexpr auto make_synthetic_field(rt::RegisterRef reg,
                                                  std::uint16_t bit_offset,
                                                  std::uint16_t bit_width) -> rt::FieldRef {
    if (!reg.valid) {
        return rt::kInvalidFieldRef;
    }
    return rt::FieldRef{rt::FieldId::none, reg, bit_offset, bit_width, true};
}

// Build a synthetic RegisterRef anchored at an existing peripheral base.
[[nodiscard]] constexpr auto make_synthetic_register(rt::RegisterRef anchor,
                                                     std::uintptr_t offset_bytes)
    -> rt::RegisterRef {
    if (!anchor.valid) {
        return rt::kInvalidRegisterRef;
    }
    return rt::RegisterRef{rt::RegisterId::none, anchor.base_address,
                           static_cast<std::uint16_t>(offset_bytes), true};
}

// ---------- Phase 1: Variable data size ----------

// True when `width` is in the descriptor's published kSupportedFrameSizes.
template <typename PortHandle>
[[nodiscard]] constexpr auto frame_size_supported(std::uint8_t width) -> bool {
    for (const auto supported : PortHandle::semantic_traits::kSupportedFrameSizes) {
        if (supported == width) {
            return true;
        }
    }
    return false;
}

template <typename PortHandle>
[[nodiscard]] auto set_data_size_impl(const PortHandle&, std::uint8_t bits)
    -> core::Result<void, core::ErrorCode> {
    if (bits < 4u || bits > 16u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (!frame_size_supported<PortHandle>(bits)) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    constexpr auto ds_field = PortHandle::ds_field;
    constexpr auto dff_field = PortHandle::dff_field;
    constexpr auto frxth_field = PortHandle::frxth_field;
    constexpr auto bits_field = PortHandle::bits_field;

    // STM32: prefer DFF when width is 8 / 16 (legacy F4 SPI semantics).
    // Use DS only for non-{8,16} widths so we don't write the I2S-mode DS
    // field on legacy peripherals where DFF is the real selector.
    if constexpr (dff_field.valid) {
        if (bits == 8u) {
            const auto r = rt::modify_field(dff_field, 0u);
            if (r.is_err()) {
                return r;
            }
            if constexpr (frxth_field.valid) {
                static_cast<void>(rt::modify_field(frxth_field, 1u));
            }
            return core::Ok();
        }
        if (bits == 16u) {
            const auto r = rt::modify_field(dff_field, 1u);
            if (r.is_err()) {
                return r;
            }
            if constexpr (frxth_field.valid) {
                static_cast<void>(rt::modify_field(frxth_field, 0u));
            }
            return core::Ok();
        }
    }

    if constexpr (ds_field.valid) {
        const auto encoding = static_cast<std::uint32_t>(bits - 1u);
        const auto r = rt::modify_field(ds_field, encoding);
        if (r.is_err()) {
            return r;
        }
        if constexpr (frxth_field.valid) {
            static_cast<void>(rt::modify_field(frxth_field, bits <= 8u ? 1u : 0u));
        }
        return core::Ok();
    }

    if constexpr (bits_field.valid) {
        // SAME70: BITS encodes (bits - 8); valid for 8..16.
        if (bits < 8u) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        return rt::modify_field(bits_field, static_cast<std::uint32_t>(bits - 8u));
    }

    return core::Err(core::ErrorCode::NotSupported);
}

// ---------- Phase 1: Variable clock speed (raw, ±5 % validated) ----------

template <typename PortHandle>
[[nodiscard]] constexpr auto compute_realised_clock_st(std::uint32_t kernel_hz,
                                                       std::uint32_t target_hz)
    -> std::uint32_t {
    if (kernel_hz == 0u || target_hz == 0u) {
        return 0u;
    }
    constexpr std::array divisors{2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u};
    const auto ratio = (kernel_hz + target_hz - 1u) / target_hz;
    for (const auto divisor : divisors) {
        if (ratio <= divisor) {
            return kernel_hz / divisor;
        }
    }
    return kernel_hz / divisors.back();
}

template <typename PortHandle>
[[nodiscard]] constexpr auto compute_realised_clock_microchip(std::uint32_t kernel_hz,
                                                              std::uint32_t target_hz)
    -> std::uint32_t {
    if (kernel_hz == 0u || target_hz == 0u) {
        return 0u;
    }
    auto divider = (kernel_hz + target_hz - 1u) / target_hz;
    if (divider == 0u) {
        divider = 1u;
    }
    if (divider > 255u) {
        divider = 255u;
    }
    return kernel_hz / divider;
}

template <typename PortHandle>
[[nodiscard]] auto realised_clock_speed_impl(const PortHandle& port) -> std::uint32_t {
    const auto kernel_hz = port.config().peripheral_clock_hz;
    if (kernel_hz == 0u) {
        return 0u;
    }
    if constexpr (PortHandle::br_field.valid) {
        const auto encoding = rt::read_field(PortHandle::br_field);
        if (encoding.is_err()) {
            return 0u;
        }
        constexpr auto& divisors = PortHandle::semantic_traits::kBaudPrescalerDivisors;
        if constexpr (divisors.size() == 0u) {
            return 0u;
        } else {
            const auto idx = encoding.unwrap();
            if (idx >= divisors.size()) {
                return 0u;
            }
            const auto divisor = divisors[idx];
            return divisor == 0u ? 0u : kernel_hz / divisor;
        }
    } else if constexpr (PortHandle::scbr_field.valid) {
        const auto scbr = rt::read_field(PortHandle::scbr_field);
        if (scbr.is_err()) {
            return 0u;
        }
        const auto divisor = scbr.unwrap();
        return divisor == 0u ? 0u : kernel_hz / divisor;
    } else {
        return 0u;
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_clock_speed_impl(const PortHandle& port, std::uint32_t hz)
    -> core::Result<void, core::ErrorCode> {
    const auto kernel_hz = port.config().peripheral_clock_hz;
    if (kernel_hz == 0u || hz == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    std::uint32_t realised = 0u;
    switch (rt::spi_schema_for<PortHandle>()) {
        case rt::SpiSchema::st_spi2s1_v3_3_cube:
        case rt::SpiSchema::st_spi2s1_v2_2_cube:
            realised = compute_realised_clock_st<PortHandle>(kernel_hz, hz);
            break;
        case rt::SpiSchema::microchip_spi_zm:
            realised = compute_realised_clock_microchip<PortHandle>(kernel_hz, hz);
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    const auto diff = realised > hz ? realised - hz : hz - realised;
    if (diff * 100u > hz * 5u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if constexpr (PortHandle::br_field.valid) {
        const auto br = st_spi_prescaler_bits(kernel_hz, hz);
        if (br.is_err()) {
            return core::Err(core::ErrorCode{br.unwrap_err()});
        }
        return rt::modify_field(PortHandle::br_field, br.unwrap());
    }
    if constexpr (PortHandle::scbr_field.valid) {
        const auto scbr = microchip_spi_scbr(kernel_hz, hz);
        if (scbr.is_err()) {
            return core::Err(core::ErrorCode{scbr.unwrap_err()});
        }
        return rt::modify_field(PortHandle::scbr_field, scbr.unwrap());
    }

    return core::Err(core::ErrorCode::NotSupported);
}

// ---------- STM32 synthetic field accessors (CR1 / CR2 / SR positions) ----------

template <typename PortHandle>
[[nodiscard]] constexpr auto st_crcen_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr1_reg, 13u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_crcnext_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr1_reg, 12u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_bidimode_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr1_reg, 15u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_bidioe_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr1_reg, 14u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_ssoe_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr2_reg, 2u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_nssp_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr2_reg, 3u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_frf_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr2_reg, 4u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_errie_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr2_reg, 5u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_rxneie_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr2_reg, 6u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_txeie_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::cr2_reg, 7u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_modf_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::sr_reg, 5u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_crcerr_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::sr_reg, 4u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_fre_field() -> rt::FieldRef {
    return make_synthetic_field(PortHandle::sr_reg, 8u, 1u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_crcpr_reg() -> rt::RegisterRef {
    // CRCPR is at base+0x10 on every STM32 SPI variant.
    return make_synthetic_register(PortHandle::cr1_reg, 0x10u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto st_rxcrcr_reg() -> rt::RegisterRef {
    // RXCRCR is at base+0x14.
    return make_synthetic_register(PortHandle::cr1_reg, 0x14u);
}

// ---------- Phase 2: Frame format / CRC / bidirectional / NSS ----------

template <typename PortHandle>
[[nodiscard]] auto set_frame_format_impl(const PortHandle&, FrameFormat fmt)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kSupportsTi = PortHandle::semantic_traits::kSupportsTiFrame;
    constexpr auto kSupportsMot = PortHandle::semantic_traits::kSupportsMotorolaFrame;
    if (fmt == FrameFormat::TI) {
        if constexpr (!kSupportsTi) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            constexpr auto frf = st_frf_field<PortHandle>();
            if (!frf.valid) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return rt::modify_field(frf, 1u);
        }
    } else {
        if constexpr (!kSupportsMot) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            constexpr auto frf = st_frf_field<PortHandle>();
            if (frf.valid) {
                return rt::modify_field(frf, 0u);
            }
            // SAM / Microchip SPI is always Motorola — nothing to set.
            return core::Ok();
        }
    }
}

template <typename PortHandle>
[[nodiscard]] auto enable_crc_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::semantic_traits::kSupportsCrc) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto crcen = st_crcen_field<PortHandle>();
        if (!crcen.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(crcen, enable ? 1u : 0u);
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_crc_polynomial_impl(const PortHandle&, std::uint16_t poly)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::semantic_traits::kSupportsCrc) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto crcpr = st_crcpr_reg<PortHandle>();
        if (!crcpr.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::write_register(crcpr, static_cast<std::uint32_t>(poly));
    }
}

template <typename PortHandle>
[[nodiscard]] auto read_crc_impl(const PortHandle&) -> std::uint16_t {
    if constexpr (!PortHandle::semantic_traits::kSupportsCrc) {
        return 0u;
    } else {
        constexpr auto rxcrcr = st_rxcrcr_reg<PortHandle>();
        if (!rxcrcr.valid) {
            return 0u;
        }
        const auto v = rt::read_register(rxcrcr);
        return v.is_ok() ? static_cast<std::uint16_t>(v.unwrap() & 0xFFFFu) : 0u;
    }
}

template <typename PortHandle>
[[nodiscard]] auto crc_error_impl(const PortHandle&) -> bool {
    if constexpr (!PortHandle::semantic_traits::kSupportsCrc) {
        return false;
    } else {
        return read_field_bool(st_crcerr_field<PortHandle>());
    }
}

template <typename PortHandle>
[[nodiscard]] auto clear_crc_error_impl(const PortHandle&)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::semantic_traits::kSupportsCrc) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto crcerr = st_crcerr_field<PortHandle>();
        if (!crcerr.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        // CRCERR is rc_w0 on STM32 — write 0 to clear.
        return rt::modify_field(crcerr, 0u);
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_bidirectional_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::semantic_traits::kSupportsBidirectional3Wire) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto bidimode = st_bidimode_field<PortHandle>();
        if (!bidimode.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(bidimode, enable ? 1u : 0u);
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_bidirectional_direction_impl(const PortHandle&, BiDir dir)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::semantic_traits::kSupportsBidirectional3Wire) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto bidioe = st_bidioe_field<PortHandle>();
        if (!bidioe.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(bidioe, dir == BiDir::Transmit ? 1u : 0u);
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_nss_management_impl(const PortHandle&, NssManagement mode)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::semantic_traits::kSupportsNssHwManagement) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto ssm = PortHandle::ssm_field;
        constexpr auto ssoe = st_ssoe_field<PortHandle>();
        if (!ssm.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        switch (mode) {
            case NssManagement::Software: {
                static_cast<void>(rt::modify_field(ssm, 1u));
                if constexpr (PortHandle::ssi_field.valid) {
                    static_cast<void>(rt::modify_field(PortHandle::ssi_field, 1u));
                }
                if (ssoe.valid) {
                    static_cast<void>(rt::modify_field(ssoe, 0u));
                }
                return core::Ok();
            }
            case NssManagement::HardwareInput: {
                static_cast<void>(rt::modify_field(ssm, 0u));
                if (ssoe.valid) {
                    static_cast<void>(rt::modify_field(ssoe, 0u));
                }
                return core::Ok();
            }
            case NssManagement::HardwareOutput: {
                static_cast<void>(rt::modify_field(ssm, 0u));
                if (!ssoe.valid) {
                    return core::Err(core::ErrorCode::NotSupported);
                }
                return rt::modify_field(ssoe, 1u);
            }
        }
        return core::Err(core::ErrorCode::InvalidParameter);
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_nss_pulse_per_transfer_impl(const PortHandle&, bool enable)
    -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::semantic_traits::kSupportsNssHwManagement) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        constexpr auto nssp = st_nssp_field<PortHandle>();
        if (!nssp.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return rt::modify_field(nssp, enable ? 1u : 0u);
    }
}

// ---------- Phase 3: SAM-style per-CS timing ----------

template <typename PortHandle>
[[nodiscard]] auto set_cs_decode_mode_impl(const PortHandle&, bool decoded)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto pcsdec = PortHandle::pcsdec_field;
    if constexpr (!pcsdec.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        return rt::modify_field(pcsdec, decoded ? 1u : 0u);
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_cs_delay_between_consecutive_impl(const PortHandle&,
                                                         std::uint16_t cycles)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto dlybct = PortHandle::dlybct_field;
    if constexpr (!dlybct.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        return rt::modify_field(dlybct, static_cast<std::uint32_t>(cycles));
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_cs_delay_clock_to_active_impl(const PortHandle&, std::uint16_t cycles)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto dlybs = PortHandle::dlybs_field;
    if constexpr (!dlybs.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        return rt::modify_field(dlybs, static_cast<std::uint32_t>(cycles));
    }
}

template <typename PortHandle>
[[nodiscard]] auto set_cs_delay_active_to_clock_impl(const PortHandle&, std::uint16_t cycles)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto dlybcs = PortHandle::dlybcs_field;
    if constexpr (!dlybcs.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    } else {
        return rt::modify_field(dlybcs, static_cast<std::uint32_t>(cycles));
    }
}

// ---------- Phase 4: Status flag clears + interrupt enables ----------

template <typename PortHandle>
[[nodiscard]] auto clear_mode_fault_impl(const PortHandle&)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto modf = st_modf_field<PortHandle>();
    if (!modf.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    // STM32 MODF clear sequence: read SR then write CR1 to release MODF.
    if constexpr (PortHandle::sr_reg.valid && PortHandle::cr1_reg.valid) {
        const auto sr_value = rt::read_register(PortHandle::sr_reg);
        if (sr_value.is_err()) {
            return core::Err(core::ErrorCode{sr_value.unwrap_err()});
        }
        const auto cr1_value = rt::read_register(PortHandle::cr1_reg);
        if (cr1_value.is_err()) {
            return core::Err(core::ErrorCode{cr1_value.unwrap_err()});
        }
        return rt::write_register(PortHandle::cr1_reg, cr1_value.unwrap());
    }
    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
[[nodiscard]] auto set_interrupt_enable_impl(const PortHandle&, InterruptKind kind, bool enable)
    -> core::Result<void, core::ErrorCode> {
    const auto value = enable ? 1u : 0u;

    switch (rt::spi_schema_for<PortHandle>()) {
        case rt::SpiSchema::st_spi2s1_v3_3_cube:
        case rt::SpiSchema::st_spi2s1_v2_2_cube: {
            rt::FieldRef field = rt::kInvalidFieldRef;
            switch (kind) {
                case InterruptKind::Txe:
                    field = st_txeie_field<PortHandle>();
                    break;
                case InterruptKind::Rxne:
                    field = st_rxneie_field<PortHandle>();
                    break;
                case InterruptKind::Error:
                case InterruptKind::ModeFault:
                case InterruptKind::FrameError:
                    // STM32 muxes MODF / OVR / FRE / CRCERR through ERRIE.
                    field = st_errie_field<PortHandle>();
                    break;
                case InterruptKind::CrcError:
                    if constexpr (!PortHandle::semantic_traits::kSupportsCrc) {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                    field = st_errie_field<PortHandle>();
                    break;
            }
            if (!field.valid) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return rt::modify_field(field, value);
        }
        case rt::SpiSchema::microchip_spi_zm: {
            // SAM SPI uses IER/IDR write-to-set (no read-modify-write).
            constexpr auto ier_reg = make_synthetic_register(PortHandle::cr_reg, 0x14u);
            constexpr auto idr_reg = make_synthetic_register(PortHandle::cr_reg, 0x18u);
            if (!ier_reg.valid || !idr_reg.valid) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            std::uint32_t mask = 0u;
            switch (kind) {
                case InterruptKind::Txe:
                    mask = (1u << 1u);  // TDRE
                    break;
                case InterruptKind::Rxne:
                    mask = (1u << 0u);  // RDRF
                    break;
                case InterruptKind::ModeFault:
                    mask = (1u << 2u);  // MODF
                    break;
                case InterruptKind::Error:
                    mask = (1u << 3u);  // OVRES
                    break;
                case InterruptKind::CrcError:
                case InterruptKind::FrameError:
                    return core::Err(core::ErrorCode::NotSupported);
            }
            return rt::write_register(enable ? ier_reg : idr_reg, mask);
        }
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }
}

}  // namespace alloy::hal::spi::detail
