# Spec: Interrupt Management with Hardware Policies

## Overview

Integrate interrupt management into the hardware policy architecture. Generate platform-specific IRQ tables from SVD files and provide policies for interrupt controller access (NVIC, PLIC, etc).

## Problem Statement

The current interrupt implementation has several issues:

1. **Hardcoded IRQ numbers**: `IrqNumber` enum has generic values that don't match real hardware
2. **No platform-specific mappings**: Each MCU has different IRQ numbers for same peripherals
3. **Stub implementations**: All functions return placeholder values
4. **No interrupt tables**: Missing IRQ name → number mappings per platform
5. **Disconnected from peripherals**: No link between peripheral policies and their IRQs

**Example Problem:**
```cpp
// interface/interrupt.hpp - WRONG!
enum class IrqNumber : u16 {
    UART0 = 10,  // ❌ This is NOT the real IRQ number for any MCU
    UART1 = 11,  // ❌ SAME70 has UART0 at IRQ 7, STM32F4 has USART1 at IRQ 37
};
```

**What we need:**
- **SAME70**: UART0 → IRQ 7, UART1 → IRQ 8, SPI0 → IRQ 24
- **STM32F4**: USART1 → IRQ 37, USART2 → IRQ 38, SPI1 → IRQ 35

---

## Solution Overview

### 1. Generate IRQ Tables from SVD

Parse `<interrupt>` tags from SVD files to extract IRQ numbers per platform.

### 2. Create Interrupt Hardware Policy

Similar to UART/SPI policies, create interrupt controller policy for each platform.

### 3. Link Peripherals to IRQs

Peripheral policies should expose their IRQ number for use with interrupt APIs.

---

## ADDED Requirements

### Requirement: Platform-Specific IRQ Tables

Each platform SHALL have generated IRQ tables mapping peripheral names to IRQ numbers.

**Rationale**: IRQ numbers vary widely between MCU families. Generic enums cause bugs.

#### SVD Input Example

```xml
<!-- STM32F429.svd -->
<peripherals>
  <peripheral>
    <name>USART1</name>
    <baseAddress>0x40011000</baseAddress>
    <interrupt>
      <name>USART1</name>
      <description>USART1 global interrupt</description>
      <value>37</value>
    </interrupt>
  </peripheral>

  <peripheral>
    <name>SPI1</name>
    <baseAddress>0x40013000</baseAddress>
    <interrupt>
      <name>SPI1</name>
      <description>SPI1 global interrupt</description>
      <value>35</value>
    </interrupt>
  </peripheral>
</peripherals>
```

#### Generated IRQ Table

