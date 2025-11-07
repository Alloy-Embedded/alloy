#pragma once

#include <cstdint>

namespace alloy::hal::stm32f103::stm32f103rc::hardware {

// ============================================================================
// Hardware Register Addresses for STM32F103RC
// Auto-generated from SVD: STM32F103
// ============================================================================

// GPIO Registers structure (STM32F1/F0 architecture)
struct GPIO_Registers {
    volatile uint32_t CRL;      // Port configuration register low (pins 0-7)
    volatile uint32_t CRH;      // Port configuration register high (pins 8-15)
    volatile uint32_t IDR;      // Input data register
    volatile uint32_t ODR;      // Output data register
    volatile uint32_t BSRR;     // Bit set/reset register
    volatile uint32_t BRR;      // Bit reset register
    volatile uint32_t LCKR;     // Port configuration lock register
};

// RCC Registers structure (STM32F1 architecture)
struct RCC_Registers {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;      // APB2 peripheral clock enable register
    volatile uint32_t APB1ENR;
};

// Base addresses from SVD
constexpr uintptr_t GPIOA_BASE = 0x40010800;
constexpr uintptr_t GPIOB_BASE = 0x40010C00;
constexpr uintptr_t GPIOC_BASE = 0x40011000;
constexpr uintptr_t GPIOD_BASE = 0x40011400;
constexpr uintptr_t GPIOE_BASE = 0x40011800;
constexpr uintptr_t GPIOF_BASE = 0x40011C00;
constexpr uintptr_t GPIOG_BASE = 0x40012000;

constexpr uintptr_t RCC_BASE = 0x40021000;

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



}  // namespace alloy::hal::stm32f103::stm32f103rc::hardware

// Hardware traits struct for template usage
namespace alloy::hal::stm32f103::stm32f103rc {

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

}  // namespace alloy::hal::stm32f103::stm32f103rc
