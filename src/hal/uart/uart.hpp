#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "hal/claim.hpp"
#include "hal/connect.hpp"
#include "hal/types.hpp"
#include "hal/uart/detail/backend.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/descriptors.hpp"

namespace alloy::hal::uart {

using Config = UartConfig;

namespace detail {

namespace runtime = alloy::hal::detail::runtime;

}  // namespace detail

template <typename Connector>
requires(Connector::valid) class port_handle {
   public:
    using connector_type = Connector;
    using connector_claim = claim::connector_claim<Connector>;

    static constexpr auto package_name = Connector::package_name;
    static constexpr auto peripheral_name = Connector::peripheral_type::name;
    static constexpr auto peripheral = detail::runtime::find_peripheral_descriptor(peripheral_name);
    static constexpr auto schema =
        peripheral == nullptr
            ? detail::runtime::UartSchema::unknown
            : detail::runtime::uart_schema_from_id(
                  detail::runtime::as_string(peripheral->backend_schema_id));
    static constexpr bool valid =
        Connector::valid && peripheral != nullptr && schema != detail::runtime::UartSchema::unknown;

    static constexpr auto base = peripheral == nullptr ? std::uintptr_t{0u} : peripheral->base_address;

    static constexpr auto cr1_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "CR1");
    static constexpr auto cr2_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "CR2");
    static constexpr auto brr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "BRR");
    static constexpr auto isr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "ISR");
    static constexpr auto sr_reg = detail::runtime::find_register_ref(peripheral_name, base, "SR");
    static constexpr auto dr_reg = detail::runtime::find_register_ref(peripheral_name, base, "DR");
    static constexpr auto tdr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "TDR");
    static constexpr auto rdr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "RDR");

    static constexpr auto ue_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "UE");
    static constexpr auto re_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "RE");
    static constexpr auto te_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "TE");
    static constexpr auto pce_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "PCE");
    static constexpr auto ps_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "PS");
    static constexpr auto m_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "M");
    static constexpr auto m0_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "M0");
    static constexpr auto m1_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR1", "M1");
    static constexpr auto stop_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR2", "STOP");
    static constexpr auto txe_isr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "ISR", "TXE");
    static constexpr auto rxne_isr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "ISR", "RXNE");
    static constexpr auto tc_isr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "ISR", "TC");
    static constexpr auto tdr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "TDR", "TDR");
    static constexpr auto rdr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "RDR", "RDR");
    static constexpr auto txe_sr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "SR", "TXE");
    static constexpr auto rxne_sr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "SR", "RXNE");
    static constexpr auto tc_sr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "SR", "TC");
    static constexpr auto dr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "DR", "DR");

    static constexpr auto cr_reg = detail::runtime::find_register_ref(peripheral_name, base, "CR");
    static constexpr auto mr_reg = detail::runtime::find_register_ref(peripheral_name, base, "MR");
    static constexpr auto brgr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "BRGR");
    static constexpr auto thr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "THR");
    static constexpr auto rhr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "RHR");
    static constexpr auto rstrx_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR", "RSTRX");
    static constexpr auto rsttx_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR", "RSTTX");
    static constexpr auto rxdis_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR", "RXDIS");
    static constexpr auto txdis_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR", "TXDIS");
    static constexpr auto rststa_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR", "RSTSTA");
    static constexpr auto par_field =
        detail::runtime::find_field_ref(peripheral_name, base, "MR", "PAR");
    static constexpr auto chmode_field =
        detail::runtime::find_field_ref(peripheral_name, base, "MR", "CHMODE");
    static constexpr auto cd_field =
        detail::runtime::find_field_ref(peripheral_name, base, "BRGR", "CD");
    static constexpr auto rxen_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR", "RXEN");
    static constexpr auto txen_field =
        detail::runtime::find_field_ref(peripheral_name, base, "CR", "TXEN");
    static constexpr auto txrdy_field =
        detail::runtime::find_field_ref(peripheral_name, base, "SR", "TXRDY");
    static constexpr auto rxrdy_field =
        detail::runtime::find_field_ref(peripheral_name, base, "SR", "RXRDY");
    static constexpr auto txempty_field =
        detail::runtime::find_field_ref(peripheral_name, base, "SR", "TXEMPTY");
    static constexpr auto txchr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "THR", "TXCHR");
    static constexpr auto rxchr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "RHR", "RXCHR");

    static constexpr auto us_cr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "US_CR_LIN_MODE");
    static constexpr auto us_mr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "US_MR_SPI_MODE");
    static constexpr auto us_brgr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "US_BRGR");
    static constexpr auto us_csr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "US_CSR_LIN_MODE");
    static constexpr auto us_thr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "US_THR");
    static constexpr auto us_rhr_reg =
        detail::runtime::find_register_ref(peripheral_name, base, "US_RHR");
    static constexpr auto us_rstrx_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CR_LIN_MODE", "RSTRX");
    static constexpr auto us_rsttx_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CR_LIN_MODE", "RSTTX");
    static constexpr auto us_rxdis_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CR_LIN_MODE", "RXDIS");
    static constexpr auto us_txdis_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CR_LIN_MODE", "TXDIS");
    static constexpr auto us_rststa_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CR_LIN_MODE", "RSTSTA");
    static constexpr auto us_usart_mode_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_MR_SPI_MODE", "USART_MODE");
    static constexpr auto us_usclks_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_MR_SPI_MODE", "USCLKS");
    static constexpr auto us_chrl_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_MR_SPI_MODE", "CHRL");
    static constexpr auto us_cd_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_BRGR", "CD");
    static constexpr auto us_rxen_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CR_LIN_MODE", "RXEN");
    static constexpr auto us_txen_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CR_LIN_MODE", "TXEN");
    static constexpr auto us_txrdy_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CSR_LIN_MODE", "TXRDY");
    static constexpr auto us_rxrdy_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CSR_LIN_MODE", "RXRDY");
    static constexpr auto us_txempty_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_CSR_LIN_MODE", "TXEMPTY");
    static constexpr auto us_txchr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_THR", "TXCHR");
    static constexpr auto us_rxchr_field =
        detail::runtime::find_field_ref(peripheral_name, base, "US_RHR", "RXCHR");

    constexpr explicit port_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() { return Connector::requirements(); }

    [[nodiscard]] static consteval auto operations() { return Connector::operations(); }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return base;
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_uart(*this);
    }

    [[nodiscard]] auto write(std::span<const std::byte> buffer) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::write_uart(*this, buffer);
    }

    [[nodiscard]] auto write_byte(std::byte value) const -> core::Result<void, core::ErrorCode> {
        return detail::write_uart_byte(*this, value);
    }

    [[nodiscard]] auto read(std::span<std::byte> buffer) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::read_uart(*this, buffer);
    }

    [[nodiscard]] auto flush() const -> core::Result<void, core::ErrorCode> {
        return detail::flush_uart(*this);
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(port_handle<Connector>::valid,
                  "Requested UART connector has no valid descriptor-backed route for the selected "
                  "device/package.");
    return port_handle<Connector>{config};
}

}  // namespace alloy::hal::uart