```cpp
// hal/vendors/st/stm32f4/irq_table.hpp (generated)
/**
 * @file irq_table.hpp
 * @brief IRQ number mappings for STM32F4
 *
 * Auto-generated from: STM32F429.svd
 * Generator: interrupt_table_generator.py
 */

#pragma once

#include "core/types.hpp"

namespace alloy::hal::st::stm32f4 {

using namespace alloy::core;

/**
 * @brief Platform-specific IRQ numbers
 *
 * These numbers match the NVIC interrupt table for STM32F4.
 */
enum class IrqNumber : u16 {
    // System exceptions
    Reset = 1,
    NMI = 2,
    HardFault = 3,
    MemManage = 4,
    BusFault = 5,
    UsageFault = 6,
    SVCall = 11,
    DebugMonitor = 12,
    PendSV = 14,
    SysTick = 15,

    // External interrupts (peripheral IRQs)
    WWDG = 16,              // Window Watchdog
    PVD = 17,               // PVD through EXTI line
    TAMP_STAMP = 18,        // Tamper and TimeStamp
    RTC_WKUP = 19,          // RTC Wakeup

    // Communication peripherals
    USART1 = 37,            // USART1 global interrupt
    USART2 = 38,            // USART2 global interrupt
    USART3 = 39,            // USART3 global interrupt
    UART4 = 52,             // UART4 global interrupt
    UART5 = 53,             // UART5 global interrupt
    USART6 = 71,            // USART6 global interrupt

    SPI1 = 35,              // SPI1 global interrupt
    SPI2 = 36,              // SPI2 global interrupt
    SPI3 = 51,              // SPI3 global interrupt
    SPI4 = 84,              // SPI4 global interrupt

    I2C1_EV = 31,           // I2C1 event interrupt
    I2C1_ER = 32,           // I2C1 error interrupt
    I2C2_EV = 33,           // I2C2 event interrupt
    I2C2_ER = 34,           // I2C2 error interrupt
    I2C3_EV = 72,           // I2C3 event interrupt
    I2C3_ER = 73,           // I2C3 error interrupt

    // DMA
    DMA1_Stream0 = 11,      // DMA1 Stream 0
    DMA1_Stream1 = 12,      // DMA1 Stream 1
    DMA1_Stream2 = 13,      // DMA1 Stream 2
    DMA1_Stream3 = 14,      // DMA1 Stream 3
    DMA1_Stream4 = 15,      // DMA1 Stream 4
    DMA1_Stream5 = 16,      // DMA1 Stream 5
    DMA1_Stream6 = 17,      // DMA1 Stream 6
    DMA1_Stream7 = 47,      // DMA1 Stream 7

    DMA2_Stream0 = 56,      // DMA2 Stream 0
    DMA2_Stream1 = 57,      // DMA2 Stream 1
    DMA2_Stream2 = 58,      // DMA2 Stream 2
    DMA2_Stream3 = 59,      // DMA2 Stream 3
    DMA2_Stream4 = 60,      // DMA2 Stream 4
    DMA2_Stream5 = 68,      // DMA2 Stream 5
    DMA2_Stream6 = 69,      // DMA2 Stream 6
    DMA2_Stream7 = 70,      // DMA2 Stream 7

    // Timers
    TIM1_BRK_TIM9 = 24,     // TIM1 Break and TIM9
    TIM1_UP_TIM10 = 25,     // TIM1 Update and TIM10
    TIM1_TRG_COM_TIM11 = 26,// TIM1 Trigger and Commutation and TIM11
    TIM1_CC = 27,           // TIM1 Capture Compare
    TIM2 = 28,              // TIM2 global interrupt
    TIM3 = 29,              // TIM3 global interrupt
    TIM4 = 30,              // TIM4 global interrupt
    TIM5 = 50,              // TIM5 global interrupt

    // ADC
    ADC = 18,               // ADC1, ADC2 and ADC3 global interrupts

    // External interrupts
    EXTI0 = 6,              // EXTI Line0
    EXTI1 = 7,              // EXTI Line1
    EXTI2 = 8,              // EXTI Line2
    EXTI3 = 9,              // EXTI Line3
    EXTI4 = 10,             // EXTI Line4
    EXTI9_5 = 23,           // EXTI Line[9:5]
    EXTI15_10 = 40,         // EXTI Line[15:10]
};

/**
 * @brief IRQ information structure
 */
struct IrqInfo {
    const char* name;
    u16 irq_number;
    const char* description;
};

/**
 * @brief IRQ lookup table
 *
 * Allows runtime lookup of IRQ information by name.
 */
inline constexpr IrqInfo irq_table[] = {
    {"USART1", 37, "USART1 global interrupt"},
    {"USART2", 38, "USART2 global interrupt"},
    {"USART3", 39, "USART3 global interrupt"},
    {"UART4", 52, "UART4 global interrupt"},
    {"UART5", 53, "UART5 global interrupt"},
    {"USART6", 71, "USART6 global interrupt"},

    {"SPI1", 35, "SPI1 global interrupt"},
    {"SPI2", 36, "SPI2 global interrupt"},
    {"SPI3", 51, "SPI3 global interrupt"},
    {"SPI4", 84, "SPI4 global interrupt"},

    {"I2C1_EV", 31, "I2C1 event interrupt"},
    {"I2C1_ER", 32, "I2C1 error interrupt"},
    {"I2C2_EV", 33, "I2C2 event interrupt"},
    {"I2C2_ER", 34, "I2C2 error interrupt"},
    {"I2C3_EV", 72, "I2C3 event interrupt"},
    {"I2C3_ER", 73, "I2C3 error interrupt"},

    // ... more entries
};

/**
 * @brief Get IRQ information by name
 *
 * @param name Peripheral name (e.g., "USART1")
 * @return Pointer to IrqInfo or nullptr if not found
 */
constexpr const IrqInfo* get_irq_info(const char* name) {
    for (const auto& info : irq_table) {
        // Simple string comparison (constexpr)
        const char* a = name;
        const char* b = info.name;
        while (*a && *b && *a == *b) {
            ++a;
            ++b;
        }
        if (*a == *b) {  // Both reached null terminator
            return &info;
        }
    }
    return nullptr;
}

}  // namespace alloy::hal::st::stm32f4
```

