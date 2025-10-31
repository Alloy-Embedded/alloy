#include "gpio.hpp"

namespace alloy::hal::stm32f1 {

template<uint8_t PIN>
void GpioPin<PIN>::enable_port_clock() {
    // Enable GPIO port clock in RCC_APB2ENR
    auto* rcc = get_rcc_registers();
    const uint32_t port_bit = get_port_enable_bit(port);
    rcc->APB2ENR |= (1U << port_bit);
}

template<uint8_t PIN>
GpioPort* GpioPin<PIN>::get_port_registers() {
    return reinterpret_cast<GpioPort*>(get_port_address(port));
}

template<uint8_t PIN>
RCC* GpioPin<PIN>::get_rcc_registers() {
    return reinterpret_cast<RCC*>(gpio_ports::RCC_BASE);
}

template<uint8_t PIN>
void GpioPin<PIN>::configure(PinMode mode) {
    auto* gpio = get_port_registers();

    // STM32F1 uses 4 bits per pin for configuration
    // Bits [1:0] = MODE (00=Input, 01=Output 10MHz, 10=Output 2MHz, 11=Output 50MHz)
    // Bits [3:2] = CNF (configuration depends on MODE)

    uint32_t config = 0;

    switch (mode) {
        case PinMode::Input:
            // MODE=00 (Input), CNF=01 (Floating input)
            config = 0b0100;
            break;

        case PinMode::InputPullUp:
        case PinMode::InputPullDown:
            // MODE=00 (Input), CNF=10 (Input with pull-up/pull-down)
            config = 0b1000;
            break;

        case PinMode::Output:
            // MODE=11 (Output 50MHz), CNF=00 (General purpose output push-pull)
            config = 0b0011;
            break;

        case PinMode::Alternate:
            // MODE=11 (Output 50MHz), CNF=10 (Alternate function output push-pull)
            config = 0b1011;
            break;

        case PinMode::Analog:
            // MODE=00 (Input), CNF=00 (Analog mode)
            config = 0b0000;
            break;
    }

    // Determine which register to use (CRL for pins 0-7, CRH for pins 8-15)
    if (pin_bit < 8) {
        // Use CRL (pins 0-7)
        const uint32_t bit_offset = pin_bit * 4;
        const uint32_t mask = ~(0xFU << bit_offset);
        gpio->CRL = (gpio->CRL & mask) | (config << bit_offset);
    } else {
        // Use CRH (pins 8-15)
        const uint32_t bit_offset = (pin_bit - 8) * 4;
        const uint32_t mask = ~(0xFU << bit_offset);
        gpio->CRH = (gpio->CRH & mask) | (config << bit_offset);
    }

    // For pull-up/pull-down modes, configure ODR
    if (mode == PinMode::InputPullUp) {
        // Set ODR bit to 1 for pull-up
        gpio->ODR |= (1U << pin_bit);
    } else if (mode == PinMode::InputPullDown) {
        // Clear ODR bit to 0 for pull-down
        gpio->ODR &= ~(1U << pin_bit);
    }
}

template<uint8_t PIN>
void GpioPin<PIN>::set_high() {
    auto* gpio = get_port_registers();
    // Use BSRR (Bit Set Register) - atomic operation
    // Lower 16 bits set pins, upper 16 bits reset pins
    gpio->BSRR = (1U << pin_bit);
}

template<uint8_t PIN>
void GpioPin<PIN>::set_low() {
    auto* gpio = get_port_registers();
    // Use BRR (Bit Reset Register) - atomic operation
    // Or use upper 16 bits of BSRR
    gpio->BRR = (1U << pin_bit);
}

template<uint8_t PIN>
void GpioPin<PIN>::toggle() {
    auto* gpio = get_port_registers();
    // Read current state and toggle using XOR on ODR
    gpio->ODR ^= (1U << pin_bit);
}

template<uint8_t PIN>
bool GpioPin<PIN>::read() const {
    auto* gpio = get_port_registers();
    // Read from IDR (Input Data Register)
    return (gpio->IDR & (1U << pin_bit)) != 0;
}

// Explicit template instantiations for common pins
// This allows the linker to find the implementations

// Port A pins
template class GpioPin<0>;   // PA0
template class GpioPin<1>;   // PA1
template class GpioPin<2>;   // PA2
template class GpioPin<3>;   // PA3
template class GpioPin<4>;   // PA4
template class GpioPin<5>;   // PA5
template class GpioPin<6>;   // PA6
template class GpioPin<7>;   // PA7
template class GpioPin<8>;   // PA8
template class GpioPin<9>;   // PA9
template class GpioPin<10>;  // PA10
template class GpioPin<11>;  // PA11
template class GpioPin<12>;  // PA12
template class GpioPin<13>;  // PA13
template class GpioPin<14>;  // PA14
template class GpioPin<15>;  // PA15

// Port B pins
template class GpioPin<16>;  // PB0
template class GpioPin<17>;  // PB1
template class GpioPin<18>;  // PB2
template class GpioPin<19>;  // PB3
template class GpioPin<20>;  // PB4
template class GpioPin<21>;  // PB5
template class GpioPin<22>;  // PB6
template class GpioPin<23>;  // PB7
template class GpioPin<24>;  // PB8
template class GpioPin<25>;  // PB9
template class GpioPin<26>;  // PB10
template class GpioPin<27>;  // PB11
template class GpioPin<28>;  // PB12
template class GpioPin<29>;  // PB13
template class GpioPin<30>;  // PB14
template class GpioPin<31>;  // PB15

// Port C pins (BluePill LED is on PC13)
template class GpioPin<32>;  // PC0
template class GpioPin<33>;  // PC1
template class GpioPin<34>;  // PC2
template class GpioPin<35>;  // PC3
template class GpioPin<36>;  // PC4
template class GpioPin<37>;  // PC5
template class GpioPin<38>;  // PC6
template class GpioPin<39>;  // PC7
template class GpioPin<40>;  // PC8
template class GpioPin<41>;  // PC9
template class GpioPin<42>;  // PC10
template class GpioPin<43>;  // PC11
template class GpioPin<44>;  // PC12
template class GpioPin<45>;  // PC13 (BluePill onboard LED)
template class GpioPin<46>;  // PC14
template class GpioPin<47>;  // PC15

} // namespace alloy::hal::stm32f1
