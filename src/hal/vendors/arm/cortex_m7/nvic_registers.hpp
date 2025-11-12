#pragma once

#include <cstdint>

namespace alloy::hal::arm::cortex_m7 {

/**
 * @brief NVIC (Nested Vectored Interrupt Controller) registers
 * 
 * Base address: 0xE000E100
 */
struct NVIC_Type {
    volatile uint32_t ISER[8];    ///< 0x000-0x01C: Interrupt Set Enable Registers
    uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];    ///< 0x080-0x09C: Interrupt Clear Enable Registers
    uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];    ///< 0x100-0x11C: Interrupt Set Pending Registers
    uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];    ///< 0x180-0x19C: Interrupt Clear Pending Registers
    uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];    ///< 0x200-0x21C: Interrupt Active Bit Registers
    uint32_t RESERVED4[56];
    volatile uint8_t  IP[240];    ///< 0x300-0x3EF: Interrupt Priority Registers
};

} // namespace alloy::hal::arm::cortex_m7