#### SAME70 IRQ Table

```cpp
// hal/vendors/atmel/same70/irq_table.hpp (generated)
namespace alloy::hal::atmel::same70 {

enum class IrqNumber : u16 {
    // System exceptions
    Reset = 1,
    NMI = 2,
    HardFault = 3,
    MemManage = 4,
    BusFault = 5,
    UsageFault = 6,
    SVCall = 11,
    DebugMonitor = 12,
    PendSV = 14,
    SysTick = 15,

    // SAME70-specific peripheral IRQs
    SUPC = 0,               // Supply Controller
    RSTC = 1,               // Reset Controller
    RTC = 2,                // Real Time Clock
    RTT = 3,                // Real Time Timer
    WDT = 4,                // Watchdog Timer
    PMC = 5,                // Power Management Controller
    EFC = 6,                // Enhanced Flash Controller

    UART0 = 7,              // UART 0
    UART1 = 8,              // UART 1
    UART2 = 44,             // UART 2
    UART3 = 45,             // UART 3
    UART4 = 46,             // UART 4

    PIOA = 10,              // Parallel I/O Controller A
    PIOB = 11,              // Parallel I/O Controller B
    PIOC = 12,              // Parallel I/O Controller C
    PIOD = 16,              // Parallel I/O Controller D
    PIOE = 17,              // Parallel I/O Controller E

    USART0 = 13,            // USART 0
    USART1 = 14,            // USART 1
    USART2 = 15,            // USART 2

    SPI0 = 21,              // Serial Peripheral Interface 0
    SPI1 = 22,              // Serial Peripheral Interface 1

    TWI0 = 19,              // Two Wire Interface 0 (I2C)
    TWI1 = 20,              // Two Wire Interface 1 (I2C)
    TWI2 = 41,              // Two Wire Interface 2 (I2C)

    HSMCI = 18,             // High Speed Multimedia Card Interface

    TC0 = 23,               // Timer/Counter 0
    TC1 = 24,               // Timer/Counter 1
    TC2 = 25,               // Timer/Counter 2
    TC3 = 26,               // Timer/Counter 3

    AFEC0 = 29,             // Analog Front End 0 (ADC)
    AFEC1 = 40,             // Analog Front End 1 (ADC)

    DACC = 30,              // Digital To Analog Converter

    PWM0 = 31,              // Pulse Width Modulation 0
    PWM1 = 60,              // Pulse Width Modulation 1

    XDMAC = 48,             // DMA Controller
};

inline constexpr IrqInfo irq_table[] = {
    {"UART0", 7, "UART 0"},
    {"UART1", 8, "UART 1"},
    {"UART2", 44, "UART 2"},
    {"UART3", 45, "UART 3"},
    {"UART4", 46, "UART 4"},

    {"USART0", 13, "USART 0"},
    {"USART1", 14, "USART 1"},
    {"USART2", 15, "USART 2"},

    {"SPI0", 21, "Serial Peripheral Interface 0"},
    {"SPI1", 22, "Serial Peripheral Interface 1"},

    {"TWI0", 19, "Two Wire Interface 0"},
    {"TWI1", 20, "Two Wire Interface 1"},
    {"TWI2", 41, "Two Wire Interface 2"},

    // ... more entries
};

}  // namespace alloy::hal::atmel::same70
```

**Success Criteria**:
- ✅ IRQ tables generated for all supported platforms
- ✅ All peripheral IRQs included
- ✅ IRQ numbers match SVD exactly
- ✅ Compile-time and runtime lookup supported

---

### Requirement: Interrupt Controller Hardware Policy

Each platform SHALL have an interrupt controller hardware policy.

