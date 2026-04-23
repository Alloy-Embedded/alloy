#include BOARD_HEADER

#include "event.hpp"
#include "low_power.hpp"
#include "time.hpp"

using BoardTime = alloy::time::source<board::BoardSysTick>;
using LocalCompletion = alloy::event::completion<alloy::event::tag_constant<1u>>;

#if ALLOY_DEVICE_LOW_POWER_AVAILABLE
constexpr auto kFirstMode = alloy::device::low_power::modes.front().mode_id;

static_assert(alloy::low_power::supported(kFirstMode));
static_assert(alloy::low_power::time_valid_in<BoardTime>(kFirstMode));
static_assert(alloy::low_power::completion_valid_in<LocalCompletion>(kFirstMode));
#endif

int main() {
#if ALLOY_DEVICE_LOW_POWER_AVAILABLE
    [[maybe_unused]] const auto first_uses_sleepdeep =
        alloy::low_power::uses_sleepdeep(kFirstMode);
    [[maybe_unused]] const auto first_time_valid =
        alloy::low_power::time_valid_in<BoardTime>(kFirstMode);
    [[maybe_unused]] const auto first_completion_valid =
        alloy::low_power::completion_valid_in<LocalCompletion>(kFirstMode);

    if constexpr (alloy::device::low_power::modes.size() > 1u) {
        constexpr auto alternate_mode = alloy::device::low_power::modes.back().mode_id;
        static_assert(alloy::low_power::supported(alternate_mode));

        [[maybe_unused]] const auto alternate_uses_sleepdeep =
            alloy::low_power::uses_sleepdeep(alternate_mode);
        [[maybe_unused]] const auto alternate_time_valid =
            alloy::low_power::time_valid_in<BoardTime>(alternate_mode);
        [[maybe_unused]] const auto alternate_completion_valid =
            alloy::low_power::completion_valid_in<LocalCompletion>(alternate_mode);
    }

#if defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
    static_assert(!alloy::device::low_power::wakeup_sources.empty());
    constexpr auto wakeup_source = alloy::device::low_power::wakeup_sources.front().wakeup_source_id;
    constexpr auto wakeup_mode = alloy::device::low_power::modes.back().mode_id;
    using WakeupCompletion = alloy::low_power::wakeup_token<wakeup_source>;
    static_assert(alloy::low_power::wakeup_source_available(wakeup_source));
    static_assert(alloy::low_power::wakeup_source_valid_in(wakeup_mode, wakeup_source));
    static_assert(alloy::low_power::completion_valid_in<WakeupCompletion>(wakeup_mode));

    [[maybe_unused]] const auto wakeup_valid =
        alloy::low_power::wakeup_source_valid_in(wakeup_mode, wakeup_source);
    WakeupCompletion::reset();
    WakeupCompletion::signal();
    [[maybe_unused]] const auto wakeup_completion_valid =
        alloy::low_power::completion_valid_in<WakeupCompletion>(wakeup_mode);
#endif
#endif
    return 0;
}
