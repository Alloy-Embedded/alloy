/* A vendor-HAL-shaped C API: the thing a company already has 40 000 lines of.
 *
 * The signature is deliberately in the vendor idiom — a pointer to the
 * peripheral's TypeDef, a bit mask, and a pin state enum — so the example
 * shows the real question: how does C++ application code under alloy hand a
 * peripheral to a C function written against CMSIS types?
 */
#ifndef VENDOR_HAL_H
#define VENDOR_HAL_H

#include "vendor_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { VENDOR_PIN_RESET = 0, VENDOR_PIN_SET = 1 } VENDOR_PinState;

void VENDOR_GPIO_WritePin(GPIO_TypeDef* port, uint32_t pin, VENDOR_PinState state);
void VENDOR_GPIO_TogglePin(GPIO_TypeDef* port, uint32_t pin);
uint32_t VENDOR_GPIO_ReadPin(const GPIO_TypeDef* port, uint32_t pin);

#ifdef __cplusplus
}
#endif

#endif /* VENDOR_HAL_H */
