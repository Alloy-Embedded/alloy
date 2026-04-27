#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "hal/connect/connector.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/spi/detail/backend.hpp"
#include "hal/spi/types.hpp"

namespace alloy::hal::spi {

using Config = SpiConfig;
using Mode = SpiMode;
using BitOrder = SpiBitOrder;
using DataSize = SpiDataSize;
// FrameFormat / BiDir / NssManagement / InterruptKind are defined in
// detail/backend.hpp so the backend can use them before this re-export.

template <device::PeripheralId PeripheralIdValue, typename... Bindings>
using route = connection::connector<PeripheralIdValue, Bindings...>;

namespace detail {

namespace runtime = alloy::hal::detail::runtime;

}  // namespace detail

template <typename Connector>
requires(Connector::valid) class port_handle {
   public:
    using connector_type = Connector;
    using runtime_peripheral_id = device::PeripheralId;

    static constexpr auto package_name = Connector::package_name;
    static constexpr auto peripheral_name = Connector::peripheral_type::name;
    static constexpr auto peripheral_id = Connector::peripheral_id;
    using peripheral_traits = device::PeripheralInstanceTraits<peripheral_id>;
    using semantic_traits = device::SpiSemanticTraits<peripheral_id>;
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

    /// Returns the NVIC IRQ line numbers for this peripheral.
    /// The span is empty when the descriptor publishes no IRQ for this SPI.
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

    // ------------------------------------------------------------------
    // Phase 1: Variable data size / clock speed / kernel clock
    // ------------------------------------------------------------------

    /// Set frame data size at runtime. Validates against the descriptor's
    /// kSupportedFrameSizes and the published kDsField/kDffField/kBitsField
    /// width. Returns InvalidParameter when the request cannot be realised.
    /// Compile-time-constant calls outside [4, 16] static_assert.
    [[nodiscard]] auto set_data_size(std::uint8_t bits) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_data_size_impl(*this, bits);
    }

    template <std::uint8_t Bits>
    [[nodiscard]] auto set_data_size_static() const
        -> core::Result<void, core::ErrorCode> {
        static_assert(Bits >= 4u && Bits <= 16u,
                      "SPI data size must be in the range [4, 16] bits");
        return detail::set_data_size_impl(*this, Bits);
    }

