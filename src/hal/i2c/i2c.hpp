#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "hal/connect/connector.hpp"
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

// ---------------------------------------------------------------------------
// extend-i2c-coverage: new enumerations
// ---------------------------------------------------------------------------

enum class SpeedMode : std::uint8_t {
    Standard100kHz,
    Fast400kHz,
    FastPlus1MHz,  ///< Gated on kSupportsFastModePlus — NotSupported otherwise.
};

enum class DutyCycle : std::uint8_t {
    Duty2,    ///< T_low / T_high = 2   (default for STM32 fast mode)
    Duty169,  ///< T_low / T_high = 16/9 (STM32 CCR.DUTY=1)
};

enum class AddressingMode : std::uint8_t {
    Bits7,
    Bits10,  ///< Gated on kSupports10BitAddressing — NotSupported otherwise.
};

enum class SmbusRole : std::uint8_t { Host, Device };

/// Kernel clock source for the I2C peripheral. Raw encoding matches the
/// device's clock-selector field. Only meaningful when
/// `kKernelClockSelectorField.valid`. Returns NotSupported otherwise.
enum class KernelClockSource : std::uint8_t { Default = 0u };

enum class InterruptKind : std::uint8_t {
    Tx,
    Rx,
    Stop,
    Tc,
    AddrMatch,
    Nack,
    BusError,
    ArbitrationLoss,
    Overrun,
    PecError,
    Timeout,
    SmbAlert,
};

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

    // -----------------------------------------------------------------------
    // extend-i2c-coverage: speed / duty / addressing / clock-stretching
    // -----------------------------------------------------------------------

    /// Reconfigure clock speed by re-running configure() with the new speed.
    /// Returns NotSupported when the TIMINGR register is not published in the
    /// device database (no kTimingrRegister in current traits). Callers can
    /// use open(Config{speed, ...}).configure() as a workaround.
    [[nodiscard]] auto set_clock_speed(std::uint32_t /*hz*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Switch the bus to a predefined speed mode. Like set_clock_speed, requires
    /// the TIMINGR register in the device database (not yet published).
    [[nodiscard]] auto set_speed_mode(SpeedMode /*mode*/) const
        -> core::Result<void, core::ErrorCode> {
        // TIMINGR / CCR register refs not in current traits → NotSupported.
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set CCR duty-cycle mode (STM32 CCR.DUTY). Only meaningful in Fast mode;
    /// applies only when kDutyField.valid (STM32 I2C v1 CCR register).
    [[nodiscard]] auto set_duty_cycle(DutyCycle cycle) const
        -> core::Result<void, core::ErrorCode> {
        if constexpr (!duty_field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(duty_field,
                                                 cycle == DutyCycle::Duty169 ? 1u : 0u);
        }
    }

    /// Switch addressing mode. Returns NotSupported when 10-bit addressing is
    /// not supported by the peripheral or when no configuration field exists.
    [[nodiscard]] auto set_addressing_mode(AddressingMode mode) const
        -> core::Result<void, core::ErrorCode> {
        if constexpr (!semantic_traits::kSupports10BitAddressing) {
            if (mode == AddressingMode::Bits10) {
                return core::Err(core::ErrorCode::NotSupported);
            }
        }
        // No dedicated mode-configuration field in current traits.
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set own address (OAR1). Returns NotSupported — no OAR1 field in current
    /// device database.
    [[nodiscard]] auto set_own_address(std::uint16_t /*addr*/, AddressingMode /*mode*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set second own address (OAR2). Returns NotSupported — no OAR2 field in
    /// current device database.
    [[nodiscard]] auto set_dual_address(std::uint16_t /*addr2*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable or disable clock stretching (NOSTRETCH). Returns NotSupported —
    /// no NOSTRETCH field in current device database.
    [[nodiscard]] auto set_clock_stretching(bool /*enabled*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    // -----------------------------------------------------------------------
    // extend-i2c-coverage: status flags + clear
    // -----------------------------------------------------------------------

    /// True when the last addressed peripheral sent a NACK.
    /// STM32 v2: NACKF in ISR. STM32 v1: AF in SR1. SAME70: NACK in SR.
    [[nodiscard]] auto nack_received() const -> bool {
        if constexpr (nackf_field.valid) {
            const auto v = detail::runtime::read_field(nackf_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (af_field.valid) {
            const auto v = detail::runtime::read_field(af_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (nack_field.valid) {
            const auto v = detail::runtime::read_field(nack_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else {
            return false;
        }
    }

    /// True when arbitration was lost on the bus.
    [[nodiscard]] auto arbitration_lost() const -> bool {
        if constexpr (arlo_isr_field.valid) {
            const auto v = detail::runtime::read_field(arlo_isr_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (arlo_field.valid) {
            const auto v = detail::runtime::read_field(arlo_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (arblst_field.valid) {
            const auto v = detail::runtime::read_field(arblst_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else {
            return false;
        }
    }

    /// True when a bus error (misplaced START/STOP) was detected.
    [[nodiscard]] auto bus_error() const -> bool {
        if constexpr (berr_isr_field.valid) {
            const auto v = detail::runtime::read_field(berr_isr_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else if constexpr (berr_field.valid) {
            const auto v = detail::runtime::read_field(berr_field);
            return v.is_ok() && v.unwrap() != 0u;
        } else {
            return false;
        }
    }

    /// Clear the NACK flag. STM32 v2: write 1 to ICR.NACKCF. STM32 v1: write
    /// 0 to SR1.AF.
    [[nodiscard]] auto clear_nack() const -> core::Result<void, core::ErrorCode> {
        if constexpr (nackcf_field.valid) {
            return detail::runtime::modify_field(nackcf_field, 1u);
        } else if constexpr (af_field.valid) {
            return detail::runtime::modify_field(af_field, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Clear the arbitration-lost flag.
    [[nodiscard]] auto clear_arbitration_lost() const -> core::Result<void, core::ErrorCode> {
        if constexpr (arlocf_field.valid) {
            return detail::runtime::modify_field(arlocf_field, 1u);
        } else if constexpr (arlo_field.valid) {
            return detail::runtime::modify_field(arlo_field, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Clear the bus-error flag.
    [[nodiscard]] auto clear_bus_error() const -> core::Result<void, core::ErrorCode> {
        if constexpr (berrcf_field.valid) {
            return detail::runtime::modify_field(berrcf_field, 1u);
        } else if constexpr (berr_field.valid) {
            return detail::runtime::modify_field(berr_field, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    // -----------------------------------------------------------------------
    // extend-i2c-coverage: interrupts
    // -----------------------------------------------------------------------

    /// Enable a typed interrupt. Returns NotSupported — individual interrupt-
    /// enable field refs are not published in the current device database.
    [[nodiscard]] auto enable_interrupt(InterruptKind /*kind*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable a typed interrupt. Returns NotSupported (see enable_interrupt).
    [[nodiscard]] auto disable_interrupt(InterruptKind /*kind*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns the NVIC IRQ line numbers for this peripheral.
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

    // -----------------------------------------------------------------------
    // extend-i2c-coverage: kernel clock source
    // -----------------------------------------------------------------------

    /// Select the I2C kernel clock source. Gated on
    /// `kKernelClockSelectorField.valid`.
    [[nodiscard]] auto set_kernel_clock_source(KernelClockSource src) const
        -> core::Result<void, core::ErrorCode> {
        if constexpr (!semantic_traits::kKernelClockSelectorField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return detail::runtime::modify_field(
                semantic_traits::kKernelClockSelectorField,
                static_cast<std::uint32_t>(src));
        }
    }

    // -----------------------------------------------------------------------
    // extend-i2c-coverage: SMBus + PEC
    // -----------------------------------------------------------------------

    /// Enable/disable SMBus mode. Returns NotSupported — no SMBus enable field
    /// in current device database (kSupportsSmbus = true on G071, but the
    /// SMBHEN / SMBDEN bits are not yet published as field refs).
    [[nodiscard]] auto enable_smbus(bool /*enable*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set SMBus role (Host or Device). Returns NotSupported.
    [[nodiscard]] auto set_smbus_role(SmbusRole /*role*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable/disable PEC (Packet Error Checking). Returns NotSupported.
    [[nodiscard]] auto enable_pec(bool /*enable*/) const
        -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns the last PEC byte. Returns 0 when not supported.
    [[nodiscard]] auto last_pec() const -> std::uint8_t { return 0u; }

    /// True when a PEC error was detected.
    [[nodiscard]] auto pec_error() const -> bool { return false; }

    /// Clear the PEC error flag. Returns NotSupported.
    [[nodiscard]] auto clear_pec_error() const -> core::Result<void, core::ErrorCode> {
        return core::Err(core::ErrorCode::NotSupported);
    }

   private:
    Config config_{};
};

template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port_handle<Connector> {
    static_assert(port_handle<Connector>::valid,
                  "Requested I2C route has no valid descriptor-backed route for the selected "
                  "device/package. If you used ergonomic role aliases such as hal::scl<Pin> or "
                  "hal::sda<Pin>, retry with an explicit expert signal alias. Canonical forms "
                  "remain supported.");
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
                  "Requested I2C route has no valid descriptor-backed route for the selected "
                  "device/package. If you used ergonomic role aliases such as hal::scl<Pin> or "
                  "hal::sda<Pin>, retry with an explicit expert signal alias. Canonical forms "
                  "remain supported.");
    return shared_bus<Connector>{config};
}

}  // namespace alloy::hal::i2c
