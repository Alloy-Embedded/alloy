#pragma once

#include <cstdint>

namespace alloy::hal::stm32f7::stm32f722re::hardware {

// ============================================================================
// Hardware Register Addresses for STM32F722RE
// Auto-generated from SVD: STM32F7x2
// ============================================================================

// GPIO Registers structure (STM32F4/F2/F7/L4/G4/H7/U5 architecture)
struct GPIO_Registers {
    volatile uint32_t MODER;    // Mode register
    volatile uint32_t OTYPER;   // Output type register
    volatile uint32_t OSPEEDR;  // Output speed register
    volatile uint32_t PUPDR;    // Pull-up/pull-down register
    volatile uint32_t IDR;      // Input data register
    volatile uint32_t ODR;      // Output data register
    volatile uint32_t BSRR;     // Bit set/reset register
    volatile uint32_t LCKR;     // Port configuration lock register
    volatile uint32_t AFR[2];   // Alternate function registers (AFRL, AFRH)
};

// RCC Registers structure (STM32F4 architecture)
struct RCC_Registers {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    volatile uint32_t _reserved0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t _reserved1[2];
    volatile uint32_t AHB1ENR;     // AHB1 peripheral clock enable register
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    volatile uint32_t _reserved2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
};

// Base addresses from SVD
constexpr uintptr_t GPIOA_BASE = 0x40020000;
constexpr uintptr_t GPIOB_BASE = 0x40020400;
constexpr uintptr_t GPIOC_BASE = 0x40020800;
constexpr uintptr_t GPIOD_BASE = 0x40020C00;
constexpr uintptr_t GPIOE_BASE = 0x40021000;
constexpr uintptr_t GPIOF_BASE = 0x40021400;
constexpr uintptr_t GPIOG_BASE = 0x40021800;

constexpr uintptr_t RCC_BASE = 0x40023800;

// GPIO port instances
inline GPIO_Registers* const GPIOA = reinterpret_cast<GPIO_Registers*>(GPIOA_BASE);
inline GPIO_Registers* const GPIOB = reinterpret_cast<GPIO_Registers*>(GPIOB_BASE);
inline GPIO_Registers* const GPIOC = reinterpret_cast<GPIO_Registers*>(GPIOC_BASE);
inline GPIO_Registers* const GPIOD = reinterpret_cast<GPIO_Registers*>(GPIOD_BASE);
inline GPIO_Registers* const GPIOE = reinterpret_cast<GPIO_Registers*>(GPIOE_BASE);
inline GPIO_Registers* const GPIOF = reinterpret_cast<GPIO_Registers*>(GPIOF_BASE);
inline GPIO_Registers* const GPIOG = reinterpret_cast<GPIO_Registers*>(GPIOG_BASE);

// RCC instance
inline RCC_Registers* const RCC = reinterpret_cast<RCC_Registers*>(RCC_BASE);

// RCC APB2ENR bits for GPIO clocks
constexpr uint32_t RCC_APB2ENR_IOPAEN = (1U << 2);
constexpr uint32_t RCC_APB2ENR_IOPBEN = (1U << 3);
constexpr uint32_t RCC_APB2ENR_IOPCEN = (1U << 4);
constexpr uint32_t RCC_APB2ENR_IOPDEN = (1U << 5);
constexpr uint32_t RCC_APB2ENR_IOPEEN = (1U << 6);
constexpr uint32_t RCC_APB2ENR_AFIOEN = (1U << 0);



}  // namespace alloy::hal::stm32f7::stm32f722re::hardware

// Hardware traits struct for template usage
namespace alloy::hal::stm32f7::stm32f722re {

struct Hardware {
    using GPIO_Registers = hardware::GPIO_Registers;
    using RCC_Registers = hardware::RCC_Registers;

    static inline GPIO_Registers* const GPIOA = hardware::GPIOA;
    static inline GPIO_Registers* const GPIOB = hardware::GPIOB;
    static inline GPIO_Registers* const GPIOC = hardware::GPIOC;
    static inline GPIO_Registers* const GPIOD = hardware::GPIOD;
    static inline GPIO_Registers* const GPIOE = hardware::GPIOE;
    static inline GPIO_Registers* const GPIOF = hardware::GPIOF;
    static inline GPIO_Registers* const GPIOG = hardware::GPIOG;
    static inline RCC_Registers* const RCC = hardware::RCC;

    static constexpr uint32_t RCC_APB2ENR_AFIOEN = hardware::RCC_APB2ENR_AFIOEN;
};

}  // namespace alloy::hal::stm32f7::stm32f722re
