#include "alloy/arch/cortex_m/systick.hpp"

#include <chrono>

#include "alloy/arch/cortex_m/nvic.hpp"
#include "alloy/arch/irq.hpp"
#include "alloy/core/mmio.hpp"
#include "alloy/time.hpp"

namespace alloy::arch::cortex_m {
namespace {

// ARMv6-M/v7-M/v8-M architectural SysTick block (B3.3 in the ARM ARM).
struct systick_regs {
    rw32 CSR;    // control and status
    rw32 RVR;    // reload value
    rw32 CVR;    // current value
    ro32 CALIB;  // calibration
};

systick_regs& systick() {
    return *reinterpret_cast<systick_regs*>(0xE000'E010u);
}

constexpr std::uint32_t kCsrEnable = 1u << 0;
constexpr std::uint32_t kCsrTickInt = 1u << 1;
constexpr std::uint32_t kCsrClkSourceCore = 1u << 2;

volatile std::uint32_t g_ticks_ms = 0;

}  // namespace

void systick_init(std::uint32_t core_hz) {
    auto& st = systick();
    st.CSR = 0;
    st.RVR = core_hz / 1'000u - 1u;  // 1 kHz
    st.CVR = 0;
    g_ticks_ms = 0;
    st.CSR = kCsrEnable | kCsrTickInt | kCsrClkSourceCore;
}

}  // namespace alloy::arch::cortex_m

extern "C" void SysTick_Handler() {
    alloy::arch::cortex_m::g_ticks_ms = alloy::arch::cortex_m::g_ticks_ms + 1u;
}

namespace alloy::arch {

irq_state irq_save() {
    std::uint32_t primask;
    __asm volatile("mrs %0, PRIMASK\n\tcpsid i" : "=r"(primask)::"memory");
    return primask;
}

void irq_restore(irq_state state) {
    if ((state & 1u) == 0u) {
        __asm volatile("cpsie i" ::: "memory");
    }
}

irq_state irq_save_below(std::uint8_t level) {
#if defined(__ARM_ARCH_6M__)
    // ARMv6-M has no BASEPRI — a priority-scoped mask is impossible, so coarsen
    // to a full mask. Correct, just less selective (blocks the urgent lines too).
    (void)level;
    return irq_save();
#else
    const std::uint32_t old = cortex_m::nvic::basepri_get();
    const std::uint32_t want = cortex_m::nvic::prio_value(level);
    // Only ever tighten the mask: a nested section must never weaken an outer
    // one. BASEPRI==0 means "not masking", so any non-zero want tightens it.
    if (old == 0u || want < old) {
        cortex_m::nvic::basepri_set(want);
    }
    return old;
#endif
}

void irq_restore_below(irq_state state) {
#if defined(__ARM_ARCH_6M__)
    irq_restore(state);
#else
    cortex_m::nvic::basepri_set(state);
#endif
}

void idle() { __asm volatile("wfi" ::: "memory"); }

}  // namespace alloy::arch

namespace alloy {

std::uint32_t uptime_ms() { return arch::cortex_m::g_ticks_ms; }

std::uint32_t uptime_us() {
    // Interpolate the down-counting SysTick CVR inside the current 1 ms tick.
    // The tick count and CVR cannot be read atomically together, so re-read the
    // tick count around the CVR sample and retry if it moved — the classic
    // seqlock shape. One retry at most in thread context (the ISR is short).
    //
    // Residual window, accepted: if the counter wraps right before the sample
    // and SysTick_Handler has not run yet (only when it is masked or preempted),
    // the reading is up to 1 ms stale — the same staleness uptime_ms() has.
    auto& st = arch::cortex_m::systick();
    for (;;) {
        const std::uint32_t ms0 = arch::cortex_m::g_ticks_ms;
        const std::uint32_t cvr = st.CVR;
        if (arch::cortex_m::g_ticks_ms != ms0) {
            continue;  // tick landed between the two reads — resample
        }
        // CVR counts RVR -> 0, so cycles into this tick = period - CVR. The
        // 32-bit product is safe: RVR is 24-bit, so period*1000 < 2^32 for any
        // real core clock. ms0*1000 wrapping IS the documented 2^32 µs wrap.
        const std::uint32_t period = st.RVR + 1u;
        const std::uint32_t elapsed = period - cvr;
        return ms0 * 1'000u + (elapsed * 1'000u) / period;
    }
}

void sleep_for(std::chrono::microseconds d) {
    // Round up to whole ticks; guarantee at least the requested duration by
    // waiting one extra edge (the first counted tick may be partial).
    const auto us = d.count();
    const std::uint32_t ticks = static_cast<std::uint32_t>((us + 999) / 1'000) + 1u;
    const std::uint32_t start = arch::cortex_m::g_ticks_ms;
    while (arch::cortex_m::g_ticks_ms - start < ticks) {
    }
}

}  // namespace alloy
