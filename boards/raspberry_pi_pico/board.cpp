#include "board.hpp"

#include <cstdint>

#include "hal/systick.hpp"
#include "arch/cortex_m/init_hooks.hpp"

#include "device/clock_config.hpp"
#include "device/runtime.hpp"
#include "device/system_clock.hpp"

using namespace alloy::hal;

namespace board {

namespace {

static bool board_initialized = false;

}  // namespace

namespace led {

// RP2040 GPIO HAL not yet implemented — LED functions are stubs.
// TODO: implement when rp2040 GPIO schema is added to the HAL.
void init() {}
void on() {}
void off() {}
void toggle() {}

}  // namespace led

void init() {
    if (board_initialized) {
        return;
    }

#if ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
    alloy::device::clock_config::apply_profile<ClockConfig::system_clock_profile>();
#else
    alloy::device::system_clock::apply_default();
#endif

    SysTickTimer::init_ms<BoardSysTick>(1);

    __asm volatile("cpsie i" ::: "memory");
    alloy::hal::arm::late_init();

    board_initialized = true;
}

}  // namespace board

extern "C" void SysTick_Handler() {
    board::BoardSysTick::increment_tick();
}
