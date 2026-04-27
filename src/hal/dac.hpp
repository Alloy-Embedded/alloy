#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::dac {

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

struct Config {
    bool enable_on_configure = true;
    bool write_initial_value = false;
    std::uint32_t initial_value = 0u;
};

// ── New enums (extend-dac-coverage) ──────────────────────────────────────────

/// Hardware trigger edge. DAC vendors rarely expose edge selection; when
/// no edge-select field is published in the DB, Rising maps to "trigger
/// enabled" and Disabled maps to "trigger off".
enum class TriggerEdge : std::uint8_t { Disabled = 0u, Rising, Falling, Both };

/// Per-kind interrupt selector.
enum class InterruptKind : std::uint8_t {
    TransferComplete = 0u,
    Underrun,
    DmaComplete,
};

/// Kernel clock source selector (reserved for future codegen; currently no
/// device publishes a DAC clock-selector field).
enum class KernelClockSource : std::uint8_t { Default = 0u };

// ─────────────────────────────────────────────────────────────────────────────

template <PeripheralId Peripheral, std::size_t Channel>
class handle {
  public:
    using peripheral_traits = device::DacSemanticTraits<Peripheral>;
    using channel_traits = device::DacChannelSemanticTraits<Peripheral, Channel>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto channel_index = Channel;
    static constexpr bool valid = peripheral_traits::kPresent && channel_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ── Peripheral lifecycle ──────────────────────────────────────────────────

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto clk = detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                clk.is_err()) {
                return clk;
            }
        }
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enable_on_configure) {
            result = enable();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.write_initial_value) {
            result = write(config_.initial_value);
        }
        return result;
    }

    // ── Single-channel ops (legacy — uses channel_traits) ────────────────────

    [[nodiscard]] auto enable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kEnableField.valid) {
            return detail::runtime::modify_field(channel_traits::kEnableField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto disable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kDisableField.valid) {
            return detail::runtime::modify_field(channel_traits::kDisableField, 1u);
        }
        if constexpr (channel_traits::kEnableField.valid) {
            return detail::runtime::modify_field(channel_traits::kEnableField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto ready() const -> bool {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kReadyField.valid) {
            const auto state = detail::runtime::read_field(channel_traits::kReadyField);
            return state.is_ok() && state.unwrap() != 0u;
        }
        return true;
    }

    [[nodiscard]] auto write(std::uint32_t value) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (channel_traits::kDataField.valid) {
            return detail::runtime::modify_field(channel_traits::kDataField, value);
        }
        if constexpr (peripheral_traits::kDataRegister.valid) {
            return detail::runtime::write_register(peripheral_traits::kDataRegister, value);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 1.1: indexed multi-channel ops ──────────────────────────────────

    /// Enable channel `ch` (0-based). Gated on kChannelEnablePattern.
    [[nodiscard]] auto enable_channel(std::uint8_t ch) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (peripheral_traits::kChannelEnablePattern.valid) {
            return detail::runtime::modify_indexed_channel_field(
                peripheral_traits::kChannelEnablePattern,
                static_cast<std::size_t>(ch), 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable channel `ch`. Uses kChannelDisablePattern if available
    /// (separate write-only register on SAME70); otherwise clears the
    /// enable bit (STM32 CR style).
    [[nodiscard]] auto disable_channel(std::uint8_t ch) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (peripheral_traits::kChannelDisablePattern.valid) {
            return detail::runtime::modify_indexed_channel_field(
                peripheral_traits::kChannelDisablePattern,
                static_cast<std::size_t>(ch), 1u);
        } else if constexpr (peripheral_traits::kChannelEnablePattern.valid) {
            return detail::runtime::modify_indexed_channel_field(
                peripheral_traits::kChannelEnablePattern,
                static_cast<std::size_t>(ch), 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Returns true when the channel output is ready / settled.
    [[nodiscard]] auto channel_ready(std::uint8_t ch) const -> bool {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (peripheral_traits::kChannelReadyPattern.valid) {
            const auto r = detail::runtime::read_indexed_channel_field(
                peripheral_traits::kChannelReadyPattern, static_cast<std::size_t>(ch));
            return r.is_ok() && r.unwrap() != 0u;
        }
        return true;
    }

    /// Write `value` to channel `ch`'s data register.
    [[nodiscard]] auto write_channel(std::uint8_t ch, std::uint32_t value) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (peripheral_traits::kDataPattern.valid) {
            return detail::runtime::modify_indexed_channel_field(
                peripheral_traits::kDataPattern, static_cast<std::size_t>(ch), value);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 1.2: hardware trigger ────────────────────────────────────────────

    /// Configure hardware trigger for the handle's channel.
    /// `source` is the vendor-specific trigger source index.
    /// `TriggerEdge::Disabled` turns the trigger off.
    [[nodiscard]] auto set_hardware_trigger(std::uint8_t source, TriggerEdge edge) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (peripheral_traits::kHasHardwareTrigger) {
            const bool en = (edge != TriggerEdge::Disabled);
            if constexpr (channel_traits::kTriggerEnableField.valid) {
                const auto r =
                    detail::runtime::modify_field(channel_traits::kTriggerEnableField, en ? 1u : 0u);
                if (!r.is_ok()) {
                    return r;
                }
            }
            if (en) {
                if constexpr (channel_traits::kTriggerSelectField.valid) {
                    return detail::runtime::modify_field(
                        channel_traits::kTriggerSelectField,
                        static_cast<std::uint32_t>(source));
                }
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 1.3: prescaler ───────────────────────────────────────────────────

    /// Set the DAC prescaler (SAME70 DACC MR.PRESCALER).
    /// NotSupported on devices where kPrescalerField is not published.
    [[nodiscard]] auto set_prescaler(std::uint16_t prescaler) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (peripheral_traits::kPrescalerField.valid) {
            return detail::runtime::modify_field(peripheral_traits::kPrescalerField,
                                                  static_cast<std::uint32_t>(prescaler));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 1.4: software reset ──────────────────────────────────────────────

    /// Issue a peripheral software reset (SAME70 DACC CR.SWRST).
    /// NotSupported when kSoftwareResetField is not published.
    [[nodiscard]] auto software_reset() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");

        if constexpr (peripheral_traits::kSoftwareResetField.valid) {
            return detail::runtime::modify_field(peripheral_traits::kSoftwareResetField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 2.1: typed interrupts ────────────────────────────────────────────

    /// Enable a specific interrupt kind.
    /// Returns NotSupported until individual IE field refs are published in DB.
    [[nodiscard]] auto enable_interrupt(InterruptKind /*kind*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Disable a specific interrupt kind.
    [[nodiscard]] auto disable_interrupt(InterruptKind /*kind*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 2.2: underrun status ─────────────────────────────────────────────

    /// Returns true when the peripheral-level underrun flag is set.
    /// NotSupported stub — no underrun field published in current DB.
    [[nodiscard]] auto underrun() const -> bool {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");
        return false;
    }

    /// Returns true when channel `ch`'s underrun flag is set.
    [[nodiscard]] auto underrun_channel(std::uint8_t /*ch*/) const -> bool {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");
        return false;
    }

    /// Clear the underrun flag(s).
    [[nodiscard]] auto clear_underrun() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 2.3: kernel clock source ────────────────────────────────────────

    /// Select the DAC kernel clock source.
    /// Returns NotSupported until kKernelClockSelectorField is published in DB.
    [[nodiscard]] auto set_kernel_clock_source(KernelClockSource /*src*/) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested DAC channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Task 2.4: IRQ vector ──────────────────────────────────────────────────

    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{peripheral_traits::kIrqNumbers};
    }

    // ── DMA helpers (pre-existing) ────────────────────────────────────────────

    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        return channel.configure();
    }

    [[nodiscard]] static constexpr auto data_register_address() -> std::uintptr_t {
        if constexpr (channel_traits::kDataField.valid) {
            return channel_traits::kDataField.reg.base_address +
                   channel_traits::kDataField.reg.offset_bytes;
        } else if constexpr (peripheral_traits::kDataRegister.valid) {
            return peripheral_traits::kDataRegister.base_address +
                   peripheral_traits::kDataRegister.offset_bytes;
        } else {
            return 0u;
        }
    }

   private:
    Config config_{};
};

template <PeripheralId Peripheral, std::size_t Channel>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral, Channel> {
    static_assert(handle<Peripheral, Channel>::valid,
                  "Requested DAC channel is not published for the selected device.");
    return handle<Peripheral, Channel>{config};
}
#endif

}  // namespace alloy::hal::dac
