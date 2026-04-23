#pragma once

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/low_power.hpp"
#include "event.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::runtime::low_power {

using ModeId = alloy::device::low_power::ModeId;
using ModeDescriptor = alloy::device::low_power::ModeDescriptor;
using WakeupSourceId = alloy::device::low_power::WakeupSourceId;
using WakeupSourceDescriptor = alloy::device::low_power::WakeupSourceDescriptor;
using PinId = alloy::device::low_power::PinId;

inline constexpr bool available = alloy::device::SelectedLowPower::available;

struct coordination_hooks {
    void (*before_entry)(ModeId) = nullptr;
    void (*after_wakeup)(ModeId) = nullptr;
};

namespace detail {

constexpr auto kScbScrAddress = std::uintptr_t{0xE000ED10u};
constexpr auto kScbScrSleepdeepMask = std::uint32_t{1u << 2u};

[[nodiscard]] inline auto installed_hooks() -> coordination_hooks& {
    static auto hooks = coordination_hooks{};
    return hooks;
}

#if defined(ALLOY_ENABLE_HOST_MMIO_RUNTIME_HOOKS)
namespace test_support {

using wait_hook = void (*)();

[[nodiscard]] inline auto installed_wait_hook() -> wait_hook& {
    static auto hook = static_cast<wait_hook>(nullptr);
    return hook;
}

[[nodiscard]] inline auto set_wait_hook(wait_hook hook) -> wait_hook {
    auto previous = installed_wait_hook();
    installed_wait_hook() = hook;
    return previous;
}

}  // namespace test_support
#endif

inline void wait_for_interrupt() {
#if defined(ALLOY_ENABLE_HOST_MMIO_RUNTIME_HOOKS)
    if (const auto hook = test_support::installed_wait_hook(); hook != nullptr) {
        hook();
    }
    return;
#else
    __asm__ volatile("dsb" ::: "memory");
    __asm__ volatile("wfi");
    __asm__ volatile("isb" ::: "memory");
#endif
}

[[nodiscard]] constexpr auto find_mode(ModeId mode) -> const ModeDescriptor* {
    for (const auto& descriptor : alloy::device::low_power::modes) {
        if (descriptor.mode_id == mode) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] constexpr auto find_wakeup_source(WakeupSourceId wakeup_source)
    -> const WakeupSourceDescriptor* {
    for (const auto& descriptor : alloy::device::low_power::wakeup_sources) {
        if (descriptor.wakeup_source_id == wakeup_source) {
            return &descriptor;
        }
    }
    return nullptr;
}

}  // namespace detail

template <typename TimeSource>
struct time_source_wakeup_traits {
    static constexpr bool kValidInSleepdeep = false;
};

template <typename CompletionToken>
struct completion_wakeup_traits {
    static constexpr bool kValidInSleepdeep = false;
};

template <WakeupSourceId Id>
using wakeup_token = alloy::event::completion<alloy::event::tag_constant<Id>>;

template <PinId Pin>
using wakeup_pin_token =
    wakeup_token<alloy::device::low_power::WakeupPinTraits<Pin>::kWakeupSourceId>;

template <WakeupSourceId Id>
struct completion_wakeup_traits<wakeup_token<Id>> {
    static constexpr bool kValidInSleepdeep = alloy::device::low_power::WakeupSourceTraits<Id>::kPresent;
};

[[nodiscard]] constexpr auto supported(ModeId mode) -> bool {
    return detail::find_mode(mode) != nullptr;
}

[[nodiscard]] constexpr auto uses_sleepdeep(ModeId mode) -> bool {
    const auto* descriptor = detail::find_mode(mode);
    return descriptor != nullptr && descriptor->uses_sleepdeep;
}

[[nodiscard]] constexpr auto wakeup_source_available(WakeupSourceId wakeup_source) -> bool {
    return detail::find_wakeup_source(wakeup_source) != nullptr;
}

[[nodiscard]] constexpr auto wakeup_source_valid_in(ModeId mode, WakeupSourceId wakeup_source)
    -> bool {
    if (!supported(mode)) {
        return false;
    }
    return wakeup_source_available(wakeup_source);
}

template <typename TimeSource>
[[nodiscard]] constexpr auto time_valid_in(ModeId mode) -> bool {
    if (!supported(mode)) {
        return false;
    }
    return !uses_sleepdeep(mode) || time_source_wakeup_traits<TimeSource>::kValidInSleepdeep;
}

template <typename CompletionToken>
[[nodiscard]] constexpr auto completion_valid_in(ModeId mode) -> bool {
    if (!supported(mode)) {
        return false;
    }
    return !uses_sleepdeep(mode) || completion_wakeup_traits<CompletionToken>::kValidInSleepdeep;
}

[[nodiscard]] inline auto hooks() -> coordination_hooks { return detail::installed_hooks(); }

[[nodiscard]] inline auto set_hooks(coordination_hooks next_hooks) -> coordination_hooks {
    auto previous = detail::installed_hooks();
    detail::installed_hooks() = next_hooks;
    return previous;
}

[[nodiscard]] inline auto enter(ModeId mode) -> core::Result<void, core::ErrorCode> {
    const auto* descriptor = detail::find_mode(mode);
    if (descriptor == nullptr) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto previous_scr = alloy::hal::detail::runtime::read_mmio32(detail::kScbScrAddress);
    const auto next_scr = descriptor->uses_sleepdeep
                              ? static_cast<std::uint32_t>(previous_scr | detail::kScbScrSleepdeepMask)
                              : static_cast<std::uint32_t>(previous_scr & ~detail::kScbScrSleepdeepMask);

    if (const auto before_entry = detail::installed_hooks().before_entry; before_entry != nullptr) {
        before_entry(mode);
    }

    alloy::hal::detail::runtime::write_mmio32(detail::kScbScrAddress, next_scr);
    detail::wait_for_interrupt();
    alloy::hal::detail::runtime::write_mmio32(detail::kScbScrAddress, previous_scr);

    if (const auto after_wakeup = detail::installed_hooks().after_wakeup; after_wakeup != nullptr) {
        after_wakeup(mode);
    }

    return core::Ok();
}

}  // namespace alloy::runtime::low_power
