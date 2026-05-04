#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/dma/bindings.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::adc {

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

enum class Resolution : std::uint8_t {
    Bits6  = 0,
    Bits8  = 1,
    Bits10 = 2,
    Bits12 = 3,
    Bits14 = 4,
    Bits16 = 5,
};

enum class Alignment : std::uint8_t {
    Right = 0,
    Left  = 1,
};

enum class TriggerEdge : std::uint8_t {
    Disabled = 0,
    Rising   = 1,
    Falling  = 2,
    Both     = 3,
};

struct Config {
    bool enable_on_configure = true;
    bool start_immediately = false;
    std::uint32_t peripheral_clock_hz = 0u;
};

namespace detail {

// Trait presence helpers — avoid compound `if constexpr (A && B)` where B
// would be ill-formed when A is false. Each returns false if the member does
// not exist at all in the traits struct.

template <typename Traits>
consteval auto has_resolution_field() -> bool {
    if constexpr (requires { Traits::kResolutionField; }) {
        return Traits::kResolutionField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_alignment_field() -> bool {
    if constexpr (requires { Traits::kAlignField; }) {
        return Traits::kAlignField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_continuous_field() -> bool {
    if constexpr (requires { Traits::kContinuousField; }) {
        return Traits::kContinuousField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_stop_field() -> bool {
    if constexpr (requires { Traits::kStopField; }) {
        return Traits::kStopField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_sample_time_register() -> bool {
    if constexpr (requires { Traits::kSampleTimeRegister; }) {
        return Traits::kSampleTimeRegister.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_sample_time_pattern() -> bool {
    if constexpr (requires { Traits::kSampleTimePattern; }) {
        return Traits::kSampleTimePattern.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_channel_bit_pattern() -> bool {
    if constexpr (requires { Traits::kChannelBitPattern; }) {
        return Traits::kChannelBitPattern.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_channel_enable_pattern() -> bool {
    if constexpr (requires { Traits::kChannelEnablePattern; }) {
        return Traits::kChannelEnablePattern.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_channel_disable_pattern() -> bool {
    if constexpr (requires { Traits::kChannelDisablePattern; }) {
        return Traits::kChannelDisablePattern.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_channel_status_pattern() -> bool {
    if constexpr (requires { Traits::kChannelStatusPattern; }) {
        return Traits::kChannelStatusPattern.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_external_trigger_select_field() -> bool {
    if constexpr (requires { Traits::kExternalTriggerSelectField; }) {
        return Traits::kExternalTriggerSelectField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_external_trigger_enable_field() -> bool {
    if constexpr (requires { Traits::kExternalTriggerEnableField; }) {
        return Traits::kExternalTriggerEnableField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_end_of_conversion_field() -> bool {
    if constexpr (requires { Traits::kEndOfConversionField; }) {
        return Traits::kEndOfConversionField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_end_of_sequence_field() -> bool {
    if constexpr (requires { Traits::kEndOfSequenceField; }) {
        return Traits::kEndOfSequenceField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto has_overrun_field() -> bool {
    if constexpr (requires { Traits::kOverrunField; }) {
        return Traits::kOverrunField.valid;
    }
    return false;
}

template <typename Traits>
consteval auto result_bits() -> std::uint8_t {
    if constexpr (requires { Traits::kResultBits; }) {
        return static_cast<std::uint8_t>(Traits::kResultBits);
    }
    return 0u;
}

template <typename Traits>
consteval auto channel_count() -> std::size_t {
    if constexpr (requires { Traits::kChannelCount; }) {
        return static_cast<std::size_t>(Traits::kChannelCount);
    }
    return 0u;
}

}  // namespace detail

template <PeripheralId Peripheral>
class handle {
  public:
    using semantic_traits = device::AdcSemanticTraits<Peripheral>;
    using config_type = Config;
    using Channel = device::AdcChannel<Peripheral>;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    // ── Capability introspection ─────────────────────────────────────────────

    [[nodiscard]] static constexpr auto has_resolution() -> bool {
        return detail::has_resolution_field<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_alignment() -> bool {
        return detail::has_alignment_field<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_continuous() -> bool {
        return detail::has_continuous_field<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_sample_time() -> bool {
        return detail::has_sample_time_pattern<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_sequence() -> bool {
        return detail::has_channel_bit_pattern<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_channel_enable() -> bool {
        return detail::has_channel_enable_pattern<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_hardware_trigger() -> bool {
        return detail::has_external_trigger_select_field<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_end_of_sequence() -> bool {
        return detail::has_end_of_sequence_field<semantic_traits>();
    }
    [[nodiscard]] static constexpr auto has_overrun() -> bool {
        return detail::has_overrun_field<semantic_traits>();
    }

    // ── Constructor ──────────────────────────────────────────────────────────

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ── Lifecycle ────────────────────────────────────────────────────────────

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        if constexpr (Peripheral != device::PeripheralId::none) {
            if (const auto clk = hal::detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
                clk.is_err()) {
                return clk;
            }
        }
        // Software reset — required by peripherals like Microchip AFEC before
        // reconfiguring the mode register.
        if constexpr (requires { semantic_traits::kResetField; } &&
                      semantic_traits::kResetField.valid) {
            if (const auto r = hal::detail::runtime::modify_field(
                    semantic_traits::kResetField, 1u);
                r.is_err()) {
                return r;
            }
        }
        if constexpr (requires { semantic_traits::kPrescalerField; } &&
                      semantic_traits::kPrescalerField.valid &&
                      semantic_traits::kAdcMaxClockHz > 0u) {
            if (config_.peripheral_clock_hz > 0u) {
                const auto divisor = 2u * semantic_traits::kAdcMaxClockHz;
                const auto prescal = (config_.peripheral_clock_hz + divisor - 1u) / divisor - 1u;
                if (const auto r = hal::detail::runtime::modify_field(
                        semantic_traits::kPrescalerField, prescal);
                    r.is_err()) {
                    return r;
                }
            }
        }
        // Set required mode register bits (e.g., AFEC_MR.ONE must always be 1).
        if constexpr (requires { semantic_traits::kModeRegisterOneField; } &&
                      semantic_traits::kModeRegisterOneField.valid) {
            if (const auto r = hal::detail::runtime::modify_field(
                    semantic_traits::kModeRegisterOneField, 1u);
                r.is_err()) {
                return r;
            }
        }
        // Analog front-end bias current (e.g., AFEC_ACR.IBCTL must be non-zero).
        if constexpr (requires { semantic_traits::kAnalogControlField; } &&
                      semantic_traits::kAnalogControlField.valid &&
                      semantic_traits::kAnalogControlValue > 0u) {
            if (const auto r = hal::detail::runtime::modify_field(
                    semantic_traits::kAnalogControlField,
                    static_cast<std::uint32_t>(semantic_traits::kAnalogControlValue));
                r.is_err()) {
                return r;
            }
        }
        core::Result<void, core::ErrorCode> result = core::Ok();
        if (config_.enable_on_configure) {
            result = enable();
            if (!result.is_ok()) {
                return result;
            }
        }
        if (config_.start_immediately) {
            result = start();
        }
        return result;
    }

    [[nodiscard]] auto enable() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kEnableField.valid) {
            return hal::detail::runtime::modify_field(semantic_traits::kEnableField, 1u);
        }
        return core::Ok();
    }

    [[nodiscard]] auto start() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kStartField.valid) {
            return hal::detail::runtime::modify_field(semantic_traits::kStartField, 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto stop() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_stop_field<semantic_traits>()) {
            return hal::detail::runtime::modify_field(semantic_traits::kStopField, 1u);
        } else if constexpr (semantic_traits::kStartField.valid) {
            return hal::detail::runtime::modify_field(semantic_traits::kStartField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto ready() const -> bool {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kReadyField.valid) {
            const auto state = hal::detail::runtime::read_field(semantic_traits::kReadyField);
            return state.is_ok() && state.unwrap() != 0u;
        }
        return false;
    }

    [[nodiscard]] auto read() const -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kDataField.valid) {
            return hal::detail::runtime::read_field(semantic_traits::kDataField);
        }
        if constexpr (semantic_traits::kDataRegister.valid) {
            return hal::detail::runtime::read_register(semantic_traits::kDataRegister);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Resolution / alignment ───────────────────────────────────────────────

    [[nodiscard]] auto set_resolution(Resolution r) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_resolution_field<semantic_traits>()) {
            constexpr auto kMaxBits = detail::result_bits<semantic_traits>();
            if constexpr (kMaxBits > 0u) {
                if (k_resolution_bits(r) > kMaxBits) {
                    return core::Err(core::ErrorCode::OutOfRange);
                }
            }
            return hal::detail::runtime::modify_field(semantic_traits::kResolutionField,
                                                       static_cast<std::uint32_t>(r));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto set_alignment(Alignment a) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_alignment_field<semantic_traits>()) {
            return hal::detail::runtime::modify_field(semantic_traits::kAlignField,
                                                       static_cast<std::uint32_t>(a));
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Mode ─────────────────────────────────────────────────────────────────

    [[nodiscard]] auto set_continuous(bool enabled) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_continuous_field<semantic_traits>()) {
            return hal::detail::runtime::modify_field(semantic_traits::kContinuousField,
                                                       enabled ? 1u : 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Sample time ──────────────────────────────────────────────────────────

    [[nodiscard]] auto set_sample_time(std::uint8_t channel, std::uint32_t ticks) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_sample_time_register<semantic_traits>()) {
            if constexpr (detail::has_sample_time_pattern<semantic_traits>()) {
                return hal::detail::runtime::modify_indexed_field(
                    semantic_traits::kSampleTimePattern,
                    static_cast<std::size_t>(channel), ticks);
            }
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Sequence builder ─────────────────────────────────────────────────────

    [[nodiscard]] auto set_sequence(std::span<const std::uint8_t> channels) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_channel_bit_pattern<semantic_traits>()) {
            if (channels.empty()) {
                return core::Ok();
            }
            constexpr auto kMax = detail::channel_count<semantic_traits>();
            if constexpr (kMax > 0u) {
                if (channels.size() > kMax) {
                    return core::Err(core::ErrorCode::OutOfRange);
                }
            }
            for (std::size_t i = 0u; i < channels.size(); ++i) {
                const auto result = hal::detail::runtime::modify_indexed_field(
                    semantic_traits::kChannelBitPattern, i,
                    static_cast<std::uint32_t>(channels[i]));
                if (!result.is_ok()) {
                    return result;
                }
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Per-channel enable (Microchip AFEC) ──────────────────────────────────

    [[nodiscard]] auto enable_channel(std::uint8_t channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_channel_enable_pattern<semantic_traits>()) {
            return hal::detail::runtime::modify_indexed_field(
                semantic_traits::kChannelEnablePattern,
                static_cast<std::size_t>(channel), 1u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto disable_channel(std::uint8_t channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_channel_disable_pattern<semantic_traits>()) {
            return hal::detail::runtime::modify_indexed_field(
                semantic_traits::kChannelDisablePattern,
                static_cast<std::size_t>(channel), 1u);
        } else if constexpr (detail::has_channel_enable_pattern<semantic_traits>()) {
            return hal::detail::runtime::modify_indexed_field(
                semantic_traits::kChannelEnablePattern,
                static_cast<std::size_t>(channel), 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto channel_enabled(std::uint8_t channel) const -> bool {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_channel_status_pattern<semantic_traits>()) {
            const auto addr = hal::detail::runtime::indexed_field_address(
                semantic_traits::kChannelStatusPattern,
                static_cast<std::size_t>(channel));
            const auto mask = hal::detail::runtime::indexed_field_mask(
                semantic_traits::kChannelStatusPattern);
            return (hal::detail::runtime::read_mmio32(addr) & mask) != 0u;
        } else if constexpr (detail::has_channel_enable_pattern<semantic_traits>()) {
            const auto addr = hal::detail::runtime::indexed_field_address(
                semantic_traits::kChannelEnablePattern,
                static_cast<std::size_t>(channel));
            const auto mask = hal::detail::runtime::indexed_field_mask(
                semantic_traits::kChannelEnablePattern);
            return (hal::detail::runtime::read_mmio32(addr) & mask) != 0u;
        } else {
            return false;
        }
    }

    // ── Hardware trigger ─────────────────────────────────────────────────────

    [[nodiscard]] auto set_hardware_trigger(std::uint8_t source, TriggerEdge edge) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_external_trigger_select_field<semantic_traits>()) {
            auto result = hal::detail::runtime::modify_field(
                semantic_traits::kExternalTriggerSelectField,
                static_cast<std::uint32_t>(source));
            if (!result.is_ok()) {
                return result;
            }
            if constexpr (detail::has_external_trigger_enable_field<semantic_traits>()) {
                return hal::detail::runtime::modify_field(
                    semantic_traits::kExternalTriggerEnableField,
                    static_cast<std::uint32_t>(edge));
            }
            return result;
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Sequence read (polling, no DMA) ──────────────────────────────────────

    [[nodiscard]] auto read_sequence(std::span<std::uint16_t> samples) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_end_of_conversion_field<semantic_traits>() &&
                      semantic_traits::kDataField.valid) {
            constexpr std::uint32_t kPollLimit = 1'000'000u;
            for (auto& sample : samples) {
                std::uint32_t attempts = 0u;
                while (true) {
                    if constexpr (detail::has_overrun_field<semantic_traits>()) {
                        const auto ovr =
                            hal::detail::runtime::read_field(semantic_traits::kOverrunField);
                        if (ovr.is_ok() && ovr.unwrap() != 0u) {
                            return core::Err(core::ErrorCode::AdcOverrun);
                        }
                    }
                    const auto eoc =
                        hal::detail::runtime::read_field(semantic_traits::kEndOfConversionField);
                    if (eoc.is_ok() && eoc.unwrap() != 0u) {
                        break;
                    }
                    if (++attempts >= kPollLimit) {
                        return core::Err(core::ErrorCode::AdcConversionTimeout);
                    }
                }
                const auto data = hal::detail::runtime::read_field(semantic_traits::kDataField);
                if (!data.is_ok()) {
                    return core::Err(core::ErrorCode{data.err()});
                }
                sample = static_cast<std::uint16_t>(data.unwrap());
            }
            return core::Ok();
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    // ── Status ───────────────────────────────────────────────────────────────

    [[nodiscard]] auto end_of_sequence() const -> bool {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_end_of_sequence_field<semantic_traits>()) {
            const auto state =
                hal::detail::runtime::read_field(semantic_traits::kEndOfSequenceField);
            return state.is_ok() && state.unwrap() != 0u;
        }
        return false;
    }

    [[nodiscard]] auto overrun() const -> bool {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_overrun_field<semantic_traits>()) {
            const auto state =
                hal::detail::runtime::read_field(semantic_traits::kOverrunField);
            return state.is_ok() && state.unwrap() != 0u;
        }
        return false;
    }

    [[nodiscard]] auto clear_overrun() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (detail::has_overrun_field<semantic_traits>()) {
            return hal::detail::runtime::modify_field(semantic_traits::kOverrunField, 0u);
        }
        return core::Ok();
    }

    // ── DMA ──────────────────────────────────────────────────────────────────

    [[nodiscard]] auto enable_dma(bool circular = false) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kDmaEnableField.valid) {
            auto result =
                hal::detail::runtime::modify_field(semantic_traits::kDmaEnableField, 1u);
            if (!result.is_ok()) {
                return result;
            }
            if constexpr (semantic_traits::kDmaModeField.valid) {
                return hal::detail::runtime::modify_field(semantic_traits::kDmaModeField,
                                                           circular ? 1u : 0u);
            }
            return result;
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    [[nodiscard]] auto disable_dma() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested ADC is not published for the selected device.");

        if constexpr (semantic_traits::kDmaEnableField.valid) {
            return hal::detail::runtime::modify_field(semantic_traits::kDmaEnableField, 0u);
        }
        return core::Err(core::ErrorCode::NotSupported);
    }

    template <typename DmaChannel>
    [[nodiscard]] auto configure_dma(const DmaChannel& channel, bool circular = false) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(DmaChannel::valid);
        static_assert(DmaChannel::peripheral_id == peripheral_id);
        static_assert(DmaChannel::signal_id == alloy::hal::dma::SignalId::signal_RX);

        const auto dma_result = channel.configure();
        if (!dma_result.is_ok()) {
            return dma_result;
        }
        return enable_dma(circular);
    }

    [[nodiscard]] static constexpr auto data_register_address() -> std::uintptr_t {
        if constexpr (semantic_traits::kDataRegister.valid) {
            return semantic_traits::kDataRegister.base_address +
                   semantic_traits::kDataRegister.offset_bytes;
        } else if constexpr (semantic_traits::kDataField.valid) {
            return semantic_traits::kDataField.reg.base_address +
                   semantic_traits::kDataField.reg.offset_bytes;
        } else {
            return 0u;
        }
    }

    // ── Async adapter interface ───────────────────────────────────────────────
    // Called by runtime::async::adc::read / scan_dma.

    [[nodiscard]] auto read_async() const -> core::Result<void, core::ErrorCode> {
        return start();
    }

    template <typename DmaChannel>
    [[nodiscard]] auto scan_dma_async(const DmaChannel& channel,
                                       std::span<std::uint16_t>) const
        -> core::Result<void, core::ErrorCode> {
        return configure_dma(channel, true);
    }

   private:
    Config config_{};

    [[nodiscard]] static constexpr auto k_resolution_bits(Resolution r) -> std::uint8_t {
        constexpr std::uint8_t kMap[] = {6u, 8u, 10u, 12u, 14u, 16u};
        const auto idx = static_cast<std::uint8_t>(r);
        return idx < 6u ? kMap[idx] : 0u;
    }
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested ADC is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::adc

// alloy.device.v2.1 concept-based ADC — no descriptor-runtime required.
// Supports:
//   StSimpleAdc (kIpVersion starts with "aditf4"): F0 / G0 simplified ADC.
//   StModernAdc (kIpVersion contains "aditf5"): G4 / H7 full ADC.
// NOT supported: legacy F1 / F2 / F4 ADC (kIpVersion "aditf2_v…").
#include "hal/adc/lite.hpp"
