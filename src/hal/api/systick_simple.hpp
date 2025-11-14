/**
 * @file systick_simple.hpp
 * @brief Level 1 Simple API for SysTick
 *
 * Provides timing and delay functions using SysTick timer.
 * Platform-agnostic API using template-based Platform Layer.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Simple SysTick API for timing and delays
 *
 * Platform-agnostic API that delegates to platform-specific SysTick implementation.
 *
 * Usage:
 * @code
 * // In board initialization
 * SysTickTimer::init_ms<SysTick<12000000>>(1);  // 1ms ticks at 12MHz
 *
 * // In application
 * u32 start = SysTickTimer::millis<SysTick<12000000>>();
 * SysTickTimer::delay_ms<SysTick<12000000>>(100);  // 100ms delay
 * u64 us = SysTickTimer::micros<SysTick<12000000>>();
 * @endcode
 *
 * @tparam SysTickImpl Platform-specific SysTick implementation
 */
class SysTickTimer {
public:
    /**
     * @brief Initialize SysTick with millisecond period
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param ms Millisecond period (default: 1ms)
     */
    template <typename SysTickImpl>
    static void init_ms(u32 ms = 1) {
        SysTickImpl::init_ms(ms);
    }

    /**
     * @brief Initialize SysTick with microsecond period
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param us Microsecond period
     */
    template <typename SysTickImpl>
    static void init_us(u32 us) {
        SysTickImpl::init_us(us);
    }

    /**
     * @brief Get current time in microseconds
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @return Microseconds since initialization
     */
    template <typename SysTickImpl>
    static u64 micros() {
        return SysTickImpl::micros();
    }

    /**
     * @brief Get current time in milliseconds
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @return Milliseconds since initialization
     */
    template <typename SysTickImpl>
    static u32 millis() {
        return SysTickImpl::millis();
    }

    /**
     * @brief Delay for specified milliseconds
     *
     * Blocking delay using SysTick counter.
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param ms Milliseconds to delay
     */
    template <typename SysTickImpl>
    static void delay_ms(u32 ms) {
        u64 start = SysTickImpl::micros();
        u64 delay_us = static_cast<u64>(ms) * 1000ULL;
        while ((SysTickImpl::micros() - start) < delay_us) {
            // Busy wait
        }
    }

    /**
     * @brief Delay for specified microseconds
     *
     * Blocking delay using SysTick counter.
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param us Microseconds to delay
     */
    template <typename SysTickImpl>
    static void delay_us(u32 us) {
        u64 start = SysTickImpl::micros();
        while ((SysTickImpl::micros() - start) < us) {
            // Busy wait
        }
    }

    /**
     * @brief Calculate elapsed time in microseconds
     *
     * Handles counter overflow correctly.
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param start_us Start time from micros()
     * @return Elapsed microseconds
     */
    template <typename SysTickImpl>
    static u64 elapsed_us(u64 start_us) {
        return SysTickImpl::micros() - start_us;
    }

    /**
     * @brief Calculate elapsed time in milliseconds
     *
     * Handles counter overflow correctly.
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param start_ms Start time from millis()
     * @return Elapsed milliseconds
     */
    template <typename SysTickImpl>
    static u32 elapsed_ms(u32 start_ms) {
        return SysTickImpl::millis() - start_ms;
    }

    /**
     * @brief Check if timeout has occurred
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param start_us Start time from micros()
     * @param timeout_us Timeout duration in microseconds
     * @return true if timeout expired
     */
    template <typename SysTickImpl>
    static bool is_timeout_us(u64 start_us, u64 timeout_us) {
        return elapsed_us<SysTickImpl>(start_us) >= timeout_us;
    }

    /**
     * @brief Check if timeout has occurred (millisecond version)
     *
     * @tparam SysTickImpl Platform SysTick implementation
     * @param start_ms Start time from millis()
     * @param timeout_ms Timeout duration in milliseconds
     * @return true if timeout expired
     */
    template <typename SysTickImpl>
    static bool is_timeout_ms(u32 start_ms, u32 timeout_ms) {
        return elapsed_ms<SysTickImpl>(start_ms) >= timeout_ms;
    }
};

}  // namespace alloy::hal
