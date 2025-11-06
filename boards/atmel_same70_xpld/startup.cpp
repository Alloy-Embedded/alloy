/// Startup code for ATSAME70Q21 (Cortex-M7)
/// Uses Alloy common startup framework with Cortex-M7 initialization

#include "../../src/startup/startup_common.hpp"
#include "../../src/startup/arm_cortex_m7/cortex_m7_init.hpp"
#include <stdint.h>

// Linker script symbols
extern uint32_t _estack;  // End of stack

// User application entry point
extern "C" int main();

// System initialization - called before main()
// Users can override this for custom clock/peripheral configuration
extern "C" __attribute__((weak)) void SystemInit() {
    // CRITICAL: Disable BOTH watchdog timers FIRST!
    // SAME70 has two watchdogs that must be disabled:
    // 1. WDT (Watchdog Timer) at 0x400E1854
    // 2. RSWDT (Reinforced Watchdog Timer) at 0x400E1D54
    //
    // If not disabled, RSWDT will reset the MCU periodically!

    volatile uint32_t* WDT_MR = reinterpret_cast<volatile uint32_t*>(0x400E1854);
    volatile uint32_t* RSWDT_MR = reinterpret_cast<volatile uint32_t*>(0x400E1D54);
    constexpr uint32_t WDDIS = (1U << 15);

    *WDT_MR = WDDIS;      // Disable normal watchdog
    *RSWDT_MR = WDDIS;    // Disable reinforced watchdog

    // Small delay to ensure watchdogs are disabled
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile("nop");
    }

    // Initialize Cortex-M7 features:
    // - FPU (double precision) for hardware floating point
    // - I-Cache and D-Cache: DISABLED for now (needs proper cache invalidation)
    //
    // TODO: Fix cache initialization - currently causes system hang
    // The cache invalidation code needs to be updated for SAME70's cache geometry
    alloy::arm::cortex_m7::initialize(
        true,   // Enable FPU
        true,   // Enable lazy FPU stacking
        false,  // Disable I-Cache (TODO: fix cache initialization)
        false   // Disable D-Cache (TODO: fix cache initialization)
    );

    // TODO: Configure system clock to 300MHz using PLL
    // Default: MCU starts with internal RC oscillator (4-12 MHz)
    //
    // For production use, configure PLL:
    // 1. Enable 12MHz external crystal (XTAL)
    // 2. Configure PLL: 12MHz * 50 / 2 = 300MHz
    // 3. Set flash wait states for 300MHz
    // 4. Switch to PLL as system clock
    //
    // Note: This requires PMC (Power Management Controller) HAL
}

// Reset Handler - Entry point after reset
extern "C" [[noreturn]] void Reset_Handler() {
    // TEMPORARY: Disable watchdogs manually here for testing
    volatile uint32_t* WDT_MR = reinterpret_cast<volatile uint32_t*>(0x400E1854);
    volatile uint32_t* RSWDT_MR = reinterpret_cast<volatile uint32_t*>(0x400E1D54);
    *WDT_MR = (1U << 15);     // WDDIS
    *RSWDT_MR = (1U << 15);   // WDDIS

    // Small delay
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile("nop");
    }

    // TEMPORARY: Skip SystemInit() and initialize_runtime() for testing
    // SystemInit();
    // alloy::startup::initialize_runtime();

    // 3. Call main
    main();

    // 4. If main returns, loop forever with WFI
    alloy::startup::infinite_loop();
}

// ============================================================================
// ARM Cortex-M Exception Handlers
// ============================================================================