**Rationale**: Interrupt controllers vary (NVIC on ARM, PLIC on RISC-V). Need platform-specific implementation.

#### ARM Cortex-M NVIC Policy

```cpp
// hal/vendors/arm/cortex_m/nvic_hardware_policy.hpp
/**
 * @file nvic_hardware_policy.hpp
 * @brief NVIC (Nested Vectored Interrupt Controller) hardware policy
 *
 * Implements interrupt controller interface for ARM Cortex-M processors.
 */

#pragma once

#include "core/types.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal::arm::cortex_m {

using namespace alloy::core;

/**
 * @brief NVIC hardware policy
 *
 * Provides access to ARM Cortex-M NVIC registers.
 */
struct NvicHardwarePolicy {
    // NVIC register base addresses
    static constexpr uint32_t NVIC_BASE = 0xE000E100;
    static constexpr uint32_t SCB_BASE = 0xE000ED00;

    // ========================================================================
    // Global Interrupt Control
    // ========================================================================

    /**
     * @brief Enable global interrupts
     */
    static inline void enable_global() {
        #if defined(__ARM_ARCH_6M__)  // Cortex-M0/M0+
            __asm volatile("cpsie i" ::: "memory");
        #elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)  // Cortex-M3/M4/M7
            __asm volatile("cpsie i" ::: "memory");
        #else
            #error "Unsupported ARM architecture"
        #endif
    }

    /**
     * @brief Disable global interrupts
     */
    static inline void disable_global() {
        #if defined(__ARM_ARCH_6M__)
            __asm volatile("cpsid i" ::: "memory");
        #elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
            __asm volatile("cpsid i" ::: "memory");
        #else
            #error "Unsupported ARM architecture"
        #endif
    }

    /**
     * @brief Save and disable interrupts
     *
     * @return Saved PRIMASK value
     */
    static inline uint32_t save_and_disable() {
        uint32_t primask;
        __asm volatile("mrs %0, primask" : "=r"(primask));
        __asm volatile("cpsid i" ::: "memory");
        return primask;
    }

    /**
     * @brief Restore interrupts
     *
     * @param state Saved PRIMASK value
     */
    static inline void restore(uint32_t state) {
        __asm volatile("msr primask, %0" :: "r"(state) : "memory");
    }

    // ========================================================================
    // Specific Interrupt Control
    // ========================================================================

    /**
     * @brief Enable specific interrupt
     *
     * @param irq IRQ number (0-239)
     * @return Result with error code
     */
    static inline Result<void, ErrorCode> enable(uint16_t irq) {
        if (irq >= 240) {
            return Err(ErrorCode::InvalidParameter);
        }

        volatile uint32_t* NVIC_ISER = reinterpret_cast<volatile uint32_t*>(NVIC_BASE + 0x000);
        NVIC_ISER[irq >> 5] = (1UL << (irq & 0x1F));

        return Ok();
    }

    /**
     * @brief Disable specific interrupt
     *
     * @param irq IRQ number (0-239)
     * @return Result with error code
     */
    static inline Result<void, ErrorCode> disable(uint16_t irq) {
        if (irq >= 240) {
            return Err(ErrorCode::InvalidParameter);
        }

        volatile uint32_t* NVIC_ICER = reinterpret_cast<volatile uint32_t*>(NVIC_BASE + 0x080);
        NVIC_ICER[irq >> 5] = (1UL << (irq & 0x1F));

        return Ok();
    }

    /**
     * @brief Check if interrupt is enabled
     *
     * @param irq IRQ number
     * @return true if enabled
     */
    static inline bool is_enabled(uint16_t irq) {
        if (irq >= 240) {
            return false;
        }

        volatile uint32_t* NVIC_ISER = reinterpret_cast<volatile uint32_t*>(NVIC_BASE + 0x000);
        return (NVIC_ISER[irq >> 5] & (1UL << (irq & 0x1F))) != 0;
    }

    /**
     * @brief Set interrupt priority
     *
     * @param irq IRQ number
     * @param priority Priority (0-255, lower = higher priority)
     * @return Result with error code
     */
    static inline Result<void, ErrorCode> set_priority(uint16_t irq, uint8_t priority) {
        if (irq >= 240) {
            return Err(ErrorCode::InvalidParameter);
        }

        volatile uint8_t* NVIC_IPR = reinterpret_cast<volatile uint8_t*>(NVIC_BASE + 0x300);
        NVIC_IPR[irq] = priority;

        return Ok();
    }

    /**
     * @brief Get interrupt priority
     *
     * @param irq IRQ number
     * @return Result with priority or error
     */
    static inline Result<uint8_t, ErrorCode> get_priority(uint16_t irq) {
        if (irq >= 240) {
            return Err(ErrorCode::InvalidParameter);
        }

        volatile uint8_t* NVIC_IPR = reinterpret_cast<volatile uint8_t*>(NVIC_BASE + 0x300);
        return Ok(NVIC_IPR[irq]);
    }

    /**
     * @brief Check if interrupt is pending
     *
     * @param irq IRQ number
     * @return true if pending
     */
    static inline bool is_pending(uint16_t irq) {
        if (irq >= 240) {
            return false;
        }

        volatile uint32_t* NVIC_ISPR = reinterpret_cast<volatile uint32_t*>(NVIC_BASE + 0x100);
        return (NVIC_ISPR[irq >> 5] & (1UL << (irq & 0x1F))) != 0;
    }

    /**
     * @brief Set interrupt pending
     *
     * @param irq IRQ number
     * @return Result with error code
     */
    static inline Result<void, ErrorCode> set_pending(uint16_t irq) {
        if (irq >= 240) {
            return Err(ErrorCode::InvalidParameter);
        }

        volatile uint32_t* NVIC_ISPR = reinterpret_cast<volatile uint32_t*>(NVIC_BASE + 0x100);
        NVIC_ISPR[irq >> 5] = (1UL << (irq & 0x1F));

        return Ok();
    }

    /**
     * @brief Clear interrupt pending
     *
     * @param irq IRQ number
     * @return Result with error code
     */
    static inline Result<void, ErrorCode> clear_pending(uint16_t irq) {
        if (irq >= 240) {
            return Err(ErrorCode::InvalidParameter);
        }

        volatile uint32_t* NVIC_ICPR = reinterpret_cast<volatile uint32_t*>(NVIC_BASE + 0x180);
        NVIC_ICPR[irq >> 5] = (1UL << (irq & 0x1F));

        return Ok();
    }

    /**
     * @brief Check if interrupt is active (currently executing)
     *
     * @param irq IRQ number
     * @return true if active
     */
    static inline bool is_active(uint16_t irq) {
        if (irq >= 240) {
            return false;
        }

        volatile uint32_t* NVIC_IABR = reinterpret_cast<volatile uint32_t*>(NVIC_BASE + 0x200);
        return (NVIC_IABR[irq >> 5] & (1UL << (irq & 0x1F))) != 0;
    }

    /**
     * @brief Set priority grouping
     *
     * @param grouping Priority grouping (0-7)
     * @return Result with error code
     */
    static inline Result<void, ErrorCode> set_priority_grouping(uint8_t grouping) {
        if (grouping > 7) {
            return Err(ErrorCode::InvalidParameter);
        }

        volatile uint32_t* SCB_AIRCR = reinterpret_cast<volatile uint32_t*>(SCB_BASE + 0x0C);
        uint32_t reg = *SCB_AIRCR;
        reg = (reg & ~(0x7UL << 8)) | ((grouping & 0x7UL) << 8);
        reg = (reg & 0x0000FFFF) | 0x05FA0000;  // VECTKEY
        *SCB_AIRCR = reg;

        return Ok();
    }

    /**
     * @brief System reset
     *
     * Triggers a system reset via NVIC.
     */
    [[noreturn]] static inline void system_reset() {
        volatile uint32_t* SCB_AIRCR = reinterpret_cast<volatile uint32_t*>(SCB_BASE + 0x0C);
        *SCB_AIRCR = 0x05FA0004;  // VECTKEY | SYSRESETREQ
        while (true) {
            __asm volatile("nop");
        }
    }
};

}  // namespace alloy::hal::arm::cortex_m
```

