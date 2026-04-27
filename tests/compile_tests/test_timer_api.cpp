#include "hal/timer.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::timer::PeripheralId;

// ---- extended API smoke — task 5.1 / 5.2 / 5.3 ---------------------------

template <typename TimerHandle>
void exercise_timer_extended(std::uint32_t clock_hz) {
    auto timer = TimerHandle{
        alloy::hal::timer::Config{
            .period = 999u,
            .peripheral_clock_hz = clock_hz,
            .apply_period = true,
            .start_immediately = false,
        },
    };

    // ---- irq_numbers (task 4.4) ----
    [[maybe_unused]] const auto irq_span = TimerHandle::irq_numbers();
    static_assert(
        std::is_same_v<decltype(irq_span), const std::span<const std::uint32_t>>);

    // ---- Phase 1: prescaler / frequency / counter mode ----
    [[maybe_unused]] const auto psc_r  = timer.set_prescaler(7u);
    [[maybe_unused]] const auto freq_r = timer.set_frequency(1000u);
    [[maybe_unused]] const auto hz     = timer.frequency();
    static_assert(noexcept(timer.frequency()));
    [[maybe_unused]] const auto dir_r  = timer.set_direction(alloy::hal::timer::CountDirection::Down);
    [[maybe_unused]] const auto ca_r   = timer.set_center_aligned(alloy::hal::timer::CenterAligned::Mode1);
    [[maybe_unused]] const auto opm_r  = timer.set_one_pulse(true);
    [[maybe_unused]] const auto arpe_r = timer.set_auto_reload_preload(true);
    [[maybe_unused]] const auto pp_r   = timer.set_period_preload(true);

    // ---- Phase 2: capture / compare channels ----
    [[maybe_unused]] const auto mode_r = timer.set_channel_mode(
        0u, alloy::hal::timer::CaptureCompareMode::Pwm1);
    [[maybe_unused]] const auto cmp_r  = timer.set_compare_value(0u, 499u);
    [[maybe_unused]] const auto cap_r  = timer.read_capture_value(0u);
    [[maybe_unused]] const auto oe_r   = timer.enable_channel_output(0u, true);
    [[maybe_unused]] const auto pol_r  = timer.set_channel_output_polarity(
        0u, alloy::hal::timer::Polarity::Active);

    // ---- Phase 2: complementary outputs ----
    [[maybe_unused]] const auto comp_r = timer.enable_complementary_output(0u, true);
    [[maybe_unused]] const auto cpol_r = timer.set_complementary_polarity(
        0u, alloy::hal::timer::Polarity::Inverted);

    // ---- Phase 3: dead-time / break / encoder ----
    [[maybe_unused]] const auto dt_r   = timer.set_dead_time(10u);
    [[maybe_unused]] const auto brk_r  = timer.enable_break_input(true);
    [[maybe_unused]] const auto bpol_r = timer.set_break_polarity(alloy::hal::timer::Polarity::Active);
    [[maybe_unused]] const bool  bact  = timer.break_active();
    [[maybe_unused]] const auto cbf_r  = timer.clear_break_flag();
    [[maybe_unused]] const auto enc_r  = timer.set_encoder_mode(
        alloy::hal::timer::EncoderMode::BothChannels);
    [[maybe_unused]] const auto epol_r = timer.set_encoder_polarity(
        alloy::hal::timer::Polarity::Active);

    // ---- Phase 4: status / events ----
    [[maybe_unused]] const bool  ue    = timer.update_event();
    [[maybe_unused]] const auto clr_r  = timer.clear_update_event();
    [[maybe_unused]] const bool  che   = timer.channel_event(0u);
    [[maybe_unused]] const auto cche_r = timer.clear_channel_event(0u);
    [[maybe_unused]] const auto en_u   = timer.enable_interrupt(
        alloy::hal::timer::InterruptKind::Update);
    [[maybe_unused]] const auto dis_u  = timer.disable_interrupt(
        alloy::hal::timer::InterruptKind::Update);
    [[maybe_unused]] const auto en_cc  = timer.enable_interrupt(
        alloy::hal::timer::InterruptKind::ChannelCompare, 0u);
    [[maybe_unused]] const auto dis_cc = timer.disable_interrupt(
        alloy::hal::timer::InterruptKind::ChannelCompare, 0u);
}

// ---- board sections -------------------------------------------------------

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
// task 5.1: G071 TIM1 — has encoder, complementary, center-aligned.
using Timer = alloy::hal::timer::handle<PeripheralId::TIM1>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
// task 5.2: F401 TIM1 — advanced timer, complementary outputs + dead-time flag.
using Timer = alloy::hal::timer::handle<PeripheralId::TIM1>;
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
// task 5.3: SAME70 TC0 — SAM TC encoder via kEncoderEnableField path.
using Timer = alloy::hal::timer::handle<PeripheralId::TC0>;
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
using Timer = alloy::hal::timer::handle<PeripheralId::TIMER>;
#endif

#if !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
static_assert(Timer::valid);
#endif
#endif

int main() {
#if ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE && !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    // ---- baseline API (original) ----
    auto timer = alloy::hal::timer::open<Timer::peripheral_id>(
        alloy::hal::timer::Config{.period = 1000u, .apply_period = true});
    [[maybe_unused]] const auto configure_result = timer.configure();
    [[maybe_unused]] const auto start_result     = timer.start();
    [[maybe_unused]] const auto stop_result      = timer.stop();
    [[maybe_unused]] const auto period_result    = timer.set_period(1000u);
    [[maybe_unused]] const auto count_result     = timer.get_count();
    [[maybe_unused]] const bool running          = timer.is_running();

    // ---- extended API (tasks 5.1 / 5.2 / 5.3) ----
#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    exercise_timer_extended<Timer>(64'000'000u);
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    exercise_timer_extended<Timer>(84'000'000u);
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
    exercise_timer_extended<Timer>(150'000'000u);
#endif
#endif
    return 0;
}
