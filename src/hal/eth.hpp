#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::eth {

#if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

// ── Enums ─────────────────────────────────────────────────────────────────────

/// Link speed. Gbit1 is NotSupported on all currently published devices
/// (GMAC NCFGR.SPD is a 1-bit field: 0 = 10 Mbps, 1 = 100 Mbps).
enum class LinkSpeed : std::uint8_t {
    Mbit10  = 0u,
    Mbit100 = 1u,
    Gbit1   = 2u,
};

/// PHY interface type.
/// On SAME70 GMAC, kRmiiEnableField (UR.RMII): 0 = MII, 1 = RMII.
/// RGMII is not available on GMAC and returns NotSupported.
enum class PhyInterface : std::uint8_t {
    Mii   = 0u,
    Rmii  = 1u,
    Rgmii = 2u,
};

/// Statistic counter selector.
enum class StatisticId : std::uint8_t {
    RxFrames     = 0u,
    RxBytes      = 1u,
    RxErrors     = 2u,
    RxCrcErrors  = 3u,
    RxOverflow   = 4u,
    TxFrames     = 5u,
    TxBytes      = 6u,
    TxErrors     = 7u,
    TxCollisions = 8u,
};

/// Typed interrupt selector.
enum class InterruptKind : std::uint8_t {
    RxComplete      = 0u,  ///< Frame receive complete (GMAC ISR.RCOMP)
    TxComplete      = 1u,  ///< Frame transmit complete (GMAC ISR.TCOMP)
    RxError         = 2u,  ///< RX used-bit read / overrun (RXUBR)
    TxError         = 3u,  ///< TX frame corruption (TFC)
    ManagementDone  = 4u,  ///< Management frame sent (MFS)
    LinkChange      = 5u,  ///< PHY link-change (not available on GMAC)
};

// ── Descriptor types ──────────────────────────────────────────────────────────

/// Ethernet RX buffer descriptor (GMAC hardware layout, 8 bytes).
/// word0 bits[1:0]: ownership / wrap flags; bits[31:2]: buffer address.
/// word1: receive status word.
struct RxDescriptor {
    std::uint32_t addr;    ///< Buffer base address (4-byte aligned) + flags
    std::uint32_t status;  ///< Receive status
};

/// Ethernet TX buffer descriptor (GMAC hardware layout, 8 bytes).
/// word0: buffer address; word1: control / status (length, flags, used).
struct TxDescriptor {
    std::uint32_t addr;    ///< Buffer base address
    std::uint32_t status;  ///< Transmit control / status
};

// ─────────────────────────────────────────────────────────────────────────────

struct Config {
    bool enable_rx_on_configure = false;
    bool enable_tx_on_configure = false;
};

