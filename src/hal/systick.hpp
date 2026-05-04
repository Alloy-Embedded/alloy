#pragma once

#include <concepts>

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

struct SysTickConfig {
    u32 tick_frequency_hz;

    constexpr explicit SysTickConfig(u32 freq_hz = 1000000) : tick_frequency_hz(freq_hz) {}
};

template <typename T>
concept SystemTick = requires {
    { T::init() } -> std::convertible_to<Result<void, ErrorCode>>;
    { T::micros() } -> std::convertible_to<u32>;
    { T::reset() } -> std::convertible_to<Result<void, ErrorCode>>;
    { T::is_initialized() } -> std::convertible_to<bool>;
};

class SysTickTimer {
   public:
    template <typename SysTickImpl>
    static void init_ms(u32 ms = 1) {
        SysTickImpl::init_ms(ms);
    }

    template <typename SysTickImpl>
    static void init_us(u32 us) {
        SysTickImpl::init_us(us);
    }

    template <typename SysTickImpl>
    [[nodiscard]] static auto micros() -> u64 {
        return SysTickImpl::micros();
    }

    template <typename SysTickImpl>
    [[nodiscard]] static auto millis() -> u32 {
        return SysTickImpl::millis();
    }

    template <typename SysTickImpl>
    static void delay_ms(u32 ms) {
        const u64 start = SysTickImpl::micros();
        const u64 delay_us = static_cast<u64>(ms) * 1000ULL;
        while ((SysTickImpl::micros() - start) < delay_us) {
        }
    }

    template <typename SysTickImpl>
    static void delay_us(u32 us) {
        const u64 start = SysTickImpl::micros();
        while ((SysTickImpl::micros() - start) < us) {
        }
    }

    template <typename SysTickImpl>
    [[nodiscard]] static auto elapsed_us(u64 start_us) -> u64 {
        return SysTickImpl::micros() - start_us;
    }

    template <typename SysTickImpl>
    [[nodiscard]] static auto elapsed_ms(u32 start_ms) -> u32 {
        return SysTickImpl::millis() - start_ms;
    }

    template <typename SysTickImpl>
    [[nodiscard]] static auto is_timeout_us(u64 start_us, u64 timeout_us) -> bool {
        return elapsed_us<SysTickImpl>(start_us) >= timeout_us;
    }

    template <typename SysTickImpl>
    [[nodiscard]] static auto is_timeout_ms(u32 start_ms, u32 timeout_ms) -> bool {
        return elapsed_ms<SysTickImpl>(start_ms) >= timeout_ms;
    }
};

namespace cortex_m {

struct SysTickRegisters {
    volatile std::uint32_t ctrl;
    volatile std::uint32_t load;
    volatile std::uint32_t val;
    volatile std::uint32_t calib;
};

inline constexpr std::uintptr_t kSysTickBase = 0xE000E010;

template <std::uint32_t ClockHz>
class SysTick {
   public:
    static constexpr std::uint32_t clock_hz = ClockHz;

    static void init_ms(std::uint32_t period_ms) {
        tick_period_us = period_ms * 1000u;
        configure_reload(((ClockHz / 1000u) * period_ms) - 1u);
    }

    static void init_us(std::uint32_t period_us) {
        tick_period_us = period_us;
        configure_reload(((ClockHz / 1000000u) * period_us) - 1u);
    }

    [[nodiscard]] static auto get_ticks() -> std::uint32_t {
        return tick_count;
    }

    [[nodiscard]] static auto millis() -> std::uint32_t {
        return tick_count * (tick_period_us / 1000u);
    }

    [[nodiscard]] static auto micros() -> std::uint64_t {
        return static_cast<std::uint64_t>(tick_count) * tick_period_us;
    }

    static void increment_tick() {
        tick_count = tick_count + 1u;
    }

   private:
    static void configure_reload(std::uint32_t reload) {
        auto* systick = reinterpret_cast<volatile SysTickRegisters*>(kSysTickBase);
        systick->load = reload;
        systick->val = 0u;
        systick->ctrl = 0x7u;
    }

    static inline volatile std::uint32_t tick_count = 0u;
    static inline volatile std::uint32_t tick_period_us = 1000u;
};

}  // namespace cortex_m

}  // namespace alloy::hal

namespace alloy::systick {

namespace detail {
core::u32 get_micros();
}

inline auto micros() -> core::u32 {
    return detail::get_micros();
}

inline auto micros_since(core::u32 start_time) -> core::u32 {
    return micros() - start_time;
}

inline auto is_timeout(core::u32 start_time, core::u32 timeout_us) -> bool {
    return micros_since(start_time) >= timeout_us;
}

inline void delay_us(core::u32 delay_us) {
    const core::u32 start = micros();
    while (!is_timeout(start, delay_us)) {
    }
}

}  // namespace alloy::systick

// alloy.device.v2.1 Cortex-M SysTick driver — no descriptor-runtime required.
// timer<ClockHz>: polling (delay_ms_poll/delay_us_poll) and interrupt modes.
#include "hal/systick/lite.hpp"
