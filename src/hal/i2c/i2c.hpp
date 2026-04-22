#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/i2c/detail/backend.hpp"
#include "hal/i2c/types.hpp"

namespace alloy::hal::i2c {

using Config = I2cConfig;
using Addressing = I2cAddressing;
using Speed = I2cSpeed;

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
    using semantic_traits = device::I2cSemanticTraits<peripheral_id>;
    static constexpr auto schema =
        semantic_traits::kPresent ? detail::runtime::to_i2c_schema(semantic_traits::kSchemaId)
                                  : detail::runtime::to_i2c_schema(peripheral_traits::kSchemaId);
    static constexpr bool valid =
        Connector::valid && peripheral_id != runtime_peripheral_id::none &&
        peripheral_traits::kPresent && semantic_traits::kPresent &&
        schema != detail::runtime::I2cSchema::unknown;

    static constexpr auto cr1_reg = semantic_traits::kCr1Register;
    static constexpr auto cr2_reg = semantic_traits::kCr2Register;
    static constexpr auto ccr_reg = semantic_traits::kCcrRegister;
    static constexpr auto trise_reg = semantic_traits::kTriseRegister;
    static constexpr auto sr1_reg = semantic_traits::kSr1Register;
    static constexpr auto sr2_reg = semantic_traits::kSr2Register;
    static constexpr auto dr_reg = semantic_traits::kDrRegister;
    static constexpr auto icr_reg = semantic_traits::kIcrRegister;
    static constexpr auto cr_reg = semantic_traits::kCrRegister;
    static constexpr auto mmr_reg = semantic_traits::kMmrRegister;
    static constexpr auto iadr_reg = semantic_traits::kIadrRegister;
    static constexpr auto cwgr_reg = semantic_traits::kCwgrRegister;
    static constexpr auto sr_reg = semantic_traits::kSrRegister;
    static constexpr auto rhr_reg = semantic_traits::kRhrRegister;
    static constexpr auto thr_reg = semantic_traits::kThrRegister;

    static constexpr auto pe_field = semantic_traits::kPeField;
    static constexpr auto ack_field = semantic_traits::kAckField;
    static constexpr auto start_field = semantic_traits::kStartField;
    static constexpr auto stop_field = semantic_traits::kStopField;
    static constexpr auto freq_field = semantic_traits::kFreqField;
    static constexpr auto ccr_field = semantic_traits::kCcrField;
    static constexpr auto fs_field = semantic_traits::kFsField;
    static constexpr auto duty_field = semantic_traits::kDutyField;
    static constexpr auto trise_field = semantic_traits::kTriseField;
    static constexpr auto sb_field = semantic_traits::kSbField;
    static constexpr auto addr_field = semantic_traits::kAddrField;
    static constexpr auto txe_field = semantic_traits::kTxeField;
    static constexpr auto rxne_field = semantic_traits::kRxneField;
    static constexpr auto btf_field = semantic_traits::kBtfField;
    static constexpr auto af_field = semantic_traits::kAfField;
    static constexpr auto berr_field = semantic_traits::kBerrField;
    static constexpr auto arlo_field = semantic_traits::kArloField;
    static constexpr auto busy_field = semantic_traits::kBusyField;
    static constexpr auto dr_data_field = semantic_traits::kDrDataField;
    static constexpr auto sadd_field = semantic_traits::kSaddField;
    static constexpr auto rd_wrn_field = semantic_traits::kRdWrnField;
    static constexpr auto nbytes_field = semantic_traits::kNbytesField;
    static constexpr auto autoend_field = semantic_traits::kAutoendField;
    static constexpr auto txis_field = semantic_traits::kTxisField;
    static constexpr auto tc_field = semantic_traits::kTcField;
    static constexpr auto stopf_field = semantic_traits::kStopfField;
    static constexpr auto txdata_field = semantic_traits::kTxdataField;
    static constexpr auto rxdata_field = semantic_traits::kRxdataField;
    static constexpr auto nackf_field = semantic_traits::kNackfField;
    static constexpr auto berr_isr_field = semantic_traits::kBerrIsrField;
    static constexpr auto arlo_isr_field = semantic_traits::kArloIsrField;
    static constexpr auto stopcf_field = semantic_traits::kStopcfField;
    static constexpr auto nackcf_field = semantic_traits::kNackcfField;
    static constexpr auto berrcf_field = semantic_traits::kBerrcfField;
    static constexpr auto arlocf_field = semantic_traits::kArlocfField;
    static constexpr auto msen_field = semantic_traits::kMsenField;
    static constexpr auto msdis_field = semantic_traits::kMsdisField;
    static constexpr auto svdis_field = semantic_traits::kSvdisField;
    static constexpr auto swrst_field = semantic_traits::kSwrstField;
    static constexpr auto iadrsz_field = semantic_traits::kIadrszField;
    static constexpr auto mread_field = semantic_traits::kMreadField;
    static constexpr auto dadr_field = semantic_traits::kDadrField;
    static constexpr auto iadr_field = semantic_traits::kIadrField;
    static constexpr auto cldiv_field = semantic_traits::kCldivField;
    static constexpr auto chdiv_field = semantic_traits::kChdivField;
    static constexpr auto ckdiv_field = semantic_traits::kCkdivField;
    static constexpr auto hold_field = semantic_traits::kHoldField;
    static constexpr auto txcomp_field = semantic_traits::kTxcompField;
    static constexpr auto rxrdy_field = semantic_traits::kRxrdyField;
    static constexpr auto txrdy_field = semantic_traits::kTxrdyField;
    static constexpr auto nack_field = semantic_traits::kNackField;
    static constexpr auto arblst_field = semantic_traits::kArblstField;