**Success Criteria**:
- ✅ All NVIC operations implemented
- ✅ Inline assembly for critical operations
- ✅ Register addresses correct for Cortex-M
- ✅ Works on Cortex-M0/M3/M4/M7

---

### Requirement: Link Peripherals to IRQs

Peripheral hardware policies SHALL expose their IRQ number.

**Rationale**: Enables users to enable/disable interrupts for specific peripherals easily.

#### Example: UART Policy with IRQ

```cpp
// hal/vendors/atmel/same70/uart_hardware_policy.hpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ, uint16_t IRQ_NUM>
struct Same70UartHardwarePolicy {
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock = PERIPH_CLOCK_HZ;
    static constexpr uint16_t irq_number = IRQ_NUM;  // ← NEW!

    // ... all existing methods ...
};

// Platform instances
using Uart0Hardware = Same70UartHardwarePolicy<0x400E0800, 150000000, 7>;   // IRQ 7
using Uart1Hardware = Same70UartHardwarePolicy<0x400E0A00, 150000000, 8>;   // IRQ 8
using Uart2Hardware = Same70UartHardwarePolicy<0x400E1A00, 150000000, 44>;  // IRQ 44
```

#### User Code

```cpp
// Enable UART0 interrupts
using namespace alloy::platform::same70;

// Get IRQ number from peripheral
constexpr auto uart0_irq = Uart0Hardware::irq_number;  // 7

// Enable interrupt
Interrupt::enable(static_cast<IrqNumber>(uart0_irq));
Interrupt::set_priority(static_cast<IrqNumber>(uart0_irq), IrqPriority::High);
```

