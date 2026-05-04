#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::pwm {

#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

// ---- Enumerations ---------------------------------------------------------

enum class CenterAligned : std::uint8_t { Disabled, Mode1, Mode2, Mode3 };

enum class Polarity : std::uint8_t { Active, Inverted };

enum class InterruptKind : std::uint8_t {
    ChannelCompare,
    Update,
    Fault,
    Break,
    ChannelCompare_DMA,
    Update_DMA,
};

// ---- Config ---------------------------------------------------------------

struct Config {
    std::uint32_t period = 0u;
    bool apply_period = false;
    std::uint32_t duty_cycle = 0u;
    bool apply_duty_cycle = false;
    bool start_immediately = false;
};

// ---- handle ---------------------------------------------------------------

template <PeripheralId Peripheral, std::size_t Channel>
class handle {
   public:
    using peripheral_traits = device::PwmSemanticTraits<Peripheral>;
    using channel_traits = device::PwmChannelSemanticTraits<Peripheral, Channel>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr auto channel_index = Channel;
    static constexpr bool valid = peripheral_traits::kPresent && channel_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ---- static accessors -------------------------------------------------

    /// NVIC IRQ line numbers (PwmSemanticTraits does not yet publish kIrqNumbers
    /// — returns empty span until device database adds the field).
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return {};
    }

    /// Returns the paired channel index.
    /// kPairedChannels pairing map not yet published in PwmChannelSemanticTraits;
    /// returns Channel itself until the device database adds an index field.
    [[nodiscard]] static constexpr auto paired_channel() -> std::uint8_t {
        return static_cast<std::uint8_t>(Channel);
    }

    // ---- configure --------------------------------------------------------

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto clk = detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                clk.is_err()) {
                return clk;
            }
        }
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.apply_period) {
            result = set_period(config_.period);
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.apply_duty_cycle) {
            result = set_duty_cycle(config_.duty_cycle);
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.start_immediately) {
            result = start();
        }
        return result;
    }

    // ---- start / stop -----------------------------------------------------

    [[nodiscard]] auto start() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (channel_traits::kEnableField.valid) {
            auto result = detail::runtime::modify_field(channel_traits::kEnableField, 1u);
            if (!result.is_ok()) {
                return result;
            }
            if constexpr (peripheral_traits::kMasterOutputEnableField.valid) {
                result = detail::runtime::modify_field(
                    peripheral_traits::kMasterOutputEnableField, 1u);
                if (!result.is_ok()) {
                    return result;
                }
            }
            if constexpr (peripheral_traits::kLoadField.valid) {
                result = detail::runtime::modify_field(peripheral_traits::kLoadField, 1u);
            }
            return result;
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto stop() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (channel_traits::kEnableField.valid) {
            return detail::runtime::modify_field(channel_traits::kEnableField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    // ---- period / duty / frequency ----------------------------------------

    [[nodiscard]] auto set_period(std::uint32_t period) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (channel_traits::kPeriodField.valid) {
            constexpr bool kDirectRegisterWrite = channel_traits::kPeriodField.field_id ==
                                                      device::FieldId::none &&
                                                  channel_traits::kPeriodField.bit_offset == 0u;
            if constexpr (kDirectRegisterWrite) {
                const auto bits =
                    detail::runtime::field_bits(channel_traits::kPeriodField, period);
                if (bits.is_err()) {
                    return core::Err(core::ErrorCode{bits.unwrap_err()});
                }
                return detail::runtime::write_register(channel_traits::kPeriodField.reg,
                                                       bits.unwrap());
            }
            return detail::runtime::modify_field(channel_traits::kPeriodField, period);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_duty_cycle(std::uint32_t duty) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (channel_traits::kDutyField.valid) {
            constexpr bool kDirectRegisterWrite =
                channel_traits::kDutyField.field_id == device::FieldId::none &&
                channel_traits::kDutyField.bit_offset == 0u;
            if constexpr (kDirectRegisterWrite) {
                const auto bits = detail::runtime::field_bits(channel_traits::kDutyField, duty);
                if (bits.is_err()) {
                    return core::Err(core::ErrorCode{bits.unwrap_err()});
                }
                return detail::runtime::write_register(channel_traits::kDutyField.reg,
                                                       bits.unwrap());
            }
            return detail::runtime::modify_field(channel_traits::kDutyField, duty);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_frequency(std::uint32_t) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ---- Phase 1: mode / polarity / complementary -------------------------

    [[nodiscard]] auto set_center_aligned(CenterAligned mode) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr ((peripheral_traits::kHasCenterAligned) &&
                      channel_traits::kCenterAlignedField.valid) {
            return detail::runtime::modify_field(channel_traits::kCenterAlignedField,
                                                 static_cast<std::uint32_t>(mode));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_channel_polarity(Polarity polarity) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (channel_traits::kPolarityField.valid) {
            return detail::runtime::modify_field(channel_traits::kPolarityField,
                                                 static_cast<std::uint32_t>(polarity));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto enable_complementary_output(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr ((peripheral_traits::kHasComplementaryOutputs ||
                       peripheral_traits::kSupportsComplementaryOutputs) &&
                      channel_traits::kComplementaryOutputEnableField.valid) {
            return detail::runtime::modify_field(channel_traits::kComplementaryOutputEnableField,
                                                 enable ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Complementary polarity: no dedicated field in current database → NotSupported.
    [[nodiscard]] auto set_complementary_polarity(Polarity polarity) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        (void)polarity;
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ---- Phase 2: dead-time / fault / master-output / sync ---------------

    /// Sets dead-time via kDeadtimeRiseField and kDeadtimeFallField.
    /// On STM32 both fields map to the same BDTR.DTG register (rise == fall).
    /// On SAME70 PWM they are separate DTHI / DTLI fields.
    [[nodiscard]] auto set_dead_time(std::uint8_t rise_cycles,
                                     std::uint8_t fall_cycles) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr ((peripheral_traits::kHasDeadtime || peripheral_traits::kSupportsDeadtime) &&
                      channel_traits::kDeadtimeRiseField.valid) {
            auto r = detail::runtime::modify_field(channel_traits::kDeadtimeRiseField,
                                                   static_cast<std::uint32_t>(rise_cycles));
            if (r.is_err()) {
                return r;
            }
            if constexpr (channel_traits::kDeadtimeFallField.valid) {
                // Only write fall separately when it maps to a different register location.
                constexpr bool kSameField =
                    channel_traits::kDeadtimeRiseField.reg.offset_bytes ==
                        channel_traits::kDeadtimeFallField.reg.offset_bytes &&
                    channel_traits::kDeadtimeRiseField.bit_offset ==
                        channel_traits::kDeadtimeFallField.bit_offset;
                if constexpr (!kSameField) {
                    r = detail::runtime::modify_field(channel_traits::kDeadtimeFallField,
                                                      static_cast<std::uint32_t>(fall_cycles));
                }
            }
            return r;
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Fault input: kFaultEnableField not yet published → NotSupported.
    [[nodiscard]] auto enable_fault_input(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        (void)enable;
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto set_fault_polarity(Polarity polarity) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        (void)polarity;
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto fault_active() const -> bool {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        return false;
    }

    [[nodiscard]] auto clear_fault_flag() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Asserts or deasserts the master output enable (MOE) for advanced timers.
    [[nodiscard]] auto enable_master_output(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (peripheral_traits::kMasterOutputEnableField.valid) {
            return detail::runtime::modify_field(peripheral_traits::kMasterOutputEnableField,
                                                 enable ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Enables or disables synchronized period/duty update (shadow register lock).
    [[nodiscard]] auto set_update_synchronized(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr ((peripheral_traits::kHasSynchronizedUpdate) &&
                      peripheral_traits::kLoadField.valid) {
            return detail::runtime::modify_field(peripheral_traits::kLoadField,
                                                 enable ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    // ---- Phase 3: carrier modulation / force-initialize -------------------

    /// Carrier modulation: not yet published in device database → NotSupported.
    [[nodiscard]] auto set_carrier_modulation(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        (void)enable;
        return core::Err(core::ErrorCode::NotSupported);
    }

    /// Force-initialize output: not yet published in device database → NotSupported.
    [[nodiscard]] auto force_initialize() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ---- Phase 4: status / events / interrupts ----------------------------

    [[nodiscard]] auto channel_event() const -> bool {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (channel_traits::kInterruptFlagField.valid) {
            const auto val = detail::runtime::read_field(channel_traits::kInterruptFlagField);
            return val.is_ok() && val.unwrap() != 0u;
        } else {
            return false;
        }
    }

    [[nodiscard]] auto clear_channel_event() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        if constexpr (channel_traits::kInterruptFlagField.valid) {
            return detail::runtime::modify_field(channel_traits::kInterruptFlagField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto enable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        return interrupt_arm_(kind, true);
    }

    [[nodiscard]] auto disable_interrupt(InterruptKind kind) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested PWM channel is not published for the selected device.");
        return interrupt_arm_(kind, false);
    }

    // ---- DMA helpers (unchanged) ------------------------------------------

    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& dma_ch) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_TX);
        return dma_ch.configure();
    }

    [[nodiscard]] static constexpr auto data_register_address() -> std::uintptr_t {
        if constexpr (channel_traits::kDutyField.valid) {
            return channel_traits::kDutyField.reg.base_address +
                   channel_traits::kDutyField.reg.offset_bytes;
        } else if constexpr (channel_traits::kPeriodField.valid) {
            return channel_traits::kPeriodField.reg.base_address +
                   channel_traits::kPeriodField.reg.offset_bytes;
        } else {
            return 0u;
        }
    }

   private:
    Config config_{};

    [[nodiscard]] auto interrupt_arm_(InterruptKind kind, bool enable) const
        -> core::Result<void, core::ErrorCode> {
        const std::uint32_t val = enable ? 1u : 0u;
        if (kind == InterruptKind::ChannelCompare) {
            if constexpr (channel_traits::kInterruptEnableField.valid) {
                return detail::runtime::modify_field(channel_traits::kInterruptEnableField, val);
            } else {
                return core::Err(core::ErrorCode::NotSupported);
            }
        }
        // Update, Fault, Break, *_DMA: IE field refs not yet published.
        return core::Err(core::ErrorCode::NotSupported);
    }
};

template <PeripheralId Peripheral, std::size_t Channel>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral, Channel> {
    static_assert(handle<Peripheral, Channel>::valid,
                  "Requested PWM channel is not published for the selected device.");
    return handle<Peripheral, Channel>{config};
}
#endif

}  // namespace alloy::hal::pwm

// alloy.device.v2.1: PWM is provided by hal/timer/lite.hpp (StPwmTimer concept).
// Include hal/timer.hpp and use port<P>::configure_pwm_ch / set_duty / enable_ch.
// No separate hal/pwm/lite.hpp is needed — timer and PWM share the same STM32 TIM
// peripheral and the same port<P> type.
#include "hal/timer/lite.hpp"