    constexpr explicit port_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() { return Connector::requirements(); }

    [[nodiscard]] static consteval auto operations() { return Connector::operations(); }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return peripheral_traits::kBaseAddress;
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_i2c(*this);
    }

    [[nodiscard]] auto read(std::uint16_t address, std::span<std::uint8_t> buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::read_i2c(*this, address, buffer);
    }

    [[nodiscard]] auto write(std::uint16_t address,
                             std::span<const std::uint8_t> buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::write_i2c(*this, address, buffer);
    }

    [[nodiscard]] auto write_read(std::uint16_t address,
                                  std::span<const std::uint8_t> write_buffer,
                                  std::span<std::uint8_t> read_buffer) const
        -> core::Result<void, core::ErrorCode> {
        return detail::write_read_i2c(*this, address, write_buffer, read_buffer);
    }

    [[nodiscard]] auto scan_bus(std::span<std::uint8_t> found_devices) const
        -> core::Result<std::size_t, core::ErrorCode> {
        return detail::scan_i2c_bus(*this, found_devices);
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
        if constexpr (thr_reg.valid) {
            return thr_reg.base_address + thr_reg.offset_bytes;
        } else if constexpr (dr_reg.valid) {
            return dr_reg.base_address + dr_reg.offset_bytes;
        } else {
            return 0u;
        }
    }

    [[nodiscard]] static constexpr auto rx_data_register_address() -> std::uintptr_t {
        if constexpr (rhr_reg.valid) {
            return rhr_reg.base_address + rhr_reg.offset_bytes;
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
                  "Requested I2C connector has no valid descriptor-backed route for the selected "
                  "device/package.");
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

    [[nodiscard]] auto read(std::uint16_t address, std::span<std::uint8_t> buffer) const
        -> core::Result<void, core::ErrorCode> {
        auto port = open<Connector>(config_);
        const auto configured = port.configure();
        if (!configured.is_ok()) {
            return configured;
        }
        return port.read(address, buffer);
    }

    [[nodiscard]] auto write(std::uint16_t address,
                             std::span<const std::uint8_t> buffer) const
        -> core::Result<void, core::ErrorCode> {
        auto port = open<Connector>(config_);
        const auto configured = port.configure();
        if (!configured.is_ok()) {
            return configured;
        }
        return port.write(address, buffer);
    }

    [[nodiscard]] auto write_read(std::uint16_t address,
                                  std::span<const std::uint8_t> write_buffer,
                                  std::span<std::uint8_t> read_buffer) const
        -> core::Result<void, core::ErrorCode> {
        auto port = open<Connector>(config_);
        const auto configured = port.configure();
        if (!configured.is_ok()) {
            return configured;
        }
        return port.write_read(address, write_buffer, read_buffer);
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open_shared_bus(Config config = {}) -> shared_bus<Connector> {
    static_assert(shared_bus<Connector>::valid,
                  "Requested I2C connector has no valid descriptor-backed route for the selected "
                  "device/package.");
    return shared_bus<Connector>{config};
}

}  // namespace alloy::hal::i2c
