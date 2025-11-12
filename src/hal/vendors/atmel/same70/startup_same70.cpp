/**
 * @file startup_same70.cpp
 * @brief Modern C++23 Startup Code for SAME70Q21B
 *
 * Uses constexpr vector table builder and modern initialization hooks.
 * This file will be replaced by auto-generated version in Phase 14.2.
 */

#include "hal/vendors/arm/cortex_m7/startup_impl.hpp"
#include "hal/vendors/arm/cortex_m7/vector_table.hpp"
#include "startup_config.hpp"

using namespace alloy::hal::arm;
using namespace alloy::hal::same70;

// =============================================================================
// Forward Declarations
// =============================================================================

extern "C" {
    [[noreturn]] void Reset_Handler();
    void Default_Handler();
    void NMI_Handler();
    void HardFault_Handler();
    void MemManage_Handler();
    void BusFault_Handler();
    void UsageFault_Handler();
    void SVC_Handler();
    void DebugMon_Handler();
    void PendSV_Handler();
    void SysTick_Handler();

    // SAME70 specific IRQ handlers (weak aliases to Default_Handler)
    // These can be overridden by application
    void SUPC_Handler() __attribute__((weak, alias("Default_Handler")));
    void RSTC_Handler() __attribute__((weak, alias("Default_Handler")));
    void RTC_Handler() __attribute__((weak, alias("Default_Handler")));
    void RTT_Handler() __attribute__((weak, alias("Default_Handler")));
    void WDT_Handler() __attribute__((weak, alias("Default_Handler")));
    void PMC_Handler() __attribute__((weak, alias("Default_Handler")));
    void EEFC_Handler() __attribute__((weak, alias("Default_Handler")));
    void UART0_Handler() __attribute__((weak, alias("Default_Handler")));
    void UART1_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIOA_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIOB_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIOC_Handler() __attribute__((weak, alias("Default_Handler")));
    void USART0_Handler() __attribute__((weak, alias("Default_Handler")));
    void USART1_Handler() __attribute__((weak, alias("Default_Handler")));
    void USART2_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIOD_Handler() __attribute__((weak, alias("Default_Handler")));
    void PIOE_Handler() __attribute__((weak, alias("Default_Handler")));
    void HSMCI_Handler() __attribute__((weak, alias("Default_Handler")));
    void TWIHS0_Handler() __attribute__((weak, alias("Default_Handler")));
    void TWIHS1_Handler() __attribute__((weak, alias("Default_Handler")));
    void SPI0_Handler() __attribute__((weak, alias("Default_Handler")));
    void SSC_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC0_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC1_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC2_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC3_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC4_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC5_Handler() __attribute__((weak, alias("Default_Handler")));
    void AFEC0_Handler() __attribute__((weak, alias("Default_Handler")));
    void DACC_Handler() __attribute__((weak, alias("Default_Handler")));
    void PWM0_Handler() __attribute__((weak, alias("Default_Handler")));
    void ICM_Handler() __attribute__((weak, alias("Default_Handler")));
    void ACC_Handler() __attribute__((weak, alias("Default_Handler")));
    void USBHS_Handler() __attribute__((weak, alias("Default_Handler")));
    void MCAN0_Handler() __attribute__((weak, alias("Default_Handler")));
    void MCAN1_Handler() __attribute__((weak, alias("Default_Handler")));
    void GMAC_Handler() __attribute__((weak, alias("Default_Handler")));
    void AFEC1_Handler() __attribute__((weak, alias("Default_Handler")));
    void TWIHS2_Handler() __attribute__((weak, alias("Default_Handler")));
    void SPI1_Handler() __attribute__((weak, alias("Default_Handler")));
    void QSPI_Handler() __attribute__((weak, alias("Default_Handler")));
    void UART2_Handler() __attribute__((weak, alias("Default_Handler")));
    void UART3_Handler() __attribute__((weak, alias("Default_Handler")));
    void UART4_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC6_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC7_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC8_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC9_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC10_Handler() __attribute__((weak, alias("Default_Handler")));
    void TC11_Handler() __attribute__((weak, alias("Default_Handler")));
    void MLB_Handler() __attribute__((weak, alias("Default_Handler")));
    void AES_Handler() __attribute__((weak, alias("Default_Handler")));
    void TRNG_Handler() __attribute__((weak, alias("Default_Handler")));
    void XDMAC_Handler() __attribute__((weak, alias("Default_Handler")));
    void ISI_Handler() __attribute__((weak, alias("Default_Handler")));
    void PWM1_Handler() __attribute__((weak, alias("Default_Handler")));
    void FPU_Handler() __attribute__((weak, alias("Default_Handler")));
    void SDRAMC_Handler() __attribute__((weak, alias("Default_Handler")));
    void RSWDT_Handler() __attribute__((weak, alias("Default_Handler")));
    void CCW_Handler() __attribute__((weak, alias("Default_Handler")));
    void CCF_Handler() __attribute__((weak, alias("Default_Handler")));
    void GMAC_Q1_Handler() __attribute__((weak, alias("Default_Handler")));
    void GMAC_Q2_Handler() __attribute__((weak, alias("Default_Handler")));
    void IXC_Handler() __attribute__((weak, alias("Default_Handler")));
    void I2SC0_Handler() __attribute__((weak, alias("Default_Handler")));
    void I2SC1_Handler() __attribute__((weak, alias("Default_Handler")));
    void GMAC_Q3_Handler() __attribute__((weak, alias("Default_Handler")));
    void GMAC_Q4_Handler() __attribute__((weak, alias("Default_Handler")));
    void GMAC_Q5_Handler() __attribute__((weak, alias("Default_Handler")));
}