extern "C" {
    // Standard Cortex-M7 exception handlers (weak aliases)
    void NMI_Handler() __attribute__((weak, alias("Default_Handler")));
    void HardFault_Handler() __attribute__((weak, alias("Default_Handler")));
    void MemManage_Handler() __attribute__((weak, alias("Default_Handler")));
    void BusFault_Handler() __attribute__((weak, alias("Default_Handler")));
    void UsageFault_Handler() __attribute__((weak, alias("Default_Handler")));
    void SVC_Handler() __attribute__((weak, alias("Default_Handler")));
    void DebugMon_Handler() __attribute__((weak, alias("Default_Handler")));
    void PendSV_Handler() __attribute__((weak, alias("Default_Handler")));
    void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

    // ATSAME70Q21 Peripheral Interrupt Handlers
    // Users can override these by providing non-weak implementations

    void SUPC_Handler() __attribute__((weak, alias("Default_Handler")));     // Supply Controller
    void RSTC_Handler() __attribute__((weak, alias("Default_Handler")));     // Reset Controller
    void RTC_Handler() __attribute__((weak, alias("Default_Handler")));      // Real-time Clock
    void RTT_Handler() __attribute__((weak, alias("Default_Handler")));      // Real-time Timer
    void WDT_Handler() __attribute__((weak, alias("Default_Handler")));      // Watchdog Timer
    void PMC_Handler() __attribute__((weak, alias("Default_Handler")));      // Power Management
    void EFC_Handler() __attribute__((weak, alias("Default_Handler")));      // Embedded Flash
    void UART0_Handler() __attribute__((weak, alias("Default_Handler")));    // UART 0
    void UART1_Handler() __attribute__((weak, alias("Default_Handler")));    // UART 1
    void PIOA_Handler() __attribute__((weak, alias("Default_Handler")));     // GPIO Port A
    void PIOB_Handler() __attribute__((weak, alias("Default_Handler")));     // GPIO Port B
    void PIOC_Handler() __attribute__((weak, alias("Default_Handler")));     // GPIO Port C
    void USART0_Handler() __attribute__((weak, alias("Default_Handler")));   // USART 0
    void USART1_Handler() __attribute__((weak, alias("Default_Handler")));   // USART 1
    void USART2_Handler() __attribute__((weak, alias("Default_Handler")));   // USART 2
    void PIOD_Handler() __attribute__((weak, alias("Default_Handler")));     // GPIO Port D
    void PIOE_Handler() __attribute__((weak, alias("Default_Handler")));     // GPIO Port E
    void HSMCI_Handler() __attribute__((weak, alias("Default_Handler")));    // SD/MMC
    void TWIHS0_Handler() __attribute__((weak, alias("Default_Handler")));   // I2C 0
    void TWIHS1_Handler() __attribute__((weak, alias("Default_Handler")));   // I2C 1
    void SPI0_Handler() __attribute__((weak, alias("Default_Handler")));     // SPI 0
    void SSC_Handler() __attribute__((weak, alias("Default_Handler")));      // Synchronous Serial
    void TC0_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 0
    void TC1_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 1
    void TC2_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 2
    void TC3_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 3
    void TC4_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 4
    void TC5_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 5
    void AFEC0_Handler() __attribute__((weak, alias("Default_Handler")));    // ADC 0
    void DACC_Handler() __attribute__((weak, alias("Default_Handler")));     // DAC
    void PWM0_Handler() __attribute__((weak, alias("Default_Handler")));     // PWM 0
    void ICM_Handler() __attribute__((weak, alias("Default_Handler")));      // Integrity Check
    void ACC_Handler() __attribute__((weak, alias("Default_Handler")));      // Analog Comparator
    void USBHS_Handler() __attribute__((weak, alias("Default_Handler")));    // USB High Speed
    void MCAN0_INT0_Handler() __attribute__((weak, alias("Default_Handler")));  // CAN 0 Line 0
    void MCAN0_INT1_Handler() __attribute__((weak, alias("Default_Handler")));  // CAN 0 Line 1
    void MCAN1_INT0_Handler() __attribute__((weak, alias("Default_Handler")));  // CAN 1 Line 0
    void MCAN1_INT1_Handler() __attribute__((weak, alias("Default_Handler")));  // CAN 1 Line 1
    void GMAC_Handler() __attribute__((weak, alias("Default_Handler")));     // Ethernet MAC
    void AFEC1_Handler() __attribute__((weak, alias("Default_Handler")));    // ADC 1
    void TWIHS2_Handler() __attribute__((weak, alias("Default_Handler")));   // I2C 2
    void SPI1_Handler() __attribute__((weak, alias("Default_Handler")));     // SPI 1
    void QSPI_Handler() __attribute__((weak, alias("Default_Handler")));     // Quad SPI
    void UART2_Handler() __attribute__((weak, alias("Default_Handler")));    // UART 2
    void UART3_Handler() __attribute__((weak, alias("Default_Handler")));    // UART 3
    void UART4_Handler() __attribute__((weak, alias("Default_Handler")));    // UART 4
    void TC6_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 6
    void TC7_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 7
    void TC8_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 8
    void TC9_Handler() __attribute__((weak, alias("Default_Handler")));      // Timer Counter 9
    void TC10_Handler() __attribute__((weak, alias("Default_Handler")));     // Timer Counter 10
    void TC11_Handler() __attribute__((weak, alias("Default_Handler")));     // Timer Counter 11
    void AES_Handler() __attribute__((weak, alias("Default_Handler")));      // AES
    void TRNG_Handler() __attribute__((weak, alias("Default_Handler")));     // True RNG
    void XDMAC_Handler() __attribute__((weak, alias("Default_Handler")));    // DMA Controller
    void ISI_Handler() __attribute__((weak, alias("Default_Handler")));      // Image Sensor
    void PWM1_Handler() __attribute__((weak, alias("Default_Handler")));     // PWM 1
    void FPU_Handler() __attribute__((weak, alias("Default_Handler")));      // FPU Exception
    void SDRAMC_Handler() __attribute__((weak, alias("Default_Handler")));   // SDRAM Controller
    void RSWDT_Handler() __attribute__((weak, alias("Default_Handler")));    // Reinforced WDT
    void GMAC_Q1_Handler() __attribute__((weak, alias("Default_Handler")));  // Ethernet Queue 1
    void GMAC_Q2_Handler() __attribute__((weak, alias("Default_Handler")));  // Ethernet Queue 2
    void IXC_Handler() __attribute__((weak, alias("Default_Handler")));      // Floating Point
}

