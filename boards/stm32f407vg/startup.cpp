/// Startup code for STM32F407VG
/// Minimal vector table and reset handler

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
    // Enable FPU (Cortex-M4F has hardware FPU)
    // CPACR: Coprocessor Access Control Register
    // CP10 and CP11 bits [23:20] = 0b1111 (full access)
    uint32_t* cpacr = reinterpret_cast<uint32_t*>(0xE000ED88);
    *cpacr |= (0xF << 20);
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

    // 3. Call system initialization (enables FPU)
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
    void MemManage_Handler() __attribute__((weak, alias("Default_Handler")));
    void BusFault_Handler() __attribute__((weak, alias("Default_Handler")));
    void UsageFault_Handler() __attribute__((weak, alias("Default_Handler")));
    void SVC_Handler() __attribute__((weak, alias("Default_Handler")));
    void DebugMon_Handler() __attribute__((weak, alias("Default_Handler")));
    void PendSV_Handler() __attribute__((weak, alias("Default_Handler")));
    void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

    // External interrupts (STM32F407 specific)
    void WWDG_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void PVD_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TAMP_STAMP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void RTC_WKUP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void FLASH_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void RCC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void ADC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN1_TX_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN1_RX0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN1_RX1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN1_SCE_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI9_5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM1_BRK_TIM9_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM1_UP_TIM10_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM1_TRG_COM_TIM11_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM1_CC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void I2C1_EV_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void I2C1_ER_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void I2C2_EV_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void I2C2_ER_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SPI1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SPI2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void USART1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void USART2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void USART3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI15_10_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void RTC_Alarm_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void OTG_FS_WKUP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_BRK_TIM12_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_UP_TIM13_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_TRG_COM_TIM14_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_CC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Stream7_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void FSMC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SDIO_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SPI3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void UART4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void UART5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM6_DAC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM7_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void ETH_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void ETH_WKUP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN2_TX_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN2_RX0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN2_RX1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CAN2_SCE_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void OTG_FS_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Stream7_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void USART6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void I2C3_EV_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void I2C3_ER_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void OTG_HS_EP1_OUT_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void OTG_HS_EP1_IN_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void OTG_HS_WKUP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void OTG_HS_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DCMI_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void CRYP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void HASH_RNG_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void FPU_IRQHandler() __attribute__((weak, alias("Default_Handler")));
}

// Vector table
using vector_table_entry_t = void (*)();

