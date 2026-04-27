#include "hal/pwm.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::pwm::PeripheralId;

// ---- extended API smoke — tasks 5.1 / 5.2 / 5.3 --------------------------

template <typename PwmHandle>
void exercise_pwm_extended() {
    auto pwm = PwmHandle{
        alloy::hal::pwm::Config{
            .period = 999u,
            .apply_period = true,
            .duty_cycle = 499u,
            .apply_duty_cycle = true,
            .start_immediately = false,
        },
    };

    // ---- irq_numbers (task 4.3) ----
    [[maybe_unused]] const auto irq_span = PwmHandle::irq_numbers();
    static_assert(
        std::is_same_v<decltype(irq_span), const std::span<const std::uint32_t>>);

    // ---- paired_channel (task 4.4) ----
    [[maybe_unused]] constexpr auto paired = PwmHandle::paired_channel();
    static_assert(std::is_same_v<decltype(paired), const std::uint8_t>);

    // ---- Phase 1: mode / polarity / complementary ----
    [[maybe_unused]] const auto ca_r   = pwm.set_center_aligned(alloy::hal::pwm::CenterAligned::Mode1);
    [[maybe_unused]] const auto pol_r  = pwm.set_channel_polarity(alloy::hal::pwm::Polarity::Active);
    [[maybe_unused]] const auto comp_r = pwm.enable_complementary_output(true);
    [[maybe_unused]] const auto cpol_r = pwm.set_complementary_polarity(alloy::hal::pwm::Polarity::Inverted);

    // ---- Phase 2: dead-time / fault / master-output / sync ----
    [[maybe_unused]] const auto dt_r   = pwm.set_dead_time(10u, 10u);
    [[maybe_unused]] const auto fi_r   = pwm.enable_fault_input(true);
    [[maybe_unused]] const auto fp_r   = pwm.set_fault_polarity(alloy::hal::pwm::Polarity::Active);
    [[maybe_unused]] const bool  fa    = pwm.fault_active();
    [[maybe_unused]] const auto cff_r  = pwm.clear_fault_flag();
    [[maybe_unused]] const auto moe_r  = pwm.enable_master_output(true);
    [[maybe_unused]] const auto sync_r = pwm.set_update_synchronized(true);

    // ---- Phase 3: carrier / force-init ----
    [[maybe_unused]] const auto cm_r   = pwm.set_carrier_modulation(false);
    [[maybe_unused]] const auto fi2_r  = pwm.force_initialize();

    // ---- Phase 4: channel events / interrupts ----
    [[maybe_unused]] const bool  ce    = pwm.channel_event();
    [[maybe_unused]] const auto cce_r  = pwm.clear_channel_event();
    [[maybe_unused]] const auto en_cc  = pwm.enable_interrupt(alloy::hal::pwm::InterruptKind::ChannelCompare);
    [[maybe_unused]] const auto dis_cc = pwm.disable_interrupt(alloy::hal::pwm::InterruptKind::ChannelCompare);
    [[maybe_unused]] const auto en_u   = pwm.enable_interrupt(alloy::hal::pwm::InterruptKind::Update);
}

// ---- board sections -------------------------------------------------------

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
// task 5.1: G071 TIM1 ch0 — center-aligned, complementary, dead-time.
using Pwm = alloy::hal::pwm::handle<PeripheralId::TIM1, 0u>;
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
// task 5.2: F401 TIM1 ch0 — advanced timer, complementary + dead-time + master-output.
using Pwm = alloy::hal::pwm::handle<PeripheralId::TIM1, 0u>;
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
// task 5.3: SAME70 PWM0 ch0 — separate DTHI / DTLI fields.
using Pwm = alloy::hal::pwm::handle<PeripheralId::PWM0, 0u>;
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
using Pwm = alloy::hal::pwm::handle<PeripheralId::PWM, 0u>;
#endif

#if !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
static_assert(Pwm::valid);
#endif
#endif

int main() {
#if ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE && !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    // ---- baseline API (original) ----
    auto pwm = alloy::hal::pwm::open<Pwm::peripheral_id, Pwm::channel_index>(
        alloy::hal::pwm::Config{
            .period = 1000u, .apply_period = true,
            .duty_cycle = 500u, .apply_duty_cycle = true});
    [[maybe_unused]] const auto configure_result = pwm.configure();
    [[maybe_unused]] const auto start_result     = pwm.start();
    [[maybe_unused]] const auto stop_result      = pwm.stop();
    [[maybe_unused]] const auto period_result    = pwm.set_period(1000u);
    [[maybe_unused]] const auto duty_result      = pwm.set_duty_cycle(500u);
    [[maybe_unused]] const auto freq_result      = pwm.set_frequency(1000u);

    // ---- extended API ----
#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE) || \
    defined(ALLOY_BOARD_NUCLEO_F401RE) || defined(ALLOY_BOARD_SAME70_XPLD)   || \
    defined(ALLOY_BOARD_SAME70_XPLAINED)
    exercise_pwm_extended<Pwm>();
#endif
#endif
    return 0;
}
