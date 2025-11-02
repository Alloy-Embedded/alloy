/// RP2040 PIO (Programmable I/O) Driver
///
/// PIO is a versatile hardware block that can emulate various serial protocols.
/// Perfect for driving WS2812 LEDs with precise timing.

#ifndef ALLOY_HAL_RP2040_PIO_HPP
#define ALLOY_HAL_RP2040_PIO_HPP

#include <cstdint>
#include "resets.hpp"

namespace alloy::hal::raspberrypi::rp2040 {

/// PIO State Machine registers (per SM)
struct PIO_SM_Registers {
    volatile uint32_t CLKDIV;         // Clock divider
    volatile uint32_t EXECCTRL;       // Execution control
    volatile uint32_t SHIFTCTRL;      // Shift control
    volatile uint32_t ADDR;           // Current instruction address
    volatile uint32_t INSTR;          // Instruction to execute
    volatile uint32_t PINCTRL;        // Pin control
};

/// PIO peripheral registers
struct PIO_Registers {
    volatile uint32_t CTRL;           // 0x000: Control
    volatile uint32_t FSTAT;          // 0x004: FIFO status
    volatile uint32_t FDEBUG;         // 0x008: FIFO debug
    volatile uint32_t FLEVEL;         // 0x00C: FIFO levels
    volatile uint32_t TXF[4];         // 0x010-0x01C: TX FIFOs
    volatile uint32_t RXF[4];         // 0x020-0x02C: RX FIFOs
    volatile uint32_t IRQ;            // 0x030: IRQ flags
    volatile uint32_t IRQ_FORCE;      // 0x034: IRQ force
    volatile uint32_t INPUT_SYNC_BYPASS; // 0x038: Input sync bypass
    volatile uint32_t DBG_PADOUT;     // 0x03C: Debug pad output
    volatile uint32_t DBG_PADOE;      // 0x040: Debug pad output enable
    volatile uint32_t DBG_CFGINFO;    // 0x044: Config info
    volatile uint32_t INSTR_MEM[32];  // 0x048-0x0C4: Instruction memory
    PIO_SM_Registers SM[4];           // 0x0C8+: State machines 0-3
    volatile uint32_t INTR;           // Interrupt request
    volatile uint32_t IRQ0_INTE;      // Interrupt enable for IRQ0
    volatile uint32_t IRQ0_INTF;      // Interrupt force for IRQ0
    volatile uint32_t IRQ0_INTS;      // Interrupt status for IRQ0
    volatile uint32_t IRQ1_INTE;      // Interrupt enable for IRQ1
    volatile uint32_t IRQ1_INTF;      // Interrupt force for IRQ1
    volatile uint32_t IRQ1_INTS;      // Interrupt status for IRQ1
};

/// PIO0 base address
static constexpr uint32_t PIO0_BASE = 0x50200000;
/// PIO1 base address
static constexpr uint32_t PIO1_BASE = 0x50300000;

/// Get PIO0 peripheral
inline PIO_Registers* get_pio0() {
    return reinterpret_cast<PIO_Registers*>(PIO0_BASE);
}

/// Get PIO1 peripheral
inline PIO_Registers* get_pio1() {
    return reinterpret_cast<PIO_Registers*>(PIO1_BASE);
}

/// Initialize PIO peripheral
inline void pio_init(PIO_Registers* pio) {
    // Release PIO from reset
    if (pio == get_pio0()) {
        reset_peripheral_blocking(ResetBits::PIO0);
    } else {
        reset_peripheral_blocking(ResetBits::PIO1);
    }
}

/// Load program into PIO instruction memory
/// @param pio PIO peripheral (PIO0 or PIO1)
/// @param program Array of 16-bit instructions
/// @param length Number of instructions
/// @param offset Offset in instruction memory (0-31)
inline void pio_load_program(PIO_Registers* pio, const uint16_t* program,
                              uint8_t length, uint8_t offset) {
    for (uint8_t i = 0; i < length; i++) {
        pio->INSTR_MEM[offset + i] = program[i];
    }
}

/// Configure PIO state machine for GPIO output
/// @param pio PIO peripheral
/// @param sm State machine number (0-3)
/// @param pin GPIO pin number
inline void pio_gpio_init(PIO_Registers* pio, uint8_t sm, uint8_t pin) {
    // Configure GPIO for PIO control (function 6 = PIO0, function 7 = PIO1)
    volatile uint32_t* io_ctrl = reinterpret_cast<volatile uint32_t*>(
        0x40014000 + 0x004 + (pin * 8));

    if (pio == get_pio0()) {
        *io_ctrl = 6;  // Function 6 = PIO0
    } else {
        *io_ctrl = 7;  // Function 7 = PIO1
    }

    // Set pin direction to output (controlled by PIO)
    // PIO will take control of the pin direction
}

/// Set PIO state machine clock divider
/// @param pio PIO peripheral
/// @param sm State machine number (0-3)
/// @param div Clock divider (16.16 fixed point)
inline void pio_sm_set_clkdiv(PIO_Registers* pio, uint8_t sm, uint32_t div) {
    pio->SM[sm].CLKDIV = div;
}

/// Enable/disable PIO state machine
/// @param pio PIO peripheral
/// @param sm State machine number (0-3)
/// @param enable true to enable, false to disable
inline void pio_sm_set_enabled(PIO_Registers* pio, uint8_t sm, bool enable) {
    if (enable) {
        pio->CTRL |= (1 << sm);  // Set enable bit for this SM
    } else {
        pio->CTRL &= ~(1 << sm); // Clear enable bit for this SM
    }
}

/// Check if TX FIFO has space
/// @param pio PIO peripheral
/// @param sm State machine number (0-3)
/// @return true if TX FIFO has space
inline bool pio_sm_tx_fifo_has_space(PIO_Registers* pio, uint8_t sm) {
    // FSTAT bit layout: bits 0-3 = TX FIFO full flags for SM 0-3
    return (pio->FSTAT & (1 << sm)) == 0;
}

/// Write to PIO TX FIFO (blocking)
/// @param pio PIO peripheral
/// @param sm State machine number (0-3)
/// @param data Data to write
inline void pio_sm_put_blocking(PIO_Registers* pio, uint8_t sm, uint32_t data) {
    while (!pio_sm_tx_fifo_has_space(pio, sm)) {
        // Wait for space in TX FIFO
    }
    pio->TXF[sm] = data;
}

} // namespace alloy::hal::raspberrypi::rp2040

#endif // ALLOY_HAL_RP2040_PIO_HPP
