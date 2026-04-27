#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::qspi {

#if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

// ── Enums ─────────────────────────────────────────────────────────────────────

/// Frame width for instruction / address / data phases.
/// On SAME70 all phases share a single IFR.WIDTH field; the enum values map
/// to the SINGLE_BIT_SPI (0), DUAL_OUTPUT (1) and QUAD_OUTPUT (2) encodings.
enum class FrameWidth : std::uint8_t {
    Single = 0u,  ///< 1-bit SPI
    Dual   = 1u,  ///< 2-bit dual output
    Quad   = 2u,  ///< 4-bit quad output
    Octal  = 3u,  ///< 8-bit octal (not all devices)
};

/// Operating mode.
enum class QspiMode : std::uint8_t {
    Indirect     = 0u,  ///< Normal indirect / SPI mode (SMM = 0)
    MemoryMapped = 1u,  ///< Serial-memory / XIP mode (SMM = 1)
    AutoPoll     = 2u,  ///< Auto-poll / status-match mode
};

/// Chip-select assertion policy.
enum class CsMode : std::uint8_t {
    LowOnTransfer  = 0u,  ///< Deassert when FIFO drained / not reloaded
    HighOnTransfer = 1u,  ///< Deassert on LASTXFER command
    PerInstruction = 2u,  ///< Deassert after every transfer
};

/// Typed interrupt selector.
enum class InterruptKind : std::uint8_t {
    TransferComplete = 0u,  ///< TX FIFO empty / transfer done (TXEMPTY)
    FifoThreshold    = 1u,  ///< RX data ready (RDRF)
    StatusMatch      = 2u,  ///< Auto-poll status match (STM32-style; NotSupported on SAME70)
    Timeout          = 3u,  ///< Timeout (NotSupported on SAME70)
    Error            = 4u,  ///< Error (NotSupported on SAME70)
};

/// Kernel clock source selector.
enum class KernelClockSource : std::uint8_t { Default = 0u };

// ─────────────────────────────────────────────────────────────────────────────

struct Config {
    bool enable_on_configure = true;
};

