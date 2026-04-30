#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::timer {

#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
using PeripheralId = device::PeripheralId;

// ---- Enumerations ---------------------------------------------------------

enum class CountDirection : std::uint8_t { Up, Down };

enum class CenterAligned : std::uint8_t { Disabled, Mode1, Mode2, Mode3 };

// ---- Dead-time config -----------------------------------------------------

/// High-level dead-time configuration for advanced timers.
struct DeadTimeConfig {
    std::uint8_t dtg        = 0u;    ///< Raw DTG[7:0] value — use ns_to_dtg() to compute.
    bool enable_moe         = true;  ///< Assert MOE (main output enable) after configuration.
    bool auto_output_enable = false; ///< AOE: re-enable outputs automatically after break.
};

// ---- Break input config ---------------------------------------------------

/// High-level break-input configuration for advanced timers.
struct BreakConfig {
    bool enabled       = false; ///< BKE: assert break enable.
    bool active_high   = true;  ///< BKP: false = active-low.
    bool auto_rearm    = false; ///< AOE: automatic output re-enable after break clears.
};

enum class CaptureCompareMode : std::uint8_t {
    Frozen,
    Active,
    Inactive,
    Toggle,
    ForceLow,
    ForceHigh,
    Pwm1,
    Pwm2,
};

enum class EncoderMode : std::uint8_t { Disabled, Channel1, Channel2, BothChannels };

enum class Polarity : std::uint8_t { Active, Inverted };

enum class InterruptKind : std::uint8_t {
    Update,
    ChannelCompare,
    Trigger,
    Break,
    Update_DMA,
    Channel_DMA,
    Trigger_DMA,
};

// ---- Config ---------------------------------------------------------------

struct Config {
    std::uint32_t period = 0u;
    std::uint32_t peripheral_clock_hz = 0u;
    bool apply_period = false;
    bool start_immediately = false;
};

// ---- handle ---------------------------------------------------------------

template <PeripheralId Peripheral>
class handle {
   public:
    using semantic_traits = device::TimerSemanticTraits<Peripheral>;
    using config_type = Config;

    static constexpr auto peripheral_id = Peripheral;
    static constexpr bool valid = semantic_traits::kPresent;

    constexpr explicit handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    // ---- static accessors -------------------------------------------------

    /// NVIC IRQ line numbers for this peripheral (may be empty).
    [[nodiscard]] static constexpr auto irq_numbers() -> std::span<const std::uint32_t> {
        return std::span<const std::uint32_t>{semantic_traits::kIrqNumbers};
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
        if (config_.start_immediately) {
            result = start();
        }
        return result;
    }

    // ---- start / stop / running -------------------------------------------

