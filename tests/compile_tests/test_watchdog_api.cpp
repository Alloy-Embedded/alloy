// Compile test: pins the alloy::hal::watchdog::handle<P> public surface
// against every foundational backend. Methods that the descriptor doesn't
// publish for the bound peripheral return NotSupported at runtime — this
// test validates the signatures + capability gates compile.

#include <cstdint>
#include <span>
#include <type_traits>

#include "hal/watchdog.hpp"

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
#include "runtime/async_watchdog.hpp"
#endif

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE

namespace {

template <typename Handle>
void exercise_watchdog_backend(const Handle& wdg) {
    // Core API (unchanged).
    [[maybe_unused]] const auto enable_result = wdg.enable();
    [[maybe_unused]] const auto disable_result = wdg.disable();
    [[maybe_unused]] const auto refresh_result = wdg.refresh();

    // Phase 1: window mode + early warning + status flags + reset enable.
    [[maybe_unused]] const auto window_set = wdg.set_window(std::uint16_t{0x40});
    [[maybe_unused]] const auto window_on  = wdg.enable_window_mode(true);
    [[maybe_unused]] const auto window_off = wdg.enable_window_mode(false);
    [[maybe_unused]] const auto ewi_on     = wdg.enable_early_warning(std::uint16_t{50});
    [[maybe_unused]] const bool ewi_pending = wdg.early_warning_pending();
    [[maybe_unused]] const auto ewi_clear  = wdg.clear_early_warning();
    [[maybe_unused]] const bool to_flag    = wdg.timeout_occurred();
    [[maybe_unused]] const bool pvu        = wdg.prescaler_update_in_progress();
    [[maybe_unused]] const bool rvu        = wdg.reload_update_in_progress();
    [[maybe_unused]] const bool wvu        = wdg.window_update_in_progress();
    [[maybe_unused]] const bool err        = wdg.error();
    [[maybe_unused]] const auto reset_on   = wdg.set_reset_on_timeout(true);
    [[maybe_unused]] const auto reset_off  = wdg.set_reset_on_timeout(false);

    // Phase 2: kernel clock source (always NotSupported on current backends).
    [[maybe_unused]] const auto kclk = wdg.set_kernel_clock_source(
        alloy::hal::watchdog::KernelClockSource::Default);

    // Phase 2: typed interrupts + irq numbers.
    using K = alloy::hal::watchdog::InterruptKind;
    [[maybe_unused]] const auto irq_arm = wdg.enable_interrupt(K::EarlyWarning);
    [[maybe_unused]] const auto irq_off = wdg.disable_interrupt(K::EarlyWarning);
    [[maybe_unused]] const auto irqs    = Handle::irq_numbers();
    static_assert(std::is_same_v<decltype(irqs), const std::span<const std::uint32_t>>);

    // Phase 3: async wait_for(EarlyWarning).
    [[maybe_unused]] const auto wait_op =
        alloy::runtime::async::watchdog::wait_for<K::EarlyWarning>(wdg);
}

}  // namespace

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_F401RE) ||  \
    defined(ALLOY_BOARD_SAME70_XPLD)
[[maybe_unused]] void compile_iwdg_backend() {
    auto wdg = alloy::hal::watchdog::open<alloy::device::PeripheralId::IWDG>();
    exercise_watchdog_backend(wdg);
}
#endif

#if defined(ALLOY_BOARD_NUCLEO_F401RE)
[[maybe_unused]] void compile_wwdg_backend() {
    auto wdg = alloy::hal::watchdog::open<alloy::device::PeripheralId::WWDG>();
    exercise_watchdog_backend(wdg);
}
#endif

#if defined(ALLOY_BOARD_SAME70_XPLD)
[[maybe_unused]] void compile_same70_wdt_backend() {
    auto wdg = alloy::hal::watchdog::open<alloy::device::PeripheralId::WDT>();
    exercise_watchdog_backend(wdg);
}
#endif

#endif  // ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE
