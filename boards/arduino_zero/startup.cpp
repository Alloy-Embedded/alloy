/// Startup code for ATSAMD21G18 (Arduino Zero)
/// Minimal vector table and reset handler for Cortex-M0+

#include <cstdint>
#include <cstring>

// Linker script symbols
extern uint32_t _sidata;  // Start of .data in flash
extern uint32_t _sdata;   // Start of .data in RAM
extern uint32_t _edata;   // End of .data in RAM
extern uint32_t _sbss;    // Start of .bss
extern uint32_t _ebss;    // End of .bss
extern uint32_t _estack;  // End of stack

// User application entry point
extern "C" int main();

// System initialization (weak, can be overridden)
extern "C" void SystemInit() __attribute__((weak));
extern "C" void SystemInit() {
    // SAMD21 system initialization
    // Typically done by bootloader, but we can set up basics here
}

// Default handler for unhandled interrupts
extern "C" [[noreturn]] void Default_Handler() {
    while (true) {
        // Trap
    }
}

// Reset Handler - Entry point after reset
extern "C" [[noreturn]] void Reset_Handler() {
    // 1. Copy .data section from Flash to RAM
    uint32_t* src = &_sidata;
    uint32_t* dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    // 2. Zero out .bss section
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    // 3. Call system initialization
    SystemInit();

    // 4. Call static constructors
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();
    for (auto ctor = __init_array_start; ctor < __init_array_end; ++ctor) {
        (*ctor)();
    }

    // 5. Call main
    main();

    // 6. If main returns, loop forever
    while (true) {
        __asm__ volatile("wfi");  // Wait for interrupt
    }
}

// Exception handlers (weak aliases to Default_Handler)
extern "C" {
    void NMI_Handler() __attribute__((weak, alias("Default_Handler")));
    void HardFault_Handler() __attribute__((weak, alias("Default_Handler")));
    void SVC_Handler() __attribute__((weak, alias("Default_Handler")));
    void PendSV_Handler() __attribute__((weak, alias("Default_Handler")));
    void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

    // SAMD21 peripheral interrupts
    void PM_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SYSCTRL_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void WDT_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void RTC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EIC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void NVMCTRL_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMAC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void USB_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EVSYS_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SERCOM0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SERCOM1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SERCOM2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SERCOM3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SERCOM4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SERCOM5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TCC0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TCC1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TCC2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TC3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TC4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TC5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void ADC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void AC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DAC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void PTC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void I2S_IRQHandler() __attribute__((weak, alias("Default_Handler")));
}

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

    // SAMD21 peripheral interrupts (28 total)
    PM_IRQHandler,                // 0: Power Manager
    SYSCTRL_IRQHandler,           // 1: System Control
    WDT_IRQHandler,               // 2: Watchdog Timer
    RTC_IRQHandler,               // 3: Real-Time Counter
    EIC_IRQHandler,               // 4: External Interrupt Controller
    NVMCTRL_IRQHandler,           // 5: Non-Volatile Memory Controller
    DMAC_IRQHandler,              // 6: Direct Memory Access Controller
    USB_IRQHandler,               // 7: Universal Serial Bus
    EVSYS_IRQHandler,             // 8: Event System Interface
    SERCOM0_IRQHandler,           // 9: Serial Communication Interface 0
    SERCOM1_IRQHandler,           // 10: Serial Communication Interface 1
    SERCOM2_IRQHandler,           // 11: Serial Communication Interface 2
    SERCOM3_IRQHandler,           // 12: Serial Communication Interface 3
    SERCOM4_IRQHandler,           // 13: Serial Communication Interface 4
    SERCOM5_IRQHandler,           // 14: Serial Communication Interface 5
    TCC0_IRQHandler,              // 15: Timer Counter Control 0
    TCC1_IRQHandler,              // 16: Timer Counter Control 1
    TCC2_IRQHandler,              // 17: Timer Counter Control 2
    TC3_IRQHandler,               // 18: Basic Timer Counter 3
    TC4_IRQHandler,               // 19: Basic Timer Counter 4
    TC5_IRQHandler,               // 20: Basic Timer Counter 5
    nullptr,                      // 21: Reserved
    nullptr,                      // 22: Reserved
    ADC_IRQHandler,               // 23: Analog Digital Converter
    AC_IRQHandler,                // 24: Analog Comparators
    DAC_IRQHandler,               // 25: Digital Analog Converter
    PTC_IRQHandler,               // 26: Peripheral Touch Controller
    I2S_IRQHandler,               // 27: Inter-IC Sound Interface
};