// ── Handle ────────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
class handle {
  public:
    using semantic_traits = device::EthSemanticTraits<Peripheral>;
    using config_type     = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid         = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto r =
                    detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                r.is_err()) {
                return r;
            }
        }
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enable_rx_on_configure) {
            result = enable_rx(true);
        }
        if (result.is_ok() && config_.enable_tx_on_configure) {
            result = enable_tx(true);
        }
        return result;
    }

    // ── Phase 1: Mode + PHY interface + MDIO ─────────────────────────────────

    /// Set the link speed. Gbit1 returns NotSupported (GMAC is 10/100 only).
    [[nodiscard]] auto set_speed(LinkSpeed speed) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if (speed == LinkSpeed::Gbit1) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        if constexpr (semantic_traits::kSpeedField.valid) {
            return detail::runtime::modify_field(semantic_traits::kSpeedField,
                                                 static_cast<std::uint32_t>(speed));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set full-duplex (true) or half-duplex (false) mode.
    [[nodiscard]] auto set_full_duplex(bool full) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kFullDuplexField.valid) {
            return detail::runtime::modify_field(semantic_traits::kFullDuplexField,
                                                 full ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the PHY interface type.
    /// GMAC UR.RMII: 0 = MII, 1 = RMII. RGMII returns NotSupported.
    [[nodiscard]] auto set_phy_interface(PhyInterface iface) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if (iface == PhyInterface::Rgmii) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        if constexpr (semantic_traits::kRmiiEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kRmiiEnableField,
                                                 (iface == PhyInterface::Rmii) ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the MDC clock divider (GMAC NCFGR.CLK). Gated on kMdcClockDividerField.
    [[nodiscard]] auto set_mdc_clock_divider(std::uint8_t div) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kMdcClockDividerField.valid) {
            return detail::runtime::modify_field(semantic_traits::kMdcClockDividerField,
                                                 static_cast<std::uint32_t>(div));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable or disable the management port (GMAC NCR.MPE).
    [[nodiscard]] auto enable_management_port(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kManagementPortEnableField.valid) {
            return detail::runtime::modify_field(
                semantic_traits::kManagementPortEnableField, enable ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 2: DMA descriptor rings + statistics ────────────────────────────

    /// Enable or disable the MAC receive path (GMAC NCR.RXEN).
    [[nodiscard]] auto enable_rx(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kRxEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kRxEnableField,
                                                 enable ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable or disable the MAC transmit path (GMAC NCR.TXEN).
    [[nodiscard]] auto enable_tx(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kTxEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kTxEnableField,
                                                 enable ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the RX descriptor ring base address (GMAC RBQB).
    [[nodiscard]] auto set_rx_descriptor_base(std::uintptr_t addr) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kRxDescriptorBaseRegister.valid) {
            return detail::runtime::write_register(
                semantic_traits::kRxDescriptorBaseRegister,
                static_cast<std::uint32_t>(addr));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the TX descriptor ring base address (GMAC TBQB).
    [[nodiscard]] auto set_tx_descriptor_base(std::uintptr_t addr) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kTxDescriptorBaseRegister.valid) {
            return detail::runtime::write_register(
                semantic_traits::kTxDescriptorBaseRegister,
                static_cast<std::uint32_t>(addr));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Configure RX descriptor ring: writes base address from the first descriptor.
    [[nodiscard]] auto configure_rx_descriptors(std::span<RxDescriptor> ring) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if (ring.empty()) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        return set_rx_descriptor_base(
            reinterpret_cast<std::uintptr_t>(ring.data()));
    }

    /// Configure TX descriptor ring: writes base address from the first descriptor.
    [[nodiscard]] auto configure_tx_descriptors(std::span<TxDescriptor> ring) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if (ring.empty()) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        return set_tx_descriptor_base(
            reinterpret_cast<std::uintptr_t>(ring.data()));
    }

    /// Kick the TX DMA engine (GMAC NCR.TSTART).
    [[nodiscard]] auto tx_start() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kTxStartField.valid) {
            return detail::runtime::modify_field(semantic_traits::kTxStartField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Read a statistics counter. Returns 0 when kHasStatisticsCounters is false
    /// or the counter register is not yet mapped in the device contract.
    /// GMAC statistics are at fixed offsets from base (see SAME70 §44.7).
    [[nodiscard]] auto read_statistic(StatisticId /*id*/) const -> std::uint32_t {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        // Statistics registers are not yet individually exposed as FieldRefs in
        // the device contract; returning 0 pending a codegen extension.
        return 0u;
    }

    /// Clear all statistics counters (GMAC NCR.CLRSTAT).
    [[nodiscard]] auto clear_statistics() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasStatisticsCounters &&
                      semantic_traits::kClearStatisticsField.valid) {
            return detail::runtime::modify_field(semantic_traits::kClearStatisticsField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 3: Interrupts + IRQ vector ──────────────────────────────────────

    /// Enable a specific interrupt kind (writes to IER on GMAC).
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kInterruptEnableRegister.valid) {
            const std::uint32_t bit = interrupt_kind_to_bit(kind);
            if (bit == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::write_register(
                semantic_traits::kInterruptEnableRegister, bit);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable a specific interrupt kind (writes to IDR on GMAC).
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Ethernet peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kInterruptDisableRegister.valid) {
            const std::uint32_t bit = interrupt_kind_to_bit(kind);
            if (bit == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::write_register(
                semantic_traits::kInterruptDisableRegister, bit);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns the device-level IRQ numbers for this peripheral.
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

   private:
    Config config_{};

    // ── Interrupt bit mapping ─────────────────────────────────────────────────
    // IER/IDR bit positions mirror ISR register layout (GMAC-specific).
    // Known IE FieldRefs provide bit_offset; remaining bits use GMAC spec §44.8.

    [[nodiscard]] static constexpr auto interrupt_kind_to_bit(InterruptKind kind)
        -> std::uint32_t {
        switch (kind) {
            case InterruptKind::RxComplete:
                return semantic_traits::kRxCompleteInterruptEnableField.valid
                       ? (1u << semantic_traits::kRxCompleteInterruptEnableField.bit_offset)
                       : 0u;
            case InterruptKind::TxComplete:
                return semantic_traits::kTxCompleteInterruptEnableField.valid
                       ? (1u << semantic_traits::kTxCompleteInterruptEnableField.bit_offset)
                       : 0u;
            case InterruptKind::ManagementDone: return 1u << 0u;  // MFS
            case InterruptKind::RxError:        return 1u << 2u;  // RXUBR
            case InterruptKind::TxError:        return 1u << 6u;  // TFC
            case InterruptKind::LinkChange:
            default:                            return 0u;
        }
    }
};

// ── Factory ───────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Ethernet peripheral is not present on the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::eth
