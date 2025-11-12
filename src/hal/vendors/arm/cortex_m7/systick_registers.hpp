#pragma once

#include <cstdint>

namespace alloy::hal::arm::cortex_m7 {

/**
 * @brief SysTick Timer registers (ARM Cortex-M standard)
 * 
 * Base address: 0xE000E010
 */
struct SysTick_Type {
    volatile uint32_t CTRL;   ///< 0x00: Control and Status Register
    volatile uint32_t LOAD;   ///< 0x04: Reload Value Register
    volatile uint32_t VAL;    ///< 0x08: Current Value Register
    volatile uint32_t CALIB;  ///< 0x0C: Calibration Value Register
};

} // namespace alloy::hal::arm::cortex_m7