**Success Criteria**:
- ✅ All peripheral policies have `irq_number` constant
- ✅ IRQ numbers match generated IRQ tables
- ✅ User can enable peripheral interrupts easily

---

### Requirement: Update Generic Interrupt APIs

Generic interrupt APIs SHALL use platform-specific interrupt controller policies.

#### Updated Simple API

```cpp
// hal/interrupt_simple.hpp
#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"

// Platform-specific includes
#if defined(ALLOY_PLATFORM_SAME70) || defined(ALLOY_PLATFORM_STM32F4)
    #include "hal/vendors/arm/cortex_m/nvic_hardware_policy.hpp"
    using InterruptControllerPolicy = alloy::hal::arm::cortex_m::NvicHardwarePolicy;
#elif defined(ALLOY_PLATFORM_LINUX)
    // Host implementation (signals, etc.)
    #include "hal/vendors/host/interrupt_policy.hpp"
    using InterruptControllerPolicy = alloy::hal::host::InterruptPolicy;
#else
    #error "No interrupt controller policy defined for this platform"
#endif

namespace alloy::hal {

class Interrupt {
public:
    static void enable_all() noexcept {
        InterruptControllerPolicy::enable_global();
    }

    static void disable_all() noexcept {
        InterruptControllerPolicy::disable_global();
    }

    static Result<void, ErrorCode> enable(IrqNumber irq) noexcept {
        return InterruptControllerPolicy::enable(static_cast<uint16_t>(irq));
    }

    static Result<void, ErrorCode> disable(IrqNumber irq) noexcept {
        return InterruptControllerPolicy::disable(static_cast<uint16_t>(irq));
    }

    static bool is_enabled(IrqNumber irq) noexcept {
        return InterruptControllerPolicy::is_enabled(static_cast<uint16_t>(irq));
    }

    static Result<void, ErrorCode> set_priority(
        IrqNumber irq,
        IrqPriority priority) noexcept {
        return InterruptControllerPolicy::set_priority(
            static_cast<uint16_t>(irq),
            static_cast<uint8_t>(priority)
        );
    }
};

class CriticalSection {
public:
    CriticalSection() noexcept {
        saved_state_ = InterruptControllerPolicy::save_and_disable();
    }

    ~CriticalSection() noexcept {
        InterruptControllerPolicy::restore(saved_state_);
    }

    CriticalSection(const CriticalSection&) = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;

private:
    uint32_t saved_state_;
};

}  // namespace alloy::hal
```

**Success Criteria**:
- ✅ Generic APIs work with any interrupt controller policy
- ✅ No platform-specific code in generic API
- ✅ CriticalSection uses policy for save/restore

---

## Code Generation

### JSON Metadata for IRQ Tables