__attribute__((section(".isr_vector"), used))
const vector_table_entry_t vector_table[] = {
    // ARM Cortex-M4 system exceptions
    reinterpret_cast<vector_table_entry_t>(&_estack),  // Initial stack pointer
    Reset_Handler,                // Reset
    NMI_Handler,                  // NMI
    HardFault_Handler,            // Hard Fault
    MemManage_Handler,            // Memory Management Fault
    BusFault_Handler,             // Bus Fault
    UsageFault_Handler,           // Usage Fault
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    nullptr,                      // Reserved
    SVC_Handler,                  // SVCall
    DebugMon_Handler,             // Debug Monitor
    nullptr,                      // Reserved
    PendSV_Handler,               // PendSV
    SysTick_Handler,              // SysTick

    // STM32F407 peripheral interrupts
    WWDG_IRQHandler,              // Window Watchdog
    PVD_IRQHandler,               // PVD through EXTI Line
    TAMP_STAMP_IRQHandler,        // Tamper and TimeStamp
    RTC_WKUP_IRQHandler,          // RTC Wakeup
    FLASH_IRQHandler,             // FLASH
    RCC_IRQHandler,               // RCC
    EXTI0_IRQHandler,             // EXTI Line0
    EXTI1_IRQHandler,             // EXTI Line1
    EXTI2_IRQHandler,             // EXTI Line2
    EXTI3_IRQHandler,             // EXTI Line3
    EXTI4_IRQHandler,             // EXTI Line4
    DMA1_Stream0_IRQHandler,      // DMA1 Stream 0
    DMA1_Stream1_IRQHandler,      // DMA1 Stream 1
    DMA1_Stream2_IRQHandler,      // DMA1 Stream 2
    DMA1_Stream3_IRQHandler,      // DMA1 Stream 3
    DMA1_Stream4_IRQHandler,      // DMA1 Stream 4
    DMA1_Stream5_IRQHandler,      // DMA1 Stream 5
    DMA1_Stream6_IRQHandler,      // DMA1 Stream 6
    ADC_IRQHandler,               // ADC1, ADC2 and ADC3
    CAN1_TX_IRQHandler,           // CAN1 TX
    CAN1_RX0_IRQHandler,          // CAN1 RX0
    CAN1_RX1_IRQHandler,          // CAN1 RX1
    CAN1_SCE_IRQHandler,          // CAN1 SCE
    EXTI9_5_IRQHandler,           // EXTI Line[9:5]
    TIM1_BRK_TIM9_IRQHandler,     // TIM1 Break and TIM9
    TIM1_UP_TIM10_IRQHandler,     // TIM1 Update and TIM10
    TIM1_TRG_COM_TIM11_IRQHandler,// TIM1 Trigger and TIM11
    TIM1_CC_IRQHandler,           // TIM1 Capture Compare
    TIM2_IRQHandler,              // TIM2
    TIM3_IRQHandler,              // TIM3
    TIM4_IRQHandler,              // TIM4
    I2C1_EV_IRQHandler,           // I2C1 Event
    I2C1_ER_IRQHandler,           // I2C1 Error
    I2C2_EV_IRQHandler,           // I2C2 Event
    I2C2_ER_IRQHandler,           // I2C2 Error
    SPI1_IRQHandler,              // SPI1
    SPI2_IRQHandler,              // SPI2
    USART1_IRQHandler,            // USART1
    USART2_IRQHandler,            // USART2
    USART3_IRQHandler,            // USART3
    EXTI15_10_IRQHandler,         // EXTI Line[15:10]
    RTC_Alarm_IRQHandler,         // RTC Alarm through EXTI
    OTG_FS_WKUP_IRQHandler,       // USB OTG FS Wakeup
    TIM8_BRK_TIM12_IRQHandler,    // TIM8 Break and TIM12
    TIM8_UP_TIM13_IRQHandler,     // TIM8 Update and TIM13
    TIM8_TRG_COM_TIM14_IRQHandler,// TIM8 Trigger and TIM14
    TIM8_CC_IRQHandler,           // TIM8 Capture Compare
    DMA1_Stream7_IRQHandler,      // DMA1 Stream7
    FSMC_IRQHandler,              // FSMC
    SDIO_IRQHandler,              // SDIO
    TIM5_IRQHandler,              // TIM5
    SPI3_IRQHandler,              // SPI3
    UART4_IRQHandler,             // UART4
    UART5_IRQHandler,             // UART5
    TIM6_DAC_IRQHandler,          // TIM6 and DAC
    TIM7_IRQHandler,              // TIM7
    DMA2_Stream0_IRQHandler,      // DMA2 Stream 0
    DMA2_Stream1_IRQHandler,      // DMA2 Stream 1
    DMA2_Stream2_IRQHandler,      // DMA2 Stream 2
    DMA2_Stream3_IRQHandler,      // DMA2 Stream 3
    DMA2_Stream4_IRQHandler,      // DMA2 Stream 4
    ETH_IRQHandler,               // Ethernet
    ETH_WKUP_IRQHandler,          // Ethernet Wakeup
    CAN2_TX_IRQHandler,           // CAN2 TX
    CAN2_RX0_IRQHandler,          // CAN2 RX0
    CAN2_RX1_IRQHandler,          // CAN2 RX1
    CAN2_SCE_IRQHandler,          // CAN2 SCE
    OTG_FS_IRQHandler,            // USB OTG FS
    DMA2_Stream5_IRQHandler,      // DMA2 Stream 5
    DMA2_Stream6_IRQHandler,      // DMA2 Stream 6
    DMA2_Stream7_IRQHandler,      // DMA2 Stream 7
    USART6_IRQHandler,            // USART6
    I2C3_EV_IRQHandler,           // I2C3 Event
    I2C3_ER_IRQHandler,           // I2C3 Error
    OTG_HS_EP1_OUT_IRQHandler,    // USB OTG HS EP1 OUT
    OTG_HS_EP1_IN_IRQHandler,     // USB OTG HS EP1 IN
    OTG_HS_WKUP_IRQHandler,       // USB OTG HS Wakeup
    OTG_HS_IRQHandler,            // USB OTG HS
    DCMI_IRQHandler,              // DCMI
    CRYP_IRQHandler,              // CRYP crypto
    HASH_RNG_IRQHandler,          // Hash and RNG
    FPU_IRQHandler,               // FPU
};
