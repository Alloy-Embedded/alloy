#include <cstdint>

#include "arch/cortex_m/startup.hpp"

extern "C" {

extern std::uint32_t _estack;

void SystemInit() __attribute__((weak));
void SystemInit() {}

void Default_Handler() { alloy::arch::cortex_m::default_handler(); }

void NMI_Handler() __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler() __attribute__((weak, alias("Default_Handler")));
void SVCall_Handler() __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler() __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

void WWDG_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RTC_STAMP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void EXTI0_1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void EXTI2_3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void EXTI4_15_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void UCPD1_UCPD2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void DMA_Channel1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void DMA_Channel2_3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void DMA_Channel4_5_6_7_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void ADC_COMP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_UP_TRG_COMP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM6_DAC_LPTIM1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM7_LPTIM2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM14_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM15_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM16_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TIM17_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void I2C1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void I2C2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void USART3_USART4_LPUART1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void CEC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RNG_IRQHandler() __attribute__((weak, alias("Default_Handler")));

[[noreturn]] void Reset_Handler() { alloy::arch::cortex_m::reset_and_enter_main(); }

__attribute__((section(".isr_vector"), used)) void (*const vector_table[])() = {
    reinterpret_cast<void (*)()>(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    SVCall_Handler,
    nullptr,
    nullptr,
    PendSV_Handler,
    SysTick_Handler,
    WWDG_IRQHandler,
    PVD_IRQHandler,
    RTC_STAMP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_1_IRQHandler,
    EXTI2_3_IRQHandler,
    EXTI4_15_IRQHandler,
    UCPD1_UCPD2_IRQHandler,
    DMA_Channel1_IRQHandler,
    DMA_Channel2_3_IRQHandler,
    DMA_Channel4_5_6_7_IRQHandler,
    ADC_COMP_IRQHandler,
    TIM1_BRK_UP_TRG_COMP_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM6_DAC_LPTIM1_IRQHandler,
    TIM7_LPTIM2_IRQHandler,
    TIM14_IRQHandler,
    TIM15_IRQHandler,
    TIM16_IRQHandler,
    TIM17_IRQHandler,
    I2C1_IRQHandler,
    I2C2_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_USART4_LPUART1_IRQHandler,
    CEC_IRQHandler,
    RNG_IRQHandler,
};

}  // extern "C"
