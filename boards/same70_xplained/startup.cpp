#include <cstdint>

#include "arch/cortex_m/startup.hpp"

extern "C" {

extern std::uint32_t _estack;

void SystemInit() __attribute__((weak));
void SystemInit() {}

void Default_Handler() { alloy::arch::cortex_m::default_handler(); }

void NMI_Handler() __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler() __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler() __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler() __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler() __attribute__((weak, alias("Default_Handler")));
void SVCall_Handler() __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler() __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler() __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

void SUPC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RSTC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RTT_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void WDT_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PMC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void EFC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void UART0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void UART1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PIOA_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PIOB_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PIOC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void USART0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PIOD_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PIOE_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void HSMCI_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TWIHS0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TWIHS1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void SPI0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void SSC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC5_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void AFEC0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void DACC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PWM0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void ICM_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void ACC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void USBHS_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void MCAN0_INT0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void MCAN0_INT1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void MCAN1_INT0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void MCAN1_INT1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void GMAC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void AFEC1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TWIHS2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void QSPI_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void UART2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void UART3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC6_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC7_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC8_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC9_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC10_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TC11_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void AES_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void TRNG_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void XDMAC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void ISI_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PWM1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RSWDT_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void CCW_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void CCF_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void GMAC_Q1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void GMAC_Q2_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void IXC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void I2SC0_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void I2SC1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void GMAC_Q3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void GMAC_Q4_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void GMAC_Q5_IRQHandler() __attribute__((weak, alias("Default_Handler")));

[[noreturn]] void Reset_Handler() { alloy::arch::cortex_m::reset_and_enter_main(); }

}  // extern "C"

extern "C" {

__attribute__((section(".isr_vector"), used)) void (*const vector_table[])() = {
    reinterpret_cast<void (*)()>(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    SVCall_Handler,
    DebugMon_Handler,
    nullptr,
    PendSV_Handler,
    SysTick_Handler,
    SUPC_IRQHandler,
    RSTC_IRQHandler,
    RTC_IRQHandler,
    RTT_IRQHandler,
    WDT_IRQHandler,
    PMC_IRQHandler,
    EFC_IRQHandler,
    UART0_IRQHandler,
    UART1_IRQHandler,
    nullptr,
    PIOA_IRQHandler,
    PIOB_IRQHandler,
    PIOC_IRQHandler,
    USART0_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    PIOD_IRQHandler,
    PIOE_IRQHandler,
    HSMCI_IRQHandler,
    TWIHS0_IRQHandler,
    TWIHS1_IRQHandler,
    SPI0_IRQHandler,
    SSC_IRQHandler,
    TC0_IRQHandler,
    TC1_IRQHandler,
    TC2_IRQHandler,
    TC3_IRQHandler,
    TC4_IRQHandler,
    TC5_IRQHandler,
    AFEC0_IRQHandler,
    DACC_IRQHandler,
    PWM0_IRQHandler,
    ICM_IRQHandler,
    ACC_IRQHandler,
    USBHS_IRQHandler,
    MCAN0_INT0_IRQHandler,
    MCAN0_INT1_IRQHandler,
    MCAN1_INT0_IRQHandler,
    MCAN1_INT1_IRQHandler,
    GMAC_IRQHandler,
    AFEC1_IRQHandler,
    TWIHS2_IRQHandler,
    SPI1_IRQHandler,
    QSPI_IRQHandler,
    UART2_IRQHandler,
    UART3_IRQHandler,
    UART4_IRQHandler,
    TC6_IRQHandler,
    TC7_IRQHandler,
    TC8_IRQHandler,
    TC9_IRQHandler,
    TC10_IRQHandler,
    TC11_IRQHandler,
    nullptr,
    nullptr,
    nullptr,
    AES_IRQHandler,
    TRNG_IRQHandler,
    XDMAC_IRQHandler,
    ISI_IRQHandler,
    PWM1_IRQHandler,
    FPU_IRQHandler,
    nullptr,
    RSWDT_IRQHandler,
    CCW_IRQHandler,
    CCF_IRQHandler,
    GMAC_Q1_IRQHandler,
    GMAC_Q2_IRQHandler,
    IXC_IRQHandler,
    I2SC0_IRQHandler,
    I2SC1_IRQHandler,
    GMAC_Q3_IRQHandler,
    GMAC_Q4_IRQHandler,
    GMAC_Q5_IRQHandler,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

}  // extern "C"
