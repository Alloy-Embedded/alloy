#include <array>
#include <cstdint>
#include <span>

#include "hal/adc.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::adc::PeripheralId;

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
using Adc = alloy::hal::adc::handle<PeripheralId::ADC1>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using Adc = alloy::hal::adc::handle<PeripheralId::ADC1>;
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
using Adc = alloy::hal::adc::handle<PeripheralId::AFEC0>;
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
using Adc = alloy::hal::adc::handle<PeripheralId::ADC>;
#endif

#if !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
static_assert(Adc::valid);

// ── Capability introspection (task 3.3) ─────────────────────────────────────
static_assert(std::is_same_v<decltype(Adc::has_resolution()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_alignment()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_continuous()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_sample_time()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_sequence()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_channel_enable()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_hardware_trigger()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_end_of_sequence()), bool>);
static_assert(std::is_same_v<decltype(Adc::has_overrun()), bool>);
#endif

// ── Per-channel enable: SAME70 AFEC uses kChannelEnablePattern (task 3.2) ───
#if (defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED))
static_assert(Adc::has_channel_enable(),
              "AFEC0 on SAME70 must publish kChannelEnablePattern");
#endif
#endif

int main() {
#if ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE && !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    auto adc = alloy::hal::adc::open<Adc::peripheral_id>(
        alloy::hal::adc::Config{.enable_on_configure = true});

    // ── Original methods ─────────────────────────────────────────────────────
    [[maybe_unused]] const auto configure_result = adc.configure();
    [[maybe_unused]] const auto enable_result    = adc.enable();
    [[maybe_unused]] const auto start_result     = adc.start();
    [[maybe_unused]] const auto ready            = adc.ready();
    [[maybe_unused]] const auto read_result      = adc.read();

    // ── Task 2.2: stop ───────────────────────────────────────────────────────
    [[maybe_unused]] const auto stop_result = adc.stop();

    // ── Task 2.1: resolution / alignment ────────────────────────────────────
    [[maybe_unused]] const auto res_result =
        adc.set_resolution(alloy::hal::adc::Resolution::Bits12);
    [[maybe_unused]] const auto align_result =
        adc.set_alignment(alloy::hal::adc::Alignment::Right);

    // ── Task 2.2: continuous mode ────────────────────────────────────────────
    [[maybe_unused]] const auto cont_result = adc.set_continuous(true);

    // ── Task 2.3: sample time ────────────────────────────────────────────────
    [[maybe_unused]] const auto smp_result = adc.set_sample_time(0u, 160u);

    // ── Task 2.4: sequence builder ───────────────────────────────────────────
    constexpr std::array<std::uint8_t, 3> kChannels = {0u, 1u, 4u};
    [[maybe_unused]] const auto seq_result =
        adc.set_sequence(std::span<const std::uint8_t>{kChannels});

    // ── Task 2.5: per-channel enable (SAME70 / generic) ──────────────────────
    [[maybe_unused]] const auto en_ch_result   = adc.enable_channel(0u);
    [[maybe_unused]] const auto dis_ch_result  = adc.disable_channel(0u);
    [[maybe_unused]] const auto ch_en          = adc.channel_enabled(0u);

    // ── Task 2.6: hardware trigger ───────────────────────────────────────────
    [[maybe_unused]] const auto trig_result =
        adc.set_hardware_trigger(0u, alloy::hal::adc::TriggerEdge::Rising);

    // ── Task 2.7: sequence read (polling) ────────────────────────────────────
    std::array<std::uint16_t, 3> samples{};
    [[maybe_unused]] const auto rseq_result =
        adc.read_sequence(std::span<std::uint16_t>{samples});

    // ── Task 2.8: status ─────────────────────────────────────────────────────
    [[maybe_unused]] const auto eos         = adc.end_of_sequence();
    [[maybe_unused]] const auto ovr         = adc.overrun();
    [[maybe_unused]] const auto clr_result  = adc.clear_overrun();

    // ── Return type checks ───────────────────────────────────────────────────
    static_assert(
        std::is_same_v<decltype(res_result),
                       const alloy::core::Result<void, alloy::core::ErrorCode>>);
    static_assert(
        std::is_same_v<decltype(rseq_result),
                       const alloy::core::Result<void, alloy::core::ErrorCode>>);
    static_assert(std::is_same_v<decltype(eos), const bool>);
    static_assert(std::is_same_v<decltype(ovr), const bool>);
    static_assert(std::is_same_v<decltype(ch_en), const bool>);
#endif
    return 0;
}
