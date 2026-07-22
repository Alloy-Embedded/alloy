#include "alloy/arch/cortex_m/systick.hpp"

#include <chrono>

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

namespace alloy {

std::uint32_t uptime_ms() { return arch::cortex_m::g_ticks_ms; }

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
