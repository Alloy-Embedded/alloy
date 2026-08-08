/* Vendor HAL body — plain C, compiled by the same alloy build as the C++.
 *
 * Nothing here knows alloy exists. That is the point: legacy C keeps its own
 * idiom, its own header, and its own file, and the build compiles it because
 * `alloy build` puts every .c under src/ on the compiler's command line
 * (build.py: build_inputs globs the project's C sources as well as its C++ ones).
 */
#include "vendor_hal.h"

void VENDOR_GPIO_WritePin(GPIO_TypeDef* port, uint32_t pin, VENDOR_PinState state) {
    port->BSRR = (state == VENDOR_PIN_SET) ? (1UL << pin) : (1UL << (pin + 16U));
}

void VENDOR_GPIO_TogglePin(GPIO_TypeDef* port, uint32_t pin) {
    const uint32_t odr = port->ODR;
    port->BSRR = (odr & (1UL << pin)) ? (1UL << (pin + 16U)) : (1UL << pin);
}

uint32_t VENDOR_GPIO_ReadPin(const GPIO_TypeDef* port, uint32_t pin) {
    return (port->IDR >> pin) & 1UL;
}