// ============================================================================
// VECTOR TABLE
// ============================================================================
// This table is placed at the beginning of flash (0x00400000)
// by the linker script (.isr_vector section)

using vector_t = void(*)();

__attribute__((section(".isr_vector"), used))
const vector_t vector_table[] = {
    // Cortex-M7 Core Exceptions
    reinterpret_cast<vector_t>(&_estack),  // 0: Initial Stack Pointer
    Reset_Handler,                          // 1: Reset Handler
    NMI_Handler,                            // 2: NMI Handler
    HardFault_Handler,                      // 3: Hard Fault Handler
    MemManage_Handler,                      // 4: MPU Fault Handler
    BusFault_Handler,                       // 5: Bus Fault Handler
    UsageFault_Handler,                     // 6: Usage Fault Handler
    nullptr,                                // 7: Reserved
    nullptr,                                // 8: Reserved
    nullptr,                                // 9: Reserved
    nullptr,                                // 10: Reserved
    SVC_Handler,                            // 11: SVCall Handler
    DebugMon_Handler,                       // 12: Debug Monitor Handler
    nullptr,                                // 13: Reserved
    PendSV_Handler,                         // 14: PendSV Handler
    SysTick_Handler,                        // 15: SysTick Handler

    // ATSAME70Q21 Peripheral Interrupts (IRQ0-IRQ63)
    SUPC_Handler,           // 16: IRQ0  - Supply Controller
    RSTC_Handler,           // 17: IRQ1  - Reset Controller
    RTC_Handler,            // 18: IRQ2  - Real-time Clock
    RTT_Handler,            // 19: IRQ3  - Real-time Timer
    WDT_Handler,            // 20: IRQ4  - Watchdog Timer
    PMC_Handler,            // 21: IRQ5  - Power Management
    EFC_Handler,            // 22: IRQ6  - Embedded Flash
    UART0_Handler,          // 23: IRQ7  - UART 0
    UART1_Handler,          // 24: IRQ8  - UART 1
    nullptr,                // 25: IRQ9  - Reserved
    PIOA_Handler,           // 26: IRQ10 - GPIO Port A
    PIOB_Handler,           // 27: IRQ11 - GPIO Port B
    PIOC_Handler,           // 28: IRQ12 - GPIO Port C
    USART0_Handler,         // 29: IRQ13 - USART 0
    USART1_Handler,         // 30: IRQ14 - USART 1
    USART2_Handler,         // 31: IRQ15 - USART 2
    PIOD_Handler,           // 32: IRQ16 - GPIO Port D
    PIOE_Handler,           // 33: IRQ17 - GPIO Port E
    HSMCI_Handler,          // 34: IRQ18 - SD/MMC
    TWIHS0_Handler,         // 35: IRQ19 - I2C 0
    TWIHS1_Handler,         // 36: IRQ20 - I2C 1
    SPI0_Handler,           // 37: IRQ21 - SPI 0
    SSC_Handler,            // 38: IRQ22 - Synchronous Serial
    TC0_Handler,            // 39: IRQ23 - Timer Counter 0
    TC1_Handler,            // 40: IRQ24 - Timer Counter 1
    TC2_Handler,            // 41: IRQ25 - Timer Counter 2
    TC3_Handler,            // 42: IRQ26 - Timer Counter 3
    TC4_Handler,            // 43: IRQ27 - Timer Counter 4
    TC5_Handler,            // 44: IRQ28 - Timer Counter 5
    AFEC0_Handler,          // 45: IRQ29 - ADC 0
    DACC_Handler,           // 46: IRQ30 - DAC
    PWM0_Handler,           // 47: IRQ31 - PWM 0
    ICM_Handler,            // 48: IRQ32 - Integrity Check
    ACC_Handler,            // 49: IRQ33 - Analog Comparator
    USBHS_Handler,          // 50: IRQ34 - USB High Speed
    MCAN0_INT0_Handler,     // 51: IRQ35 - CAN 0 Line 0
    MCAN0_INT1_Handler,     // 52: IRQ36 - CAN 0 Line 1
    MCAN1_INT0_Handler,     // 53: IRQ37 - CAN 1 Line 0
    MCAN1_INT1_Handler,     // 54: IRQ38 - CAN 1 Line 1
    GMAC_Handler,           // 55: IRQ39 - Ethernet MAC
    AFEC1_Handler,          // 56: IRQ40 - ADC 1
    TWIHS2_Handler,         // 57: IRQ41 - I2C 2
    SPI1_Handler,           // 58: IRQ42 - SPI 1
    QSPI_Handler,           // 59: IRQ43 - Quad SPI
    UART2_Handler,          // 60: IRQ44 - UART 2
    UART3_Handler,          // 61: IRQ45 - UART 3
    UART4_Handler,          // 62: IRQ46 - UART 4
    TC6_Handler,            // 63: IRQ47 - Timer Counter 6
    TC7_Handler,            // 64: IRQ48 - Timer Counter 7
    TC8_Handler,            // 65: IRQ49 - Timer Counter 8
    TC9_Handler,            // 66: IRQ50 - Timer Counter 9
    TC10_Handler,           // 67: IRQ51 - Timer Counter 10
    TC11_Handler,           // 68: IRQ52 - Timer Counter 11
    nullptr,                // 69: IRQ53 - Reserved
    nullptr,                // 70: IRQ54 - Reserved
    nullptr,                // 71: IRQ55 - Reserved
    AES_Handler,            // 72: IRQ56 - AES
    TRNG_Handler,           // 73: IRQ57 - True RNG
    XDMAC_Handler,          // 74: IRQ58 - DMA Controller
    ISI_Handler,            // 75: IRQ59 - Image Sensor
    PWM1_Handler,           // 76: IRQ60 - PWM 1
    FPU_Handler,            // 77: IRQ61 - FPU Exception
    SDRAMC_Handler,         // 78: IRQ62 - SDRAM Controller
    RSWDT_Handler,          // 79: IRQ63 - Reinforced WDT
    nullptr,                // 80: IRQ64 - Reserved
    nullptr,                // 81: IRQ65 - Reserved
    nullptr,                // 82: IRQ66 - Reserved
    GMAC_Q1_Handler,        // 83: IRQ67 - Ethernet Queue 1
    GMAC_Q2_Handler,        // 84: IRQ68 - Ethernet Queue 2
    IXC_Handler,            // 85: IRQ69 - Floating Point
};
