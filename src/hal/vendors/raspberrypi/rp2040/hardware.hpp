#pragma once

#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040::hardware {

// ============================================================================
// Hardware Register Definitions for RP2040
// Based on RP2040 Datasheet - SIO and PADS
// ============================================================================

// Memory map
constexpr uint32_t FLASH_BASE = 0x10000000;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = 264U * 1024U;

// SIO (Single-cycle I/O) base address
constexpr uint32_t SIO_BASE = 0xD0000000;

// IO Bank 0 base address (for FUNCSEL and other config)
constexpr uint32_t IO_BANK0_BASE = 0x40014000;

// PADS Bank 0 base address (for pull-up/down, drive strength)
constexpr uint32_t PADS_BANK0_BASE = 0x4001C000;

// ============================================================================
// SIO Register Structure
// ============================================================================

struct SIO_Registers {
    volatile uint32_t CPUID;              // 0x00: Processor core identifier
    volatile uint32_t GPIO_IN;            // 0x04: Input value for GPIO0-29
    volatile uint32_t GPIO_HI_IN;         // 0x08: Input value for GPIO30-47 (QSPI)
    uint32_t RESERVED0;                   // 0x0C
    volatile uint32_t GPIO_OUT;           // 0x10: GPIO output value
    volatile uint32_t GPIO_OUT_SET;       // 0x14: GPIO output value set
    volatile uint32_t GPIO_OUT_CLR;       // 0x18: GPIO output value clear
    volatile uint32_t GPIO_OUT_XOR;       // 0x1C: GPIO output value XOR
    volatile uint32_t GPIO_OE;            // 0x20: GPIO output enable
    volatile uint32_t GPIO_OE_SET;        // 0x24: GPIO output enable set
    volatile uint32_t GPIO_OE_CLR;        // 0x28: GPIO output enable clear
    volatile uint32_t GPIO_OE_XOR;        // 0x2C: GPIO output enable XOR
};

// IO Bank 0 - per GPIO config
struct IO_BANK0_GPIO_CTRL {
    volatile uint32_t STATUS;             // GPIO status
    volatile uint32_t CTRL;               // GPIO control (includes FUNCSEL)
};

struct IO_BANK0_Registers {
    IO_BANK0_GPIO_CTRL GPIO[30];          // GPIO0-GPIO29 control
};

// PADS Bank 0 - per GPIO pad config
struct PADS_BANK0_Registers {
    volatile uint32_t VOLTAGE_SELECT;     // 0x00: Voltage select
    volatile uint32_t GPIO[30];           // 0x04+: GPIO0-GPIO29 pad control
};

// SIO instance
inline SIO_Registers* SIO = reinterpret_cast<SIO_Registers*>(SIO_BASE);
inline IO_BANK0_Registers* IO_BANK0 = reinterpret_cast<IO_BANK0_Registers*>(IO_BANK0_BASE);
inline PADS_BANK0_Registers* PADS_BANK0 = reinterpret_cast<PADS_BANK0_Registers*>(PADS_BANK0_BASE);

// FUNCSEL values for IO_BANK0 CTRL register
constexpr uint32_t FUNCSEL_SPI    = 1;
constexpr uint32_t FUNCSEL_UART   = 2;
constexpr uint32_t FUNCSEL_I2C    = 3;
constexpr uint32_t FUNCSEL_PWM    = 4;
constexpr uint32_t FUNCSEL_SIO    = 5;  // GPIO
constexpr uint32_t FUNCSEL_PIO0   = 6;
constexpr uint32_t FUNCSEL_PIO1   = 7;
constexpr uint32_t FUNCSEL_CLOCK  = 8;
constexpr uint32_t FUNCSEL_USB    = 9;
constexpr uint32_t FUNCSEL_NULL   = 31;

// PADS control bits
constexpr uint32_t PADS_SLEWFAST  = (1 << 0);
constexpr uint32_t PADS_SCHMITT   = (1 << 1);
constexpr uint32_t PADS_PDE       = (1 << 2);  // Pull-down enable
constexpr uint32_t PADS_PUE       = (1 << 3);  // Pull-up enable
constexpr uint32_t PADS_DRIVE_2MA = (0 << 4);
constexpr uint32_t PADS_DRIVE_4MA = (1 << 4);
constexpr uint32_t PADS_DRIVE_8MA = (2 << 4);
constexpr uint32_t PADS_DRIVE_12MA = (3 << 4);
constexpr uint32_t PADS_IE        = (1 << 6);  // Input enable
constexpr uint32_t PADS_OD        = (1 << 7);  // Output disable

}  // namespace alloy::hal::raspberrypi::rp2040::hardware
