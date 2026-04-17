#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "hal/spi/detail/backend.hpp"
#include "hal/spi/types.hpp"

namespace alloy::hal::spi {

using Config = SpiConfig;
using Mode = SpiMode;
using BitOrder = SpiBitOrder;
using DataSize = SpiDataSize;

namespace detail {

namespace runtime = alloy::hal::detail::runtime;

}  // namespace detail

template <typename Connector>
requires(Connector::valid) class port_handle {
   public:
    using connector_type = Connector;
    using runtime_peripheral_id = device::runtime::PeripheralId;

    static constexpr auto package_name = Connector::package_name;
    static constexpr auto peripheral_name = Connector::peripheral_type::name;
    static constexpr auto peripheral_id = Connector::peripheral_id;
    using peripheral_traits = device::runtime::PeripheralInstanceTraits<peripheral_id>;
    using semantic_traits = device::runtime::SpiSemanticTraits<peripheral_id>;
    static constexpr auto schema =
        peripheral_id == runtime_peripheral_id::none
            ? detail::runtime::SpiSchema::unknown
            : detail::runtime::to_spi_schema(peripheral_traits::kSchemaId);
    static constexpr bool valid =
        Connector::valid && peripheral_id != runtime_peripheral_id::none &&
        peripheral_traits::kPresent && semantic_traits::kPresent &&
        schema != detail::runtime::SpiSchema::unknown;

    static constexpr auto cr1_reg = semantic_traits::kCr1Register;
    static constexpr auto cr2_reg = semantic_traits::kCr2Register;
    static constexpr auto sr_reg = semantic_traits::kSrRegister;
    static constexpr auto dr_reg = semantic_traits::kDrRegister;
    static constexpr auto cr_reg = semantic_traits::kCrRegister;
    static constexpr auto mr_reg = semantic_traits::kMrRegister;
    static constexpr auto csr_reg = semantic_traits::kCsrRegister;
    static constexpr auto tdr_reg = semantic_traits::kTdrRegister;
    static constexpr auto rdr_reg = semantic_traits::kRdrRegister;

    static constexpr auto cpha_field = semantic_traits::kCphaField;
    static constexpr auto cpol_field = semantic_traits::kCpolField;
    static constexpr auto mstr_field = semantic_traits::kMstrField;
    static constexpr auto br_field = semantic_traits::kBrField;
    static constexpr auto spe_field = semantic_traits::kSpeField;
    static constexpr auto lsbfirst_field = semantic_traits::kLsbfirstField;
    static constexpr auto ssi_field = semantic_traits::kSsiField;
    static constexpr auto ssm_field = semantic_traits::kSsmField;
    static constexpr auto dff_field = semantic_traits::kDffField;
    static constexpr auto ds_field = semantic_traits::kDsField;
    static constexpr auto frxth_field = semantic_traits::kFrxthField;
    static constexpr auto txe_field = semantic_traits::kTxeField;
    static constexpr auto rxne_field = semantic_traits::kRxneField;
    static constexpr auto bsy_field = semantic_traits::kBsyField;
    static constexpr auto dr_data_field = semantic_traits::kDrDataField;
    static constexpr auto spien_field = semantic_traits::kSpienField;
    static constexpr auto spidis_field = semantic_traits::kSpidisField;
    static constexpr auto swrst_field = semantic_traits::kSwrstField;
    static constexpr auto ps_field = semantic_traits::kPsField;
    static constexpr auto pcsdec_field = semantic_traits::kPcsdecField;
    static constexpr auto modfdis_field = semantic_traits::kModfdisField;
    static constexpr auto pcs_field = semantic_traits::kPcsField;
    static constexpr auto dlybcs_field = semantic_traits::kDlybcsField;
    static constexpr auto ncpha_field = semantic_traits::kNcphaField;
    static constexpr auto bits_field = semantic_traits::kBitsField;
    static constexpr auto scbr_field = semantic_traits::kScbrField;
    static constexpr auto dlybs_field = semantic_traits::kDlybsField;
    static constexpr auto dlybct_field = semantic_traits::kDlybctField;
    static constexpr auto tdre_field = semantic_traits::kTdreField;
    static constexpr auto rdrf_field = semantic_traits::kRdrfField;
    static constexpr auto txempty_field = semantic_traits::kTxemptyField;
    static constexpr auto td_field = semantic_traits::kTdField;
    static constexpr auto tdr_pcs_field = semantic_traits::kTdrPcsField;
    static constexpr auto rd_field = semantic_traits::kRdField;

    constexpr explicit port_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() { return Connector::requirements(); }

    [[nodiscard]] static consteval auto operations() { return Connector::operations(); }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return peripheral_traits::kBaseAddress;
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_spi(*this);
    }

    [[nodiscard]] auto transfer(std::span<const std::uint8_t> tx_buffer,
                                std::span<std::uint8_t> rx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::transfer_spi(*this, tx_buffer, rx_buffer);
    }

    [[nodiscard]] auto transmit(std::span<const std::uint8_t> tx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::transmit_spi(*this, tx_buffer);
    }

    [[nodiscard]] auto receive(std::span<std::uint8_t> rx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::receive_spi(*this, rx_buffer);
    }

    [[nodiscard]] auto is_busy() const -> bool { return detail::spi_is_busy(*this); }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(port_handle<Connector>::valid,
                  "Requested SPI connector has no valid descriptor-backed route for the selected "
                  "device/package.");
    return port_handle<Connector>{config};
}

}  // namespace alloy::hal::spi