    [[nodiscard]] auto start() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kStartField.valid) {
            return detail::runtime::modify_field(semantic_traits::kStartField, 1u);
        } else if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 1u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto stop() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kStopField.valid) {
            return detail::runtime::modify_field(semantic_traits::kStopField, 1u);
        } else if constexpr (semantic_traits::kDisableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDisableField, 1u);
        } else if constexpr (semantic_traits::kEnableField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEnableField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto is_running() const -> bool {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kEnableField.valid) {
            const auto state = detail::runtime::read_field(semantic_traits::kEnableField);
            return state.is_ok() && state.unwrap() != 0u;
        } else {
            return false;
        }
    }

    // ---- period (raw) / counter -------------------------------------------

    [[nodiscard]] auto set_period(std::uint32_t period) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kPeriodField.valid) {
            constexpr bool kDirectRegisterWrite = semantic_traits::kPeriodField.field_id ==
                                                      device::FieldId::none &&
                                                  semantic_traits::kPeriodField.bit_offset == 0u;
            if constexpr (kDirectRegisterWrite) {
                const auto bits = detail::runtime::field_bits(semantic_traits::kPeriodField, period);
                if (bits.is_err()) {
                    return core::Err(core::ErrorCode{bits.unwrap_err()});
                }
                return detail::runtime::write_register(semantic_traits::kPeriodField.reg,
                                                       bits.unwrap());
            }
            return detail::runtime::modify_field(semantic_traits::kPeriodField, period);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto get_count() const -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kCounterRegister.valid) {
            return detail::runtime::read_register(semantic_traits::kCounterRegister);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    // ---- Phase 1: prescaler / frequency / counter mode -------------------

    /// Writes PSC register directly (raw value, not PSC+1).
    [[nodiscard]] auto set_prescaler(std::uint16_t prescaler) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kPrescalerField.valid) {
            return detail::runtime::modify_field(semantic_traits::kPrescalerField,
                                                 static_cast<std::uint32_t>(prescaler));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Resolves PSC+ARR from peripheral_clock_hz / hz; validates ±2 %.
    /// Requires config.peripheral_clock_hz != 0 and published kMaxAutoReload > 0.
    [[nodiscard]] auto set_frequency(std::uint32_t hz) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (!semantic_traits::kPrescalerField.valid || !semantic_traits::kPeriodField.valid ||
                      semantic_traits::kMaxAutoReload == 0u || semantic_traits::kMaxPrescaler == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            if (hz == 0u || config_.peripheral_clock_hz == 0u) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            const std::uint64_t clk = config_.peripheral_clock_hz;
            const std::uint64_t max_arr = semantic_traits::kMaxAutoReload;
            const std::uint64_t max_psc = semantic_traits::kMaxPrescaler;
            // total ticks per period (approximate, integer)
            const std::uint64_t total = clk / hz;
            if (total == 0u) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            // minimum PSC+1: ceil(total / (max_arr+1))
            const std::uint64_t psc_plus1 = (total + max_arr) / (max_arr + 1u);
            if (psc_plus1 == 0u || (psc_plus1 - 1u) > max_psc) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            const std::uint64_t arr_plus1 = total / psc_plus1;
            if (arr_plus1 == 0u || arr_plus1 > max_arr + 1u) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            // ±2 % validation
            const std::uint64_t realized = clk / (psc_plus1 * arr_plus1);
            const std::uint64_t diff =
                (realized >= hz) ? (realized - hz) : (static_cast<std::uint64_t>(hz) - realized);
            if (diff * 100u > static_cast<std::uint64_t>(hz) * 2u) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            // write PSC
            auto r = detail::runtime::modify_field(semantic_traits::kPrescalerField,
                                                   static_cast<std::uint32_t>(psc_plus1 - 1u));
            if (r.is_err()) {
                return r;
            }
            // write ARR (period register holds ARR = arr_plus1 - 1)
            r = detail::runtime::modify_field(semantic_traits::kPeriodField,
                                              static_cast<std::uint32_t>(arr_plus1 - 1u));
            if (r.is_err()) {
                return r;
            }
            realized_hz_ = static_cast<std::uint32_t>(realized);
            return core::Ok();
        }
    }

    /// Returns the last realised frequency set by set_frequency(); 0 if not yet called.
    [[nodiscard]] auto frequency() const noexcept -> std::uint32_t { return realized_hz_; }

    [[nodiscard]] auto set_direction(CountDirection dir) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kDirectionField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDirectionField,
                                                 static_cast<std::uint32_t>(dir));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_center_aligned(CenterAligned mode) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasCenterAligned &&
                      semantic_traits::kCenterAlignedField.valid) {
            return detail::runtime::modify_field(semantic_traits::kCenterAlignedField,
                                                 static_cast<std::uint32_t>(mode));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_one_pulse(bool enable) const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasOnePulse && semantic_traits::kOnePulseField.valid) {
            return detail::runtime::modify_field(semantic_traits::kOnePulseField,
                                                 enable ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Enables or disables auto-reload preload (ARR shadow register / ARPE bit).
    [[nodiscard]] auto set_auto_reload_preload(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kAutoReloadPreloadField.valid) {
            return detail::runtime::modify_field(semantic_traits::kAutoReloadPreloadField,
                                                 enable ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Enables or disables output-compare preload (OCxPE) on all channels simultaneously.
    [[nodiscard]] auto set_period_preload(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kChannelCount == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            bool any_supported = false;
            core::Result<void, core::ErrorCode> result = core::Ok();
            const std::uint32_t val = enable ? 1u : 0u;
            auto apply = [&]<std::size_t N>() {
                using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                if constexpr (ch_traits::kPreloadField.valid) {
                    any_supported = true;
                    if (result.is_ok()) {
                        result = detail::runtime::modify_field(ch_traits::kPreloadField, val);
                    }
                }
            };
            if constexpr (semantic_traits::kChannelCount >= 1u) {
                apply.template operator()<0u>();
            }
            if constexpr (semantic_traits::kChannelCount >= 2u) {
                apply.template operator()<1u>();
            }
            if constexpr (semantic_traits::kChannelCount >= 3u) {
                apply.template operator()<2u>();
            }
            if constexpr (semantic_traits::kChannelCount >= 4u) {
                apply.template operator()<3u>();
            }
            if (!any_supported) {
                return core::Err(core::ErrorCode::NotSupported);
            }
            return result;
        }
    }

    // ---- Phase 2: capture / compare channels ------------------------------

    [[nodiscard]] auto set_channel_mode(std::uint8_t channel, CaptureCompareMode mode) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasCompare || semantic_traits::kHasCapture) {
            if constexpr (semantic_traits::kChannelCount == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            } else {
                if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                auto do_set = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                    using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                    if constexpr (ch_traits::kModeField.valid) {
                        return detail::runtime::modify_field(
                            ch_traits::kModeField, static_cast<std::uint32_t>(mode));
                    } else {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                };
                if constexpr (semantic_traits::kChannelCount >= 4u) {
                    if (channel == 3u) {
                        return do_set.template operator()<3u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 3u) {
                    if (channel == 2u) {
                        return do_set.template operator()<2u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 2u) {
                    if (channel == 1u) {
                        return do_set.template operator()<1u>();
                    }
                }
                return do_set.template operator()<0u>();
            }
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_compare_value(std::uint8_t channel, std::uint32_t value) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasCompare) {
            if constexpr (semantic_traits::kChannelCount == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            } else {
                if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                auto do_set = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                    using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                    if constexpr (ch_traits::kCompareRegister.valid) {
                        return detail::runtime::write_register(ch_traits::kCompareRegister, value);
                    } else {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                };
                if constexpr (semantic_traits::kChannelCount >= 4u) {
                    if (channel == 3u) {
                        return do_set.template operator()<3u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 3u) {
                    if (channel == 2u) {
                        return do_set.template operator()<2u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 2u) {
                    if (channel == 1u) {
                        return do_set.template operator()<1u>();
                    }
                }
                return do_set.template operator()<0u>();
            }
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto read_capture_value(std::uint8_t channel) const
        -> core::Result<std::uint32_t, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasCapture) {
            if constexpr (semantic_traits::kChannelCount == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            } else {
                if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                auto do_read = [&]<std::size_t N>()
                    -> core::Result<std::uint32_t, core::ErrorCode> {
                    using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                    if constexpr (ch_traits::kCaptureRegister.valid) {
                        return detail::runtime::read_register(ch_traits::kCaptureRegister);
                    } else {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                };
                if constexpr (semantic_traits::kChannelCount >= 4u) {
                    if (channel == 3u) {
                        return do_read.template operator()<3u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 3u) {
                    if (channel == 2u) {
                        return do_read.template operator()<2u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 2u) {
                    if (channel == 1u) {
                        return do_read.template operator()<1u>();
                    }
                }
                return do_read.template operator()<0u>();
            }
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto enable_channel_output(std::uint8_t channel, bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasCompare || semantic_traits::kHasCapture) {
            if constexpr (semantic_traits::kChannelCount == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            } else {
                if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                auto do_set = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                    using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                    if constexpr (ch_traits::kOutputEnableField.valid) {
                        return detail::runtime::modify_field(ch_traits::kOutputEnableField,
                                                             enable ? 1u : 0u);
                    } else {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                };
                if constexpr (semantic_traits::kChannelCount >= 4u) {
                    if (channel == 3u) {
                        return do_set.template operator()<3u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 3u) {
                    if (channel == 2u) {
                        return do_set.template operator()<2u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 2u) {
                    if (channel == 1u) {
                        return do_set.template operator()<1u>();
                    }
                }
                return do_set.template operator()<0u>();
            }
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_channel_output_polarity(std::uint8_t channel,
                                                   Polarity polarity) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasCompare || semantic_traits::kHasCapture) {
            if constexpr (semantic_traits::kChannelCount == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            } else {
                if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                auto do_set = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                    using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                    if constexpr (ch_traits::kOutputPolarityField.valid) {
                        return detail::runtime::modify_field(
                            ch_traits::kOutputPolarityField,
                            static_cast<std::uint32_t>(polarity));
                    } else {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                };
                if constexpr (semantic_traits::kChannelCount >= 4u) {
                    if (channel == 3u) {
                        return do_set.template operator()<3u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 3u) {
                    if (channel == 2u) {
                        return do_set.template operator()<2u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 2u) {
                    if (channel == 1u) {
                        return do_set.template operator()<1u>();
                    }
                }
                return do_set.template operator()<0u>();
            }
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    // ---- Phase 2: complementary outputs -----------------------------------

    [[nodiscard]] auto enable_complementary_output(std::uint8_t channel, bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasComplementaryOutputs) {
            if constexpr (semantic_traits::kChannelCount == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            } else {
                if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                auto do_set = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                    using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                    if constexpr (ch_traits::kComplementaryOutputEnableField.valid) {
                        return detail::runtime::modify_field(
                            ch_traits::kComplementaryOutputEnableField, enable ? 1u : 0u);
                    } else {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                };
                if constexpr (semantic_traits::kChannelCount >= 4u) {
                    if (channel == 3u) {
                        return do_set.template operator()<3u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 3u) {
                    if (channel == 2u) {
                        return do_set.template operator()<2u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 2u) {
                    if (channel == 1u) {
                        return do_set.template operator()<1u>();
                    }
                }
                return do_set.template operator()<0u>();
            }
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Set complementary output polarity (CCER.CCxNP).
    [[nodiscard]] auto set_complementary_polarity(std::uint8_t channel,
                                                  Polarity polarity) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (!semantic_traits::kHasComplementaryOutputs ||
                      semantic_traits::kChannelCount == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            auto do_set = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                if constexpr (ch_traits::kComplementaryOutputPolarityField.valid) {
                    return detail::runtime::modify_field(
                        ch_traits::kComplementaryOutputPolarityField,
                        static_cast<std::uint32_t>(polarity));
                } else {
                    return core::Err(core::ErrorCode::NotSupported);
                }
            };
            if constexpr (semantic_traits::kChannelCount >= 4u) {
                if (channel == 3u) {
                    return do_set.template operator()<3u>();
                }
            }
            if constexpr (semantic_traits::kChannelCount >= 3u) {
                if (channel == 2u) {
                    return do_set.template operator()<2u>();
                }
            }
            if constexpr (semantic_traits::kChannelCount >= 2u) {
                if (channel == 1u) {
                    return do_set.template operator()<1u>();
                }
            }
            return do_set.template operator()<0u>();
        }
    }

    // ---- Phase 3: dead-time / break / main output -------------------------

    /// Convert nanoseconds to DTG[7:0] for the given timer clock.
    ///
    /// Uses ceil(ns * clk_hz / 1e9) to guarantee the requested minimum dead-time.
    /// Implements all four STM32 BDTR.DTG encoding ranges:
    ///   0b0xxxxxxx  — DT = tDTS * DTG[6:0]          (1–127 ticks, up to ~1.98 µs @64MHz)
    ///   0b10xxxxxx  — DT = tDTS * (64+n) * 2         (n∈[0,63], up to ~3.97 µs @64MHz)
    ///   0b110xxxxx  — DT = tDTS * (32+n) * 8         (n∈[0,31], up to ~7.875 µs @64MHz)
    ///   0b111xxxxx  — DT = tDTS * (32+n) * 16        (n∈[0,31], up to ~15.75 µs @64MHz)
    /// Returns 0xFF (= 255 = maximum valid DTG) if ns exceeds the encodable range.
    /// This coincides with the maximum legal DTG value (range 3, n=31), so the caller
    /// should validate that the returned encoding meets their dead-time requirement.
    [[nodiscard]] static constexpr auto ns_to_dtg(std::uint32_t ns,
                                                   std::uint32_t timer_clk_hz) -> std::uint8_t {
        if (timer_clk_hz == 0u || ns == 0u) {
            return 0u;
        }
        const std::uint64_t clk = timer_clk_hz;
        // dt_clk = ceil(ns * clk / 1e9)  — guaranteed minimum dead-time in ticks
        const std::uint64_t dt_clk =
            (static_cast<std::uint64_t>(ns) * clk + 999'999'999u) / 1'000'000'000u;

        // Range 0: DT = dt_clk * 1,  code = 0b0xxxxxxx  (dt_clk ∈ [0, 127])
        if (dt_clk <= 127u) {
            return static_cast<std::uint8_t>(dt_clk);
        }
        // Range 1: DT = (64+n) * 2,  code = 0b10nnnnnn  (n ∈ [0, 63])
        {
            const std::uint64_t half = (dt_clk + 1u) / 2u;  // ceil(dt_clk / 2)
            if (half >= 64u && half <= 127u) {
                return static_cast<std::uint8_t>(0b10000000u | static_cast<std::uint8_t>(half - 64u));
            }
        }
        // Range 2: DT = (32+n) * 8,  code = 0b110nnnnn  (n ∈ [0, 31])
        {
            const std::uint64_t eighth = (dt_clk + 7u) / 8u;  // ceil(dt_clk / 8)
            if (eighth >= 32u && eighth <= 63u) {
                return static_cast<std::uint8_t>(0b11000000u | static_cast<std::uint8_t>(eighth - 32u));
            }
        }
        // Range 3: DT = (32+n) * 16, code = 0b111nnnnn  (n ∈ [0, 31])
        {
            const std::uint64_t sixteenth = (dt_clk + 15u) / 16u;  // ceil(dt_clk / 16)
            if (sixteenth >= 32u && sixteenth <= 63u) {
                return static_cast<std::uint8_t>(0b11100000u | static_cast<std::uint8_t>(sixteenth - 32u));
            }
        }
        return 0xFFu;  // ns exceeds maximum encodable dead-time
    }

    /// Write DTG[7:0] directly (raw register encoding).
    [[nodiscard]] auto set_dead_time(std::uint8_t dtg_raw) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kDtgField.valid) {
            return detail::runtime::modify_field(semantic_traits::kDtgField,
                                                 static_cast<std::uint32_t>(dtg_raw));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// High-level dead-time configuration: sets DTG, AOE, and optionally MOE.
    [[nodiscard]] auto configure_dead_time(const DeadTimeConfig& cfg) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (!semantic_traits::kDtgField.valid || !semantic_traits::kMoeField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            if (auto r = detail::runtime::modify_field(semantic_traits::kDtgField,
                                                        static_cast<std::uint32_t>(cfg.dtg));
                r.is_err()) {
                return r;
            }
            if constexpr (semantic_traits::kAoeField.valid) {
                if (auto r = detail::runtime::modify_field(semantic_traits::kAoeField,
                                                            cfg.auto_output_enable ? 1u : 0u);
                    r.is_err()) {
                    return r;
                }
            }
            if (cfg.enable_moe) {
                return detail::runtime::modify_field(semantic_traits::kMoeField, 1u);
            }
            return core::Ok();
        }
    }

    /// Enable or disable the main output (BDTR.MOE).
    [[nodiscard]] auto enable_main_output(bool enable = true) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kMoeField.valid) {
            return detail::runtime::modify_field(semantic_traits::kMoeField, enable ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Disable the main output (BDTR.MOE = 0).  Alias for enable_main_output(false).
    [[nodiscard]] auto disable_main_output() const -> core::Result<void, core::ErrorCode> {
        return enable_main_output(false);
    }

    /// Enable or disable the break input (BDTR.BKE).
    [[nodiscard]] auto enable_break_input(bool enable) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kBkeField.valid) {
            return detail::runtime::modify_field(semantic_traits::kBkeField, enable ? 1u : 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Set break input polarity (BDTR.BKP).
    [[nodiscard]] auto set_break_polarity(Polarity polarity) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kBkpField.valid) {
            // Active = 0 (active-low), Inverted = 1 (active-high on STM32).
            return detail::runtime::modify_field(semantic_traits::kBkpField,
                                                 static_cast<std::uint32_t>(polarity));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// High-level break-input configuration: BKE, BKP, AOE in one call.
    [[nodiscard]] auto configure_break(const BreakConfig& cfg) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (!semantic_traits::kBkeField.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            if (auto r = detail::runtime::modify_field(semantic_traits::kBkeField,
                                                        cfg.enabled ? 1u : 0u);
                r.is_err()) {
                return r;
            }
            if constexpr (semantic_traits::kBkpField.valid) {
                if (auto r = detail::runtime::modify_field(semantic_traits::kBkpField,
                                                            cfg.active_high ? 1u : 0u);
                    r.is_err()) {
                    return r;
                }
            }
            if constexpr (semantic_traits::kAoeField.valid) {
                return detail::runtime::modify_field(semantic_traits::kAoeField,
                                                     cfg.auto_rearm ? 1u : 0u);
            }
            return core::Ok();
        }
    }

    /// True if the break flag (SR.BIF) is set.
    [[nodiscard]] auto break_active() const -> bool {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kBreakFlagField.valid) {
            const auto val = detail::runtime::read_field(semantic_traits::kBreakFlagField);
            return val.is_ok() && val.unwrap() != 0u;
        } else {
            return false;
        }
    }

    /// Clear the break interrupt flag (SR.BIF ← 0).
    [[nodiscard]] auto clear_break_flag() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kBreakFlagField.valid) {
            return detail::runtime::modify_field(semantic_traits::kBreakFlagField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto set_encoder_mode(EncoderMode mode) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasEncoder) {
            if constexpr (semantic_traits::kEncoderModeField.valid) {
                // STM32 path: SMS field — 0=off, 1=ch1, 2=ch2, 3=both
                return detail::runtime::modify_field(semantic_traits::kEncoderModeField,
                                                     static_cast<std::uint32_t>(mode));
            } else if constexpr (semantic_traits::kEncoderEnableField.valid) {
                // Microchip TC path: QDEN enable bit
                const std::uint32_t en = (mode != EncoderMode::Disabled) ? 1u : 0u;
                return detail::runtime::modify_field(semantic_traits::kEncoderEnableField, en);
            } else {
                return core::Err(core::ErrorCode::NotSupported);
            }
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    /// Encoder polarity via kEncoderPhaseEdgeField (Microchip EDGPHA); NotSupported on STM32.
    [[nodiscard]] auto set_encoder_polarity(Polarity polarity) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kHasEncoder &&
                      semantic_traits::kEncoderPhaseEdgeField.valid) {
            return detail::runtime::modify_field(semantic_traits::kEncoderPhaseEdgeField,
                                                 static_cast<std::uint32_t>(polarity));
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    // ---- Phase 4: status / events -----------------------------------------

    [[nodiscard]] auto update_event() const -> bool {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kUpdateFlagField.valid) {
            const auto val = detail::runtime::read_field(semantic_traits::kUpdateFlagField);
            return val.is_ok() && val.unwrap() != 0u;
        } else {
            return false;
        }
    }

    [[nodiscard]] auto clear_update_event() const -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kUpdateFlagField.valid) {
            // STM32: SR.UIF cleared by writing 0; SAME70: similar
            return detail::runtime::modify_field(semantic_traits::kUpdateFlagField, 0u);
        } else {
            return core::Err(core::ErrorCode::NotSupported);
        }
    }

    [[nodiscard]] auto channel_event(std::uint8_t channel) const -> bool {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kChannelCount == 0u) {
            return false;
        } else {
            if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                return false;
            }
            auto do_read = [&]<std::size_t N>() -> bool {
                using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                if constexpr (ch_traits::kInterruptFlagField.valid) {
                    const auto val =
                        detail::runtime::read_field(ch_traits::kInterruptFlagField);
                    return val.is_ok() && val.unwrap() != 0u;
                } else {
                    return false;
                }
            };
            if constexpr (semantic_traits::kChannelCount >= 4u) {
                if (channel == 3u) {
                    return do_read.template operator()<3u>();
                }
            }
            if constexpr (semantic_traits::kChannelCount >= 3u) {
                if (channel == 2u) {
                    return do_read.template operator()<2u>();
                }
            }
            if constexpr (semantic_traits::kChannelCount >= 2u) {
                if (channel == 1u) {
                    return do_read.template operator()<1u>();
                }
            }
            return do_read.template operator()<0u>();
        }
    }

    [[nodiscard]] auto clear_channel_event(std::uint8_t channel) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        if constexpr (semantic_traits::kChannelCount == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        } else {
            if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            auto do_clr = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                if constexpr (ch_traits::kInterruptFlagField.valid) {
                    return detail::runtime::modify_field(ch_traits::kInterruptFlagField, 0u);
                } else {
                    return core::Err(core::ErrorCode::NotSupported);
                }
            };
            if constexpr (semantic_traits::kChannelCount >= 4u) {
                if (channel == 3u) {
                    return do_clr.template operator()<3u>();
                }
            }
            if constexpr (semantic_traits::kChannelCount >= 3u) {
                if (channel == 2u) {
                    return do_clr.template operator()<2u>();
                }
            }
            if constexpr (semantic_traits::kChannelCount >= 2u) {
                if (channel == 1u) {
                    return do_clr.template operator()<1u>();
                }
            }
            return do_clr.template operator()<0u>();
        }
    }

    // ---- Phase 4: interrupts ----------------------------------------------

    [[nodiscard]] auto enable_interrupt(InterruptKind kind,
                                        std::uint8_t channel = 0u) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        return interrupt_arm_(kind, channel, true);
    }

    [[nodiscard]] auto disable_interrupt(InterruptKind kind,
                                         std::uint8_t channel = 0u) const
        -> core::Result<void, core::ErrorCode> {
        static_assert(valid, "Requested timer is not published for the selected device.");
        return interrupt_arm_(kind, channel, false);
    }

   private:
    Config config_{};
    mutable std::uint32_t realized_hz_ = 0u;

    [[nodiscard]] auto interrupt_arm_(InterruptKind kind,
                                      std::uint8_t channel,
                                      bool enable) const
        -> core::Result<void, core::ErrorCode> {
        const std::uint32_t val = enable ? 1u : 0u;
        if (kind == InterruptKind::Update) {
            if constexpr (semantic_traits::kUpdateInterruptEnableField.valid) {
                return detail::runtime::modify_field(
                    semantic_traits::kUpdateInterruptEnableField, val);
            } else {
                return core::Err(core::ErrorCode::NotSupported);
            }
        }
        if (kind == InterruptKind::ChannelCompare) {
            if constexpr (semantic_traits::kChannelCount == 0u) {
                return core::Err(core::ErrorCode::NotSupported);
            } else {
                if (channel >= static_cast<std::uint8_t>(semantic_traits::kChannelCount)) {
                    return core::Err(core::ErrorCode::InvalidParameter);
                }
                auto do_arm = [&]<std::size_t N>() -> core::Result<void, core::ErrorCode> {
                    using ch_traits = device::TimerChannelSemanticTraits<Peripheral, N>;
                    if constexpr (ch_traits::kInterruptEnableField.valid) {
                        return detail::runtime::modify_field(ch_traits::kInterruptEnableField,
                                                             val);
                    } else {
                        return core::Err(core::ErrorCode::NotSupported);
                    }
                };
                if constexpr (semantic_traits::kChannelCount >= 4u) {
                    if (channel == 3u) {
                        return do_arm.template operator()<3u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 3u) {
                    if (channel == 2u) {
                        return do_arm.template operator()<2u>();
                    }
                }
                if constexpr (semantic_traits::kChannelCount >= 2u) {
                    if (channel == 1u) {
                        return do_arm.template operator()<1u>();
                    }
                }
                return do_arm.template operator()<0u>();
            }
        }
        // Trigger, Break, *_DMA: field refs not yet published in device database.
        return core::Err(core::ErrorCode::NotSupported);
    }
};

template <PeripheralId Peripheral>
[[nodiscard]] constexpr auto open(Config config = {}) -> handle<Peripheral> {
    static_assert(handle<Peripheral>::valid,
                  "Requested timer is not published for the selected device.");
    return handle<Peripheral>{config};
}
#endif

}  // namespace alloy::hal::timer
