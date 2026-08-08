/* A stand-in for a vendor CMSIS device header (stm32g0xx.h and friends).
 *
 * NOT ST's file — that one is not redistributable here, and copying it would
 * make this example a licence problem instead of a demonstration. What matters
 * for the question "does a vendor header collide with alloy's?" is the SHAPE,
 * and this reproduces it exactly:
 *
 *   - peripheral instances as UPPERCASE object-like macros over a cast pointer
 *   - register blocks as `<NAME>_TypeDef` structs at global scope
 *   - `__IO` / `__I` / `__O` volatile macros
 *   - `<BLOCK>_<REG>_<FIELD>_Pos` / `_Msk` bit macros
 *   - an `IRQn_Type` enum with the vendor's spelling of every line
 *
 * If your project has the real header, use it: every claim on
 * docs/guide/escape-hatch.md is about those five shapes, not about this file.
 */
#ifndef VENDOR_DEVICE_H
#define VENDOR_DEVICE_H

#include <stdint.h>

#define __IO volatile
#define __I  volatile const
#define __O  volatile

typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __I  uint32_t IDR;
    __IO uint32_t ODR;
    __O  uint32_t BSRR;
    __IO uint32_t LCKR;
    __IO uint32_t AFR[2];
    __O  uint32_t BRR;
} GPIO_TypeDef;

typedef struct {
    __IO uint32_t CR;
    __IO uint32_t ICSCR;
    __IO uint32_t CFGR;
} RCC_TypeDef;

#define PERIPH_BASE       (0x40000000UL)
#define IOPORT_BASE       (0x50000000UL)
#define GPIOA_BASE        (IOPORT_BASE + 0x0000UL)
#define GPIOB_BASE        (IOPORT_BASE + 0x0400UL)
#define RCC_BASE          (PERIPH_BASE + 0x00021000UL)

#define GPIOA             ((GPIO_TypeDef*)GPIOA_BASE)
#define GPIOB             ((GPIO_TypeDef*)GPIOB_BASE)
#define RCC               ((RCC_TypeDef*)RCC_BASE)

#define GPIO_MODER_MODE5_Pos   (10U)
#define GPIO_MODER_MODE5_Msk   (0x3UL << GPIO_MODER_MODE5_Pos)
#define GPIO_BSRR_BS5_Pos      (5U)
#define GPIO_BSRR_BS5          (0x1UL << GPIO_BSRR_BS5_Pos)
#define GPIO_BSRR_BR5_Pos      (21U)
#define GPIO_BSRR_BR5          (0x1UL << GPIO_BSRR_BR5_Pos)

typedef enum {
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn      = -13,
    SysTick_IRQn        = -1,
    TIM2_IRQn           = 15,
    USART2_IRQn         = 28,
} IRQn_Type;

#endif /* VENDOR_DEVICE_H */
