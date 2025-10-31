/// Startup code for RP2040 (Raspberry Pi Pico)
/// Uses Alloy common startup framework

#include "../../src/startup/startup_common.hpp"
#include <cstdint>

// Linker script symbols
extern uint32_t _estack;  // End of stack

// User application entry point
extern "C" int main();

// Note: SystemInit() is provided by startup_common.hpp with a weak default
// RP2040 typically doesn't need custom SystemInit as bootrom handles initialization

// Reset Handler - Entry point after reset
extern "C" [[noreturn]] void Reset_Handler() {
    // Perform runtime initialization (data/bss/constructors)
    alloy::startup::initialize_runtime();

    // Call system initialization (clock setup, etc.)
    SystemInit();

    // Call main
    main();

    // If main returns, loop forever
    alloy::startup::infinite_loop();
}

// Exception handlers (weak aliases to Default_Handler)
extern "C" {
    void NMI_Handler() __attribute__((weak, alias("Default_Handler")));
    void HardFault_Handler() __attribute__((weak, alias("Default_Handler")));
    void SVC_Handler() __attribute__((weak, alias("Default_Handler")));
    void PendSV_Handler() __attribute__((weak, alias("Default_Handler")));
    void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

    // RP2040 peripheral interrupts (32 total)
    void TIMER_IRQ_0_Handler() __attribute__((weak, alias("Default_Handler")));
    void TIMER_IRQ_1_Handler() __attribute__((weak, alias("Default_Handler")));
    void TIMER_IRQ_2_Handler() __attribute__((weak, alias("Default_Handler")));
    void TIMER_IRQ_3_Handler() __attribute__((weak, alias("Default_Handler")));
    void PWM_IRQ_WRAP_Handler() __attribute__((weak, alias("Default_Handler")));
    void USBCTRL_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void XIP_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIO0_IRQ_0_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIO0_IRQ_1_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIO1_IRQ_0_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIO1_IRQ_1_Handler() __attribute__((weak, alias("Default_Handler")));
    void DMA_IRQ_0_Handler() __attribute__((weak, alias("Default_Handler")));
    void DMA_IRQ_1_Handler() __attribute__((weak, alias("Default_Handler")));
    void IO_IRQ_BANK0_Handler() __attribute__((weak, alias("Default_Handler")));
    void IO_IRQ_QSPI_Handler() __attribute__((weak, alias("Default_Handler")));
    void SIO_IRQ_PROC0_Handler() __attribute__((weak, alias("Default_Handler")));
    void SIO_IRQ_PROC1_Handler() __attribute__((weak, alias("Default_Handler")));
    void CLOCKS_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void SPI0_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void SPI1_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void UART0_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void UART1_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void ADC_IRQ_FIFO_Handler() __attribute__((weak, alias("Default_Handler")));
    void I2C0_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void I2C1_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
    void RTC_IRQ_Handler() __attribute__((weak, alias("Default_Handler")));
}

// Boot2 stage (second stage bootloader - 256 bytes)
// This configures the external flash before jumping to main firmware
// For minimal implementation, we use a placeholder
__attribute__((section(".boot2"), used))
const uint8_t boot2[256] = {
    // This would normally be a complete second stage bootloader
    // For now, placeholder (real bootloader needed for hardware)
    0x00, 0xB5, 0x32, 0x4B, // Minimal bootloader stub
    // ... (rest filled with zeros)
};

// Vector table
using vector_table_entry_t = void (*)();

__attribute__((section(".isr_vector"), used))
const vector_table_entry_t vector_table[] = {
    // ARM Cortex-M0+ system exceptions
    reinterpret_cast<vector_table_entry_t>(&_estack),  // Initial stack pointer
    Reset_Handler,                // Reset
    NMI_Handler,                  // NMI
    HardFault_Handler,            // Hard Fault
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    SVC_Handler,                  // SVCall
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    PendSV_Handler,               // PendSV
    SysTick_Handler,              // SysTick

    // RP2040 peripheral interrupts (32 total)
    TIMER_IRQ_0_Handler,          // 0: Timer 0
    TIMER_IRQ_1_Handler,          // 1: Timer 1
    TIMER_IRQ_2_Handler,          // 2: Timer 2
    TIMER_IRQ_3_Handler,          // 3: Timer 3
    PWM_IRQ_WRAP_Handler,         // 4: PWM Wrap
    USBCTRL_IRQ_Handler,          // 5: USB Controller
    XIP_IRQ_Handler,              // 6: XIP (Execute In Place)
    PIO0_IRQ_0_Handler,           // 7: PIO 0 IRQ 0
    PIO0_IRQ_1_Handler,           // 8: PIO 0 IRQ 1
    PIO1_IRQ_0_Handler,           // 9: PIO 1 IRQ 0
    PIO1_IRQ_1_Handler,           // 10: PIO 1 IRQ 1
    DMA_IRQ_0_Handler,            // 11: DMA IRQ 0
    DMA_IRQ_1_Handler,            // 12: DMA IRQ 1
    IO_IRQ_BANK0_Handler,         // 13: IO Bank 0
    IO_IRQ_QSPI_Handler,          // 14: IO QSPI
    SIO_IRQ_PROC0_Handler,        // 15: SIO Processor 0
    SIO_IRQ_PROC1_Handler,        // 16: SIO Processor 1
    CLOCKS_IRQ_Handler,           // 17: Clocks
    SPI0_IRQ_Handler,             // 18: SPI 0
    SPI1_IRQ_Handler,             // 19: SPI 1
    UART0_IRQ_Handler,            // 20: UART 0
    UART1_IRQ_Handler,            // 21: UART 1
    ADC_IRQ_FIFO_Handler,         // 22: ADC FIFO
    I2C0_IRQ_Handler,             // 23: I2C 0
    I2C1_IRQ_Handler,             // 24: I2C 1
    RTC_IRQ_Handler,              // 25: RTC
    nullptr,                      // 26: Reserved
    nullptr,                      // 27: Reserved
    nullptr,                      // 28: Reserved
    nullptr,                      // 29: Reserved
    nullptr,                      // 30: Reserved
    nullptr,                      // 31: Reserved
};

// Entry point called by boot2
extern "C" [[noreturn]] void _entry_point() {
    Reset_Handler();
}