// ── Handle ────────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
class handle {
  public:
    using semantic_traits = device::QspiSemanticTraits<Peripheral>;
    using config_type     = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid         = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto r =
                    detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                r.is_err()) {
                return r;
            }
        }
        if (config_.enable_on_configure) {
            return enable();
        }
        return core::Ok();
    }

    [[nodiscard]] auto enable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto disable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDisableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDisableField, 1u);
        }
        if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto software_reset() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kSoftwareResetField.valid) {
            return detail::runtime::modify_field(semantic_traits::kSoftwareResetField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Phase 1: Frame configuration + transfer primitives ───────────────────

    /// Set frame width for the instruction phase.
    /// Maps to kWidthField (shared with address/data on SAME70 IFR.WIDTH).
    [[nodiscard]] auto set_instruction_width(FrameWidth w) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kWidthField.valid) {
            return detail::runtime::modify_field(semantic_traits::kWidthField,
                                                 static_cast<std::uint32_t>(w));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set frame width for the address phase.
    [[nodiscard]] auto set_address_width(FrameWidth w) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kWidthField.valid) {
            return detail::runtime::modify_field(semantic_traits::kWidthField,
                                                 static_cast<std::uint32_t>(w));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set frame width for the data phase.
    [[nodiscard]] auto set_data_width(FrameWidth w) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kWidthField.valid) {
            return detail::runtime::modify_field(semantic_traits::kWidthField,
                                                 static_cast<std::uint32_t>(w));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the instruction opcode for the next transfer.
    [[nodiscard]] auto set_instruction(std::uint8_t opcode) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kInstructionField.valid) {
            return detail::runtime::modify_field(semantic_traits::kInstructionField,
                                                 static_cast<std::uint32_t>(opcode));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the address for the next transfer. `bits` is informational
    /// (address length in bits, e.g. 24 or 32); the hardware register is
    /// written with the full 32-bit `addr` value.
    [[nodiscard]] auto set_address(std::uint32_t addr, std::uint8_t /*bits*/ = 24u) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kAddressField.valid) {
            return detail::runtime::modify_field(semantic_traits::kAddressField, addr);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the number of dummy cycles between command and data phases.
    [[nodiscard]] auto set_dummy_cycles(std::uint8_t cycles) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kDummyCyclesField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDummyCyclesField,
                                                 static_cast<std::uint32_t>(cycles));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set bits-per-transfer. Gated on kBitsPerTransferField (MR.NBBITS on SAME70).
    [[nodiscard]] auto set_bits_per_transfer(std::uint8_t bits) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kBitsPerTransferField.valid) {
            return detail::runtime::modify_field(semantic_traits::kBitsPerTransferField,
                                                 static_cast<std::uint32_t>(bits));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Blocking byte-at-a-time read via the receive data register.
    /// Polls kReceiveReadyField before each byte; returns Timeout on stall.
    [[nodiscard]] auto read(std::span<std::byte> buf) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kReceiveDataRegister.valid &&
                      semantic_traits::kReceiveReadyField.valid) {
            for (auto& byte : buf) {
                bool ready = false;
                for (std::size_t i = 0u; i < 0xFFFFu && !ready; ++i) {
                    const auto r =
                        detail::runtime::read_field(semantic_traits::kReceiveReadyField);
                    ready = r.is_ok() && r.unwrap() != 0u;
                }
                if (!ready) {
                    return core::Err(core::ErrorCode::Timeout);
                }
                const auto r =
                    detail::runtime::read_register(semantic_traits::kReceiveDataRegister);
                if (!r.is_ok()) {
                    return core::Err(r.unwrap_err());
                }
                byte = static_cast<std::byte>(r.unwrap() & 0xFFu);
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Blocking byte-at-a-time write via the transmit data register.
    /// Polls kTransmitReadyField before each byte; returns Timeout on stall.
    [[nodiscard]] auto write(std::span<const std::byte> buf) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kTransmitDataRegister.valid &&
                      semantic_traits::kTransmitReadyField.valid) {
            for (const auto byte : buf) {
                bool ready = false;
                for (std::size_t i = 0u; i < 0xFFFFu && !ready; ++i) {
                    const auto r =
                        detail::runtime::read_field(semantic_traits::kTransmitReadyField);
                    ready = r.is_ok() && r.unwrap() != 0u;
                }
                if (!ready) {
                    return core::Err(core::ErrorCode::Timeout);
                }
                const auto r = detail::runtime::write_register(
                    semantic_traits::kTransmitDataRegister,
                    static_cast<std::uint32_t>(byte));
                if (!r.is_ok()) {
                    return r;
                }
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns true when the TX FIFO is empty (transfer complete).
    [[nodiscard]] auto last_transfer_done() const -> bool {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kTransmitEmptyField.valid) {
            const auto r =
                detail::runtime::read_field(semantic_traits::kTransmitEmptyField);
            return r.is_ok() && r.unwrap() != 0u;
        }
        return true;
    }

    // ── Phase 2: Mode + chip-select + continuous-read + scrambling ────────────

    /// Set the QSPI operating mode.
    /// Maps kSerialMemoryModeField (MR.SMM): 0 = Indirect/SPI, 1 = MemoryMapped.
    /// AutoPoll is NotSupported on SAME70.
    [[nodiscard]] auto set_mode(QspiMode mode) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kSerialMemoryModeField.valid) {
            const std::uint32_t val = (mode == QspiMode::MemoryMapped) ? 1u : 0u;
            if (mode == QspiMode::AutoPoll) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return detail::runtime::modify_field(semantic_traits::kSerialMemoryModeField, val);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Set the chip-select assertion policy. Gated on kChipSelectModeField.
    [[nodiscard]] auto set_cs_mode(CsMode mode) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kChipSelectModeField.valid) {
            return detail::runtime::modify_field(semantic_traits::kChipSelectModeField,
                                                 static_cast<std::uint32_t>(mode));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable or disable continuous-read mode. Gated on kContinuousReadModeField.
    [[nodiscard]] auto enable_continuous_read(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kContinuousReadModeField.valid) {
            return detail::runtime::modify_field(semantic_traits::kContinuousReadModeField,
                                                 enable ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable or disable data scrambling. Gated on kHasScrambling.
    [[nodiscard]] auto enable_scrambling(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasScrambling &&
                      semantic_traits::kScramblingEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kScramblingEnableField,
                                                 enable ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Write the scrambling key (SAME70 QSPI_SKR). Gated on kHasScrambling.
    [[nodiscard]] auto set_scrambling_key(std::uint32_t key) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasScrambling &&
                      semantic_traits::kScramblingKeyRegister.valid) {
            return detail::runtime::write_register(semantic_traits::kScramblingKeyRegister, key);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Cache-invalidation hook after writes in MemoryMapped mode.
    /// Application code must call the platform-specific D-cache invalidation
    /// routine (e.g. SCB_InvalidateDCache_by_Addr()) separately; this method
    /// marks the intent in the HAL API surface.
    void invalidate_cache_for_memory_map() const {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        // Platform hook: no-op in HAL — caller provides D-cache invalidation.
    }

    // ── Phase 3: Kernel clock + DMA + interrupts + IRQ vector ─────────────────

    /// Select the QSPI kernel clock source.
    /// Returns NotSupported on all current devices (no kKernelClockSelectorField).
    [[nodiscard]] auto set_kernel_clock_source(KernelClockSource /*src*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kKernelClockSourceOptions.size() == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Wire up a DMA channel for QSPI transfers. Gated on kHasDma.
    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        if constexpr (semantic_traits::kHasDma) {
            static_assert(DmaChannel::valid);
            static_assert(DmaChannel::peripheral_id == peripheral_id);
            return channel.configure();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Enable a specific interrupt kind.
    /// SAME70 uses write-only IER / IDR registers (IER/IDR pattern).
    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        return interrupt_arm_(kind, true);
    }

    /// Disable a specific interrupt kind.
    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "QSPI peripheral is not present on the selected device.");
        return interrupt_arm_(kind, false);
    }

    /// Returns the device-level IRQ numbers for this peripheral.
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
    }

    // ── DMA address helpers ───────────────────────────────────────────────────

    [[nodiscard]] static constexpr auto rx_data_register_address() -> std::uintptr_t {
        if constexpr (semantic_traits::kReceiveDataRegister.valid) {
            return semantic_traits::kReceiveDataRegister.base_address +
                   semantic_traits::kReceiveDataRegister.offset_bytes;
        }
        return 0u;
    }

    [[nodiscard]] static constexpr auto tx_data_register_address() -> std::uintptr_t {
        if constexpr (semantic_traits::kTransmitDataRegister.valid) {
            return semantic_traits::kTransmitDataRegister.base_address +
                   semantic_traits::kTransmitDataRegister.offset_bytes;
        }
        return 0u;
    }

   private:
    Config config_{};

    // ── Interrupt helper (IER/IDR or single-bit pattern) ─────────────────────

    [[nodiscard]] auto interrupt_arm_field_(
        const detail::runtime::FieldRef& field, bool enable) const
        -> core::Result<void, core::ErrorCode> {
        if (enable) {
            return detail::runtime::modify_field(field, 1u);
        }
        if constexpr (semantic_traits::kInterruptDisableRegister.valid) {
            // SAME70: write 1 to IDR at the same bit position as IER.
            return detail::runtime::write_register(
                semantic_traits::kInterruptDisableRegister,
                1u << field.bit_offset);
        } else {
            return detail::runtime::modify_field(field, 0u);
        }
    }

    [[nodiscard]] auto interrupt_arm_(InterruptKind kind, bool enable) const
        -> core::Result<void, core::ErrorCode> {
        switch (kind) {
            case InterruptKind::TransferComplete:
                if constexpr (semantic_traits::kTransmitEmptyInterruptEnableField.valid) {
                    return interrupt_arm_field_(
                        semantic_traits::kTransmitEmptyInterruptEnableField, enable);
                }
                return core::Err(core::ErrorCode::NotSupported);

            case InterruptKind::FifoThreshold:
                if constexpr (semantic_traits::kReceiveReadyInterruptEnableField.valid) {
                    return interrupt_arm_field_(
                        semantic_traits::kReceiveReadyInterruptEnableField, enable);
                }
                return core::Err(core::ErrorCode::NotSupported);

            case InterruptKind::StatusMatch:
            case InterruptKind::Timeout:
            case InterruptKind::Error:
            default:
                return core::Err(core::ErrorCode::NotSupported);
        }
    }
};

// ── Factory ───────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "QSPI peripheral is not present on the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::qspi