```json
// tools/codegen/metadata/platform/same70_irq_table.json
{
  "family": "same70",
  "vendor": "atmel",
  "svd_file": "ATSAME70Q21.svd",

  "interrupt_controller": {
    "type": "NVIC",
    "policy_include": "hal/vendors/arm/cortex_m/nvic_hardware_policy.hpp"
  },

  "irq_mappings": [
    {"name": "UART0", "irq": 7, "description": "UART 0"},
    {"name": "UART1", "irq": 8, "description": "UART 1"},
    {"name": "UART2", "irq": 44, "description": "UART 2"},
    {"name": "USART0", "irq": 13, "description": "USART 0"},
    {"name": "SPI0", "irq": 21, "description": "Serial Peripheral Interface 0"},
    {"name": "SPI1", "irq": 22, "description": "Serial Peripheral Interface 1"},
    {"name": "TWI0", "irq": 19, "description": "Two Wire Interface 0"},
    {"name": "XDMAC", "irq": 48, "description": "DMA Controller"}
  ]
}
```

### Generator Script

```python
# tools/codegen/generators/interrupt_table_generator.py
#!/usr/bin/env python3
"""
Interrupt Table Generator

Generates IRQ tables from SVD files.
"""

import json
import xml.etree.ElementTree as ET
from pathlib import Path
from jinja2 import Environment, FileSystemLoader

class InterruptTableGenerator:
    def __init__(self, svd_dir: Path, template_dir: Path, output_dir: Path):
        self.svd_dir = svd_dir
        self.env = Environment(loader=FileSystemLoader(template_dir))
        self.output_dir = output_dir

    def parse_svd_interrupts(self, svd_file: Path) -> list:
        """Parse IRQ numbers from SVD file."""
        tree = ET.parse(svd_file)
        root = tree.getroot()

        interrupts = []
        for peripheral in root.findall('.//peripheral'):
            name = peripheral.find('name').text
            interrupt_tags = peripheral.findall('.//interrupt')

            for interrupt in interrupt_tags:
                irq_name = interrupt.find('name').text
                irq_value = int(interrupt.find('value').text)
                description = interrupt.find('description')
                desc_text = description.text if description is not None else ""

                interrupts.append({
                    'name': irq_name,
                    'irq': irq_value,
                    'description': desc_text
                })

        # Remove duplicates
        seen = set()
        unique_interrupts = []
        for irq in interrupts:
            key = (irq['name'], irq['irq'])
            if key not in seen:
                seen.add(key)
                unique_interrupts.append(irq)

        # Sort by IRQ number
        unique_interrupts.sort(key=lambda x: x['irq'])

        return unique_interrupts

    def generate_irq_table(self, metadata_file: Path):
        """Generate IRQ table from metadata."""
        with open(metadata_file) as f:
            metadata = json.load(f)

        # Parse SVD file
        svd_path = self.svd_dir / metadata['svd_file']
        if svd_path.exists():
            irq_mappings = self.parse_svd_interrupts(svd_path)
        else:
            # Use manual mappings from JSON
            irq_mappings = metadata.get('irq_mappings', [])

        metadata['irq_mappings'] = irq_mappings

        # Load template
        template = self.env.get_template('irq_table.hpp.j2')

        # Render
        output = template.render(**metadata)

        # Write output
        vendor = metadata['vendor']
        family = metadata['family']
        output_file = self.output_dir / vendor / family / 'irq_table.hpp'

        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_text(output)

        print(f"Generated: {output_file}")

def main():
    svd_dir = Path("tools/codegen/svd")
    template_dir = Path("tools/codegen/templates")
    metadata_dir = Path("tools/codegen/metadata/platform")
    output_dir = Path("src/hal/vendors")

    generator = InterruptTableGenerator(svd_dir, template_dir, output_dir)

    # Generate IRQ tables for all platforms
    for metadata_file in metadata_dir.glob("*_irq_table.json"):
        print(f"Generating IRQ table for {metadata_file.stem}...")
        generator.generate_irq_table(metadata_file)

if __name__ == "__main__":
    main()
```

---

## Testing Strategy

### Unit Tests

