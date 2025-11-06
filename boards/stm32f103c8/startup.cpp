/// Startup code for STM32F103C8 (Cortex-M3)
/// Uses Alloy common startup framework

#include "../../src/startup/startup_common.hpp"
#include "../../src/hal/vendors/st/stm32f1/system_stm32f1.hpp"

// User application entry point
extern "C" int main();

// System initialization - called before main()
// Override this in your application if you need custom clock configuration
extern "C" __attribute__((weak)) void SystemInit() {
    // Initialize STM32F1 system (Cortex-M3, no FPU, no cache)
    // Uses default HSI clock (8 MHz)
    alloy::hal::st::stm32f1::system_init_default();
}

// Reset Handler - Entry point after reset
extern "C" [[noreturn]] void Reset_Handler() {
    // 1. Call system initialization (FPU, clock, flash latency, etc.)
    SystemInit();

    // 2. Perform runtime initialization (data/bss/constructors)
    alloy::startup::initialize_runtime();

    // 3. Call main
    main();

    // 4. If main returns, loop forever
    alloy::startup::infinite_loop();
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

    // Device-specific interrupt handlers
    void WWDG_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void PVD_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TAMPER_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void RTC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void FLASH_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void RCC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void EXTI4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Channel1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Channel2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Channel3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Channel4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Channel5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Channel6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA1_Channel7_IRQHandler() __attribute__((weak, alias("Default_Handler")));
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
    void RTCAlarm_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void USB_FS_WKUP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_BRK_TIM12_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_UP_TIM13_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_TRG_COM_TIM14_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM8_CC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void ADC3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void FSMC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SDIO_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void SPI3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void UART4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void UART5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void TIM7_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Channel1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Channel2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Channel3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
    void DMA2_Channel4_5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
}