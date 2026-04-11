#pragma once

#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

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

}  // namespace alloy::hal