// =============================================================================
// Exception Handlers
// =============================================================================

/**
 * @brief Default handler for unhandled exceptions
 *
 * Infinite loop with NOP (visible in debugger).
 */
extern "C" void Default_Handler() {
    while (true) {
        __asm volatile("nop");
    }
}

/**
 * @brief Reset handler - entry point after reset
 *
 * Uses modern startup sequence with initialization hooks.
 */
extern "C" [[noreturn]] void Reset_Handler() {
    // Execute modern startup sequence
    StartupImpl::startup_sequence<StartupConfig>();

    // Never returns
}

/**
 * @brief NMI handler
 */
extern "C" void NMI_Handler() {
    while (true) {}
}

/**
 * @brief Hard fault handler
 */
extern "C" void HardFault_Handler() {
    while (true) {}
}

/**
 * @brief Memory management fault handler
 */
extern "C" void MemManage_Handler() {
    while (true) {}
}

/**
 * @brief Bus fault handler
 */
extern "C" void BusFault_Handler() {
    while (true) {}
}

/**
 * @brief Usage fault handler
 */
extern "C" void UsageFault_Handler() {
    while (true) {}
}

/**
 * @brief SVCall handler
 */
extern "C" void SVC_Handler() {
    // Empty - used by RTOS
}

/**
 * @brief Debug monitor handler
 */
extern "C" void DebugMon_Handler() {
    // Empty - used by debugger
}

/**
 * @brief PendSV handler
 */
extern "C" void PendSV_Handler() {
    // Empty - used by RTOS
}

/**
 * @brief SysTick handler (defined in board.cpp)
 */
// extern "C" void SysTick_Handler() is defined in board.cpp

// =============================================================================
// Vector Table (Compile-Time Construction)
// =============================================================================

/**
 * @brief SAME70 vector table
 *
 * Built at compile time using constexpr.
 * Zero runtime overhead.
 */