    /// Set the SPI bit clock at runtime. Returns InvalidParameter when the
    /// realised rate falls outside ±5 % of the requested rate.
    [[nodiscard]] auto set_clock_speed(std::uint32_t hz) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_clock_speed_impl(*this, hz);
    }

    /// Returns the rate produced by the current BR / SCBR field encoding,
    /// or 0 when the descriptor does not publish a baud divider field.
    [[nodiscard]] auto realised_clock_speed() const -> std::uint32_t {
        return detail::realised_clock_speed_impl(*this);
    }

    /// Returns the kernel-clock frequency in Hz (as configured via SpiConfig).
    [[nodiscard]] auto kernel_clock_hz() const noexcept -> std::uint32_t {
        return config_.peripheral_clock_hz;
    }

    // ------------------------------------------------------------------
    // Phase 2: Frame format / CRC / bidirectional 3-wire / NSS management
    // ------------------------------------------------------------------

    [[nodiscard]] auto set_frame_format(FrameFormat fmt) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_frame_format_impl(*this, fmt);
    }

    [[nodiscard]] auto enable_crc(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::enable_crc_impl(*this, enable);
    }

    [[nodiscard]] auto set_crc_polynomial(std::uint16_t poly) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_crc_polynomial_impl(*this, poly);
    }

    [[nodiscard]] auto read_crc() const -> std::uint16_t {
        return detail::read_crc_impl(*this);
    }

    [[nodiscard]] auto crc_error() const -> bool {
        return detail::crc_error_impl(*this);
    }

    [[nodiscard]] auto clear_crc_error() const
        -> core::Result<void, core::ErrorCode> {
        return detail::clear_crc_error_impl(*this);
    }

    [[nodiscard]] auto set_bidirectional(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_bidirectional_impl(*this, enable);
    }

    [[nodiscard]] auto set_bidirectional_direction(BiDir dir) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_bidirectional_direction_impl(*this, dir);
    }

    [[nodiscard]] auto set_nss_management(NssManagement mode) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_nss_management_impl(*this, mode);
    }

    [[nodiscard]] auto set_nss_pulse_per_transfer(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_nss_pulse_per_transfer_impl(*this, enable);
    }

    // ------------------------------------------------------------------
    // Phase 3: SAM-style per-CS timing
    // ------------------------------------------------------------------

    [[nodiscard]] auto set_cs_decode_mode(bool decoded) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_cs_decode_mode_impl(*this, decoded);
    }

    [[nodiscard]] auto set_cs_delay_between_consecutive(std::uint16_t cycles) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_cs_delay_between_consecutive_impl(*this, cycles);
    }

    [[nodiscard]] auto set_cs_delay_clock_to_active(std::uint16_t cycles) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_cs_delay_clock_to_active_impl(*this, cycles);
    }

    [[nodiscard]] auto set_cs_delay_active_to_clock(std::uint16_t cycles) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_cs_delay_active_to_clock_impl(*this, cycles);
    }

    // ------------------------------------------------------------------
    // Phase 4: Status flags, interrupts
    // ------------------------------------------------------------------

    /// True when the TX data register is empty (TXE / TDRE).
    [[nodiscard]] auto tx_register_empty() const -> bool {
        switch (schema) {
            case detail::runtime::SpiSchema::st_spi2s1_v3_3_cube:
            case detail::runtime::SpiSchema::st_spi2s1_v2_2_cube:
                return detail::read_field_bool(txe_field);
            case detail::runtime::SpiSchema::microchip_spi_zm:
                return detail::read_field_bool(tdre_field);
            default:
                return false;
        }
    }

    /// True when the RX data register has data (RXNE / RDRF).
    [[nodiscard]] auto rx_register_not_empty() const -> bool {
        switch (schema) {
            case detail::runtime::SpiSchema::st_spi2s1_v3_3_cube:
            case detail::runtime::SpiSchema::st_spi2s1_v2_2_cube:
                return detail::read_field_bool(rxne_field);
            case detail::runtime::SpiSchema::microchip_spi_zm:
                return detail::read_field_bool(rdrf_field);
            default:
                return false;
        }
    }

    /// True when the peripheral is busy transmitting / receiving (BSY / !TXEMPTY).
    [[nodiscard]] auto busy() const -> bool { return detail::spi_is_busy(*this); }

    /// True when a mode fault has been detected (STM32 SR.MODF).
    [[nodiscard]] auto mode_fault() const -> bool {
        return detail::read_field_bool(detail::st_modf_field<port_handle>());
    }

    /// Clears the mode-fault flag (STM32 read-SR-then-write-CR1 sequence).
    [[nodiscard]] auto clear_mode_fault() const
        -> core::Result<void, core::ErrorCode> {
        return detail::clear_mode_fault_impl(*this);
    }

    /// True when a frame-format error has been detected (STM32 SR.FRE).
    [[nodiscard]] auto frame_format_error() const -> bool {
        return detail::read_field_bool(detail::st_fre_field<port_handle>());
    }

    /// Enable a typed interrupt. Returns NotSupported for kinds that the
    /// peripheral doesn't expose (e.g. CrcError on a non-CRC SPI).
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_interrupt_enable_impl(*this, kind, true);
    }

    /// Disable a typed interrupt.
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        return detail::set_interrupt_enable_impl(*this, kind, false);
    }

    template <typename DmaChannel>
    [[nodiscard]] auto configure_tx_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX);
        return channel.configure();
    }

    template <typename DmaChannel>
    [[nodiscard]] auto configure_rx_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX);
        return channel.configure();
    }

    [[nodiscard]] static constexpr auto tx_data_register_address() -> std::uintptr_t {
        if constexpr (tdr_reg.valid) {
            return tdr_reg.base_address + tdr_reg.offset_bytes;
        } else if constexpr (dr_reg.valid) {
            return dr_reg.base_address + dr_reg.offset_bytes;
        } else {
            return 0u;
        }
    }

    [[nodiscard]] static constexpr auto rx_data_register_address() -> std::uintptr_t {
        if constexpr (rdr_reg.valid) {
            return rdr_reg.base_address + rdr_reg.offset_bytes;
        } else if constexpr (dr_reg.valid) {
            return dr_reg.base_address + dr_reg.offset_bytes;
        } else {
            return 0u;
        }
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(port_handle<Connector>::valid,
                  "Requested SPI route has no valid descriptor-backed route for the selected "
                  "device/package. If you used ergonomic role aliases such as hal::sck<Pin>, "
                  "hal::miso<Pin>, or hal::mosi<Pin>, retry with an explicit expert signal alias. "
                  "Canonical forms remain supported.");
    return port_handle<Connector>{config};
}

template <typename Connector>
class shared_device;

template <typename Connector>
class shared_bus {
   public:
    using connector_type = Connector;
    using config_type = Config;

    static constexpr bool valid = port_handle<Connector>::valid;

    constexpr explicit shared_bus(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return open<Connector>(config_).configure();
    }

    [[nodiscard]] constexpr auto device(Config config) const -> shared_device<Connector> {
        return shared_device<Connector>{config};
    }

   private:
    Config config_{};
};

template <typename Connector>
class shared_device {
   public:
    using connector_type = Connector;
    using config_type = Config;

    static constexpr bool valid = port_handle<Connector>::valid;

    constexpr explicit shared_device(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return open<Connector>(config_).configure();
    }

    [[nodiscard]] auto transfer(std::span<const std::uint8_t> tx_buffer,
                                std::span<std::uint8_t> rx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        auto port = open<Connector>(config_);
        const auto configured = port.configure();
        if (!configured.is_ok()) {
            return configured;
        }
        return port.transfer(tx_buffer, rx_buffer);
    }

    [[nodiscard]] auto transmit(std::span<const std::uint8_t> tx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        auto port = open<Connector>(config_);
        const auto configured = port.configure();
        if (!configured.is_ok()) {
            return configured;
        }
        return port.transmit(tx_buffer);
    }

    [[nodiscard]] auto receive(std::span<std::uint8_t> rx_buffer) const
        -> core::Result<void, core::ErrorCode> {
        auto port = open<Connector>(config_);
        const auto configured = port.configure();
        if (!configured.is_ok()) {
            return configured;
        }
        return port.receive(rx_buffer);
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open_shared_bus(Config config = {}) -> shared_bus<Connector> {
    static_assert(shared_bus<Connector>::valid,
                  "Requested SPI route has no valid descriptor-backed route for the selected "
                  "device/package. If you used ergonomic role aliases such as hal::sck<Pin>, "
                  "hal::miso<Pin>, or hal::mosi<Pin>, retry with an explicit expert signal alias. "
                  "Canonical forms remain supported.");
    return shared_bus<Connector>{config};
}

}  // namespace alloy::hal::spi