```cpp
// tests/unit/test_interrupt_policy.cpp
#include "catch2/catch_test_macros.hpp"
#include "hal/vendors/arm/cortex_m/nvic_hardware_policy.hpp"

using namespace alloy::hal::arm::cortex_m;

TEST_CASE("NVIC Policy - Enable IRQ", "[interrupt][nvic]") {
    // This test would need hardware or mock NVIC registers
    auto result = NvicHardwarePolicy::enable(37);  // USART1
    REQUIRE(result.is_ok());
}

TEST_CASE("NVIC Policy - Invalid IRQ", "[interrupt][nvic]") {
    auto result = NvicHardwarePolicy::enable(300);  // Out of range
    REQUIRE(result.is_error());
    REQUIRE(result.error() == ErrorCode::InvalidParameter);
}
```

### Integration Tests

```cpp
// tests/integration/test_uart_interrupts.cpp
TEST_CASE("UART with Interrupts", "[uart][interrupt]") {
    using namespace alloy::platform::same70;

    // Setup UART
    auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
    uart.initialize();

    // Enable UART interrupt
    constexpr auto irq = Uart0Hardware::irq_number;
    Interrupt::enable(static_cast<IrqNumber>(irq));
    Interrupt::set_priority(static_cast<IrqNumber>(irq), IrqPriority::High);

    // Verify enabled
    REQUIRE(Interrupt::is_enabled(static_cast<IrqNumber>(irq)));
}
```

---

## File Organization

```
src/hal/
├── interface/
│   └── interrupt.hpp          (Concepts, no hardcoded IRQs)
│
├── api/
│   ├── interrupt_simple.hpp   (Uses interrupt controller policy)
│   └── interrupt_expert.hpp   (Uses interrupt controller policy)
│
└── vendors/
    ├── arm/
    │   └── cortex_m/
    │       └── nvic_hardware_policy.hpp
    │
    ├── atmel/same70/
    │   ├── irq_table.hpp             (Generated IRQ mappings)
    │   ├── uart_hardware_policy.hpp  (Includes IRQ number)
    │   └── spi_hardware_policy.hpp   (Includes IRQ number)
    │
    └── st/stm32f4/
        ├── irq_table.hpp             (Generated IRQ mappings)
        ├── uart_hardware_policy.hpp  (Includes IRQ number)
        └── spi_hardware_policy.hpp   (Includes IRQ number)
```

---

## Success Criteria

- [ ] IRQ tables generated from SVD for all platforms
- [ ] Interrupt controller policy implemented (NVIC)
- [ ] All peripheral policies include IRQ number
- [ ] Generic interrupt APIs use policies
- [ ] CriticalSection uses policy save/restore
- [ ] Unit tests for interrupt controller policy
- [ ] Integration tests with peripherals
- [ ] Documentation complete

---

## Timeline

| Task | Duration | Notes |
|------|----------|-------|
| Parse SVD interrupts | 2 days | Extend SVD parser |
| Generate IRQ tables | 2 days | Create generator + templates |
| Implement NVIC policy | 3 days | ARM Cortex-M assembly |
| Update peripheral policies | 2 days | Add IRQ numbers |
| Update generic APIs | 1 day | Use policies |
| Unit tests | 2 days | Mock NVIC registers |
| Integration tests | 2 days | Test with UART/SPI |
| Documentation | 1 day | Examples + guide |

**Total: 15 days (~3 weeks)**

---

## Dependencies

- Hardware policy infrastructure (Phase 8)
- SVD parser enhancements
- Platform-specific metadata (JSON files)

---

## Open Questions

1. **Q**: How to handle shared IRQs (e.g., I2C event and error on same peripheral)?
   **A**: Use separate enum values (I2C1_EV, I2C1_ER) and document relationship.

2. **Q**: Should we generate interrupt handler stubs?
   **A**: Not in this phase. Focus on control (enable/disable). Handlers in future phase.

3. **Q**: How to handle platforms without NVIC (RISC-V PLIC, x86 APIC)?
   **A**: Create separate policies (PlicHardwarePolicy, ApicHardwarePolicy) with same interface.

4. **Q**: Should IRQ numbers be strongly typed per platform?
   **A**: Yes, use platform-specific `IrqNumber` enum to prevent cross-platform bugs.