constexpr auto same70_vector_table = make_vector_table<StartupConfig::VECTOR_COUNT>()
    // Initial stack pointer and reset
    .set_stack_pointer(StartupConfig::stack_top())
    .set_reset_handler(&Reset_Handler)

    // Standard Cortex-M exceptions
    .set_nmi_handler(&NMI_Handler)
    .set_hard_fault_handler(&HardFault_Handler)
    .set_handler(StartupConfig::MEM_MANAGE_HANDLER_IDX, &MemManage_Handler)
    .set_handler(StartupConfig::BUS_FAULT_HANDLER_IDX, &BusFault_Handler)
    .set_handler(StartupConfig::USAGE_FAULT_HANDLER_IDX, &UsageFault_Handler)
    .set_handler(StartupConfig::SVCALL_HANDLER_IDX, &SVC_Handler)
    .set_handler(StartupConfig::DEBUG_MON_HANDLER_IDX, &DebugMon_Handler)
    .set_handler(StartupConfig::PENDSV_HANDLER_IDX, &PendSV_Handler)
    .set_systick_handler(&SysTick_Handler)

    // SAME70-specific IRQs (16-95)
    .set_handler(16, &SUPC_Handler)      // IRQ 0: Supply Controller
    .set_handler(17, &RSTC_Handler)      // IRQ 1: Reset Controller
    .set_handler(18, &RTC_Handler)       // IRQ 2: Real-time Clock
    .set_handler(19, &RTT_Handler)       // IRQ 3: Real-time Timer
    .set_handler(20, &WDT_Handler)       // IRQ 4: Watchdog Timer
    .set_handler(21, &PMC_Handler)       // IRQ 5: Power Management
    .set_handler(22, &EEFC_Handler)      // IRQ 6: Flash Controller
    .set_handler(23, &UART0_Handler)     // IRQ 7: UART 0
    .set_handler(24, &UART1_Handler)     // IRQ 8: UART 1
    .set_handler(25, &PIOA_Handler)      // IRQ 9: PIO Controller A
    .set_handler(26, &PIOB_Handler)      // IRQ 10: PIO Controller B
    .set_handler(27, &PIOC_Handler)      // IRQ 11: PIO Controller C
    .set_handler(28, &USART0_Handler)    // IRQ 12: USART 0
    .set_handler(29, &USART1_Handler)    // IRQ 13: USART 1
    .set_handler(30, &USART2_Handler)    // IRQ 14: USART 2
    .set_handler(31, &PIOD_Handler)      // IRQ 15: PIO Controller D
    .set_handler(32, &PIOE_Handler)      // IRQ 16: PIO Controller E
    .set_handler(33, &HSMCI_Handler)     // IRQ 17: HSMCI
    .set_handler(34, &TWIHS0_Handler)    // IRQ 18: Two-wire Interface 0
    .set_handler(35, &TWIHS1_Handler)    // IRQ 19: Two-wire Interface 1
    .set_handler(36, &SPI0_Handler)      // IRQ 20: SPI 0
    .set_handler(37, &SSC_Handler)       // IRQ 21: SSC
    .set_handler(38, &TC0_Handler)       // IRQ 22: Timer Counter 0
    .set_handler(39, &TC1_Handler)       // IRQ 23: Timer Counter 1
    .set_handler(40, &TC2_Handler)       // IRQ 24: Timer Counter 2
    .set_handler(41, &TC3_Handler)       // IRQ 25: Timer Counter 3
    .set_handler(42, &TC4_Handler)       // IRQ 26: Timer Counter 4
    .set_handler(43, &TC5_Handler)       // IRQ 27: Timer Counter 5
    .set_handler(44, &AFEC0_Handler)     // IRQ 28: ADC 0
    .set_handler(45, &DACC_Handler)      // IRQ 29: DAC
    .set_handler(46, &PWM0_Handler)      // IRQ 30: PWM 0
    .set_handler(47, &ICM_Handler)       // IRQ 31: Integrity Check Monitor
    .set_handler(48, &ACC_Handler)       // IRQ 32: Analog Comparator
    .set_handler(49, &USBHS_Handler)     // IRQ 33: USB High Speed
    .set_handler(50, &MCAN0_Handler)     // IRQ 34: CAN 0
    .set_handler(51, &MCAN1_Handler)     // IRQ 35: CAN 1
    .set_handler(52, &GMAC_Handler)      // IRQ 36: Ethernet MAC
    .set_handler(53, &AFEC1_Handler)     // IRQ 37: ADC 1
    .set_handler(54, &TWIHS2_Handler)    // IRQ 38: Two-wire Interface 2
    .set_handler(55, &SPI1_Handler)      // IRQ 39: SPI 1
    .set_handler(56, &QSPI_Handler)      // IRQ 40: QSPI
    .set_handler(57, &UART2_Handler)     // IRQ 41: UART 2
    .set_handler(58, &UART3_Handler)     // IRQ 42: UART 3
    .set_handler(59, &UART4_Handler)     // IRQ 43: UART 4
    .set_handler(60, &TC6_Handler)       // IRQ 44: Timer Counter 6
    .set_handler(61, &TC7_Handler)       // IRQ 45: Timer Counter 7
    .set_handler(62, &TC8_Handler)       // IRQ 46: Timer Counter 8
    .set_handler(63, &TC9_Handler)       // IRQ 47: Timer Counter 9
    .set_handler(64, &TC10_Handler)      // IRQ 48: Timer Counter 10
    .set_handler(65, &TC11_Handler)      // IRQ 49: Timer Counter 11
    .set_handler(66, &MLB_Handler)       // IRQ 50: MediaLB
    .set_handler(69, &AES_Handler)       // IRQ 53: AES
    .set_handler(70, &TRNG_Handler)      // IRQ 54: True Random Generator
    .set_handler(71, &XDMAC_Handler)     // IRQ 55: DMA Controller
    .set_handler(72, &ISI_Handler)       // IRQ 56: Image Sensor Interface
    .set_handler(73, &PWM1_Handler)      // IRQ 57: PWM 1
    .set_handler(74, &FPU_Handler)       // IRQ 58: FPU
    .set_handler(75, &SDRAMC_Handler)    // IRQ 59: SDRAM Controller
    .set_handler(76, &RSWDT_Handler)     // IRQ 60: Reinforced Watchdog
    .set_handler(77, &CCW_Handler)       // IRQ 61: Cache Warning
    .set_handler(78, &CCF_Handler)       // IRQ 62: Cache Fault
    .set_handler(79, &GMAC_Q1_Handler)   // IRQ 63: Ethernet Queue 1
    .set_handler(80, &GMAC_Q2_Handler)   // IRQ 64: Ethernet Queue 2
    .set_handler(81, &IXC_Handler)       // IRQ 65: Floating Point Exception
    .set_handler(85, &I2SC0_Handler)     // IRQ 69: I2S 0
    .set_handler(86, &I2SC1_Handler)     // IRQ 70: I2S 1
    .set_handler(87, &GMAC_Q3_Handler)   // IRQ 71: Ethernet Queue 3
    .set_handler(88, &GMAC_Q4_Handler)   // IRQ 72: Ethernet Queue 4
    .set_handler(89, &GMAC_Q5_Handler)   // IRQ 73: Ethernet Queue 5

    // Reserved vectors set to nullptr
    .set_reserved_null()

    .get();

/**
 * @brief Place vector table in .isr_vector section
 *
 * Linker script places this at the beginning of flash.
 */
__attribute__((section(".isr_vector"), used))
const auto vectors = same70_vector_table;

/**
 * @brief Verify vector table at compile time
 *
 * Ensure vector table was constructed correctly.
 */
static_assert(same70_vector_table.size() == StartupConfig::VECTOR_COUNT,
              "Vector table size mismatch");
