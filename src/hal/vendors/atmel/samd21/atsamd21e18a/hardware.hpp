#pragma once

#include <stdint.h>

namespace alloy::hal::atmel::samd21::atsamd21e18a::hardware {

// ============================================================================
// Hardware Register Definitions for ATSAMD21E18A
// Based on SAMD21 PORT Controller Architecture
// ============================================================================

// Memory map
constexpr uint32_t FLASH_BASE = 0x00000000;
constexpr uint32_t FLASH_SIZE = 256U * 1024U;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = 32U * 1024U;

// PORT Controller base addresses
constexpr uint32_t PORT_BASE = 0x41004400;

// ============================================================================
// PORT Register Structure (SAMD21 Architecture)
// ============================================================================

struct PORT_Group {
    volatile uint32_t DIR;          // 0x00: Data Direction
    volatile uint32_t DIRCLR;       // 0x04: Data Direction Clear
    volatile uint32_t DIRSET;       // 0x08: Data Direction Set
    volatile uint32_t DIRTGL;       // 0x0C: Data Direction Toggle
    volatile uint32_t OUT;          // 0x10: Data Output Value
    volatile uint32_t OUTCLR;       // 0x14: Data Output Value Clear
    volatile uint32_t OUTSET;       // 0x18: Data Output Value Set
    volatile uint32_t OUTTGL;       // 0x1C: Data Output Value Toggle
    volatile uint32_t IN;           // 0x20: Data Input Value
    volatile uint32_t CTRL;         // 0x24: Control
    volatile uint32_t WRCONFIG;     // 0x28: Write Configuration
    uint32_t RESERVED1;             // 0x2C: Reserved
    volatile uint8_t  PMUX[16];     // 0x30: Peripheral Multiplexing (0-15)
    volatile uint8_t  PINCFG[32];   // 0x40: Pin Configuration (0-31)
    uint32_t RESERVED2[8];          // 0x60: Reserved
};

struct PORT_Registers {
    PORT_Group GROUP[2];            // Group 0 = PORT A, Group 1 = PORT B
};

// PORT instance
static_assert(sizeof(PORT_Group) == 0x80, "PORT_Group size mismatch");

inline PORT_Registers* PORT = reinterpret_cast<PORT_Registers*>(PORT_BASE);
inline PORT_Group* PORTA = &PORT->GROUP[0];
inline PORT_Group* PORTB = &PORT->GROUP[1];

// Pin configuration bits
constexpr uint8_t PORT_PINCFG_PMUXEN  = (1 << 0);  // Peripheral Multiplexer Enable
constexpr uint8_t PORT_PINCFG_INEN    = (1 << 1);  // Input Enable
constexpr uint8_t PORT_PINCFG_PULLEN  = (1 << 2);  // Pull Enable
constexpr uint8_t PORT_PINCFG_DRVSTR  = (1 << 6);  // Output Driver Strength

}  // namespace alloy::hal::atmel::samd21::atsamd21e18a::hardware
