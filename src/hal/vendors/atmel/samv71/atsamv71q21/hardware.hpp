#pragma once

#include <cstdint>

namespace alloy::hal::atmel::samv71::atsamv71q21::hardware {

// ============================================================================
// Hardware Register Definitions for ATSAMV71Q21
// Based on SAME70 PIO (Parallel I/O) Controller Architecture
// ============================================================================

// Memory map
constexpr uint32_t FLASH_BASE = 0x00400000;
constexpr uint32_t FLASH_SIZE = 2048U * 1024U;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = 384U * 1024U;

// PIO Controller base addresses
constexpr uint32_t PIOA_BASE = 0x400E0E00;
constexpr uint32_t PIOB_BASE = 0x400E1000;
constexpr uint32_t PIOC_BASE = 0x400E1200;
constexpr uint32_t PIOD_BASE = 0x400E1400;
constexpr uint32_t PIOE_BASE = 0x400E1600;

// ============================================================================
// PIO Register Structure (SAME70 Architecture)
// ============================================================================

struct PIO_Registers {
    volatile uint32_t PER;         // 0x00: PIO Enable Register
    volatile uint32_t PDR;         // 0x04: PIO Disable Register
    volatile uint32_t PSR;         // 0x08: PIO Status Register
    uint32_t RESERVED1;            // 0x0C: Reserved
    volatile uint32_t OER;         // 0x10: Output Enable Register
    volatile uint32_t ODR;         // 0x14: Output Disable Register
    volatile uint32_t OSR;         // 0x18: Output Status Register
    uint32_t RESERVED2;            // 0x1C: Reserved
    volatile uint32_t IFER;        // 0x20: Glitch Input Filter Enable
    volatile uint32_t IFDR;        // 0x24: Glitch Input Filter Disable
    volatile uint32_t IFSR;        // 0x28: Glitch Input Filter Status
    uint32_t RESERVED3;            // 0x2C: Reserved
    volatile uint32_t SODR;        // 0x30: Set Output Data Register
    volatile uint32_t CODR;        // 0x34: Clear Output Data Register
    volatile uint32_t ODSR;        // 0x38: Output Data Status Register
    volatile uint32_t PDSR;        // 0x3C: Pin Data Status Register
    volatile uint32_t IER;         // 0x40: Interrupt Enable Register
    volatile uint32_t IDR;         // 0x44: Interrupt Disable Register
    volatile uint32_t IMR;         // 0x48: Interrupt Mask Register
    volatile uint32_t ISR;         // 0x4C: Interrupt Status Register
    volatile uint32_t MDER;        // 0x50: Multi-driver Enable Register
    volatile uint32_t MDDR;        // 0x54: Multi-driver Disable Register
    volatile uint32_t MDSR;        // 0x58: Multi-driver Status Register
    uint32_t RESERVED4;            // 0x5C: Reserved
    volatile uint32_t PUDR;        // 0x60: Pull-up Disable Register
    volatile uint32_t PUER;        // 0x64: Pull-up Enable Register
    volatile uint32_t PUSR;        // 0x68: Pull-up Status Register
    uint32_t RESERVED5;            // 0x6C: Reserved
    volatile uint32_t ABCDSR[2];   // 0x70-0x74: Peripheral ABCD Select Register
    uint32_t RESERVED6[2];         // 0x78-0x7C: Reserved
    volatile uint32_t IFSCDR;      // 0x80: Input Filter Slow Clock Disable
    volatile uint32_t IFSCER;      // 0x84: Input Filter Slow Clock Enable
    volatile uint32_t IFSCSR;      // 0x88: Input Filter Slow Clock Status
    volatile uint32_t SCDR;        // 0x8C: Slow Clock Divider Debouncing
    volatile uint32_t PPDDR;       // 0x90: Pad Pull-down Disable Register
    volatile uint32_t PPDER;       // 0x94: Pad Pull-down Enable Register
    volatile uint32_t PPDSR;       // 0x98: Pad Pull-down Status Register
    uint32_t RESERVED7;            // 0x9C: Reserved
    volatile uint32_t OWER;        // 0xA0: Output Write Enable
    volatile uint32_t OWDR;        // 0xA4: Output Write Disable
    volatile uint32_t OWSR;        // 0xA8: Output Write Status Register
    uint32_t RESERVED8;            // 0xAC: Reserved
    volatile uint32_t AIMER;       // 0xB0: Additional Interrupt Modes Enable
    volatile uint32_t AIMDR;       // 0xB4: Additional Interrupt Modes Disable
    volatile uint32_t AIMMR;       // 0xB8: Additional Interrupt Modes Mask
    uint32_t RESERVED9;            // 0xBC: Reserved
    volatile uint32_t ESR;         // 0xC0: Edge Select Register
    volatile uint32_t LSR;         // 0xC4: Level Select Register
    volatile uint32_t ELSR;        // 0xC8: Edge/Level Status Register
    uint32_t RESERVED10;           // 0xCC: Reserved
    volatile uint32_t FELLSR;      // 0xD0: Falling Edge/Low-Level Select
    volatile uint32_t REHLSR;      // 0xD4: Rising Edge/High-Level Select
    volatile uint32_t FRLHSR;      // 0xD8: Fall/Rise - Low/High Status
    uint32_t RESERVED11;           // 0xDC: Reserved
    volatile uint32_t LOCKSR;      // 0xE0: Lock Status
    volatile uint32_t WPMR;        // 0xE4: Write Protection Mode Register
    volatile uint32_t WPSR;        // 0xE8: Write Protection Status Register
    uint32_t RESERVED12[5];        // 0xEC-0xFC: Reserved
    volatile uint32_t SCHMITT;     // 0x100: Schmitt Trigger Register
    uint32_t RESERVED13[5];        // 0x104-0x114: Reserved
    volatile uint32_t DRIVER;      // 0x118: I/O Drive Register
    uint32_t RESERVED14[13];       // 0x11C-0x14C: Reserved
    volatile uint32_t PCMR;        // 0x150: Parallel Capture Mode Register
    volatile uint32_t PCIER;       // 0x154: Parallel Capture Interrupt Enable
    volatile uint32_t PCIDR;       // 0x158: Parallel Capture Interrupt Disable
    volatile uint32_t PCIMR;       // 0x15C: Parallel Capture Interrupt Mask
    volatile uint32_t PCISR;       // 0x160: Parallel Capture Interrupt Status
    volatile uint32_t PCRHR;       // 0x164: Parallel Capture Reception Holding
};

// PIO port instances
static_assert(sizeof(PIO_Registers) >= 0x164, "PIO_Registers size mismatch");

inline PIO_Registers* PIOA = reinterpret_cast<PIO_Registers*>(PIOA_BASE);
inline PIO_Registers* PIOB = reinterpret_cast<PIO_Registers*>(PIOB_BASE);
inline PIO_Registers* PIOC = reinterpret_cast<PIO_Registers*>(PIOC_BASE);
inline PIO_Registers* PIOD = reinterpret_cast<PIO_Registers*>(PIOD_BASE);
inline PIO_Registers* PIOE = reinterpret_cast<PIO_Registers*>(PIOE_BASE);

}  // namespace alloy::hal::atmel::samv71::atsamv71q21::hardware
