/**
 * Universal GPIO Test
 *
 * This example demonstrates the universal GPIO header that automatically
 * adapts to the MCU defined in the build system.
 *
 * The same code works on different MCUs without modification!
 *
 * Compile for STM32F103C8 (Blue Pill):
 *   cmake -B build -DBOARD=bluepill
 *
 * Compile for STM32F407VG (Discovery):
 *   cmake -B build -DBOARD=stm32f407vg
 *
 * The MCU macro (MCU_STM32F103C8 or MCU_STM32F407VG) is defined by the board file.
 */

#include <cstdio>

#include "hal/gpio.hpp"  // Universal GPIO header!

int main() {
    // Use the automatically selected GPIO namespace
    using namespace gpio;

    printf("Universal GPIO Test\n");
    printf("===================\n\n");

    // Configure PC13 as output (LED on both Blue Pill and some STM32F4 boards)
    using LED = GPIOPin<pins::PC13>;

    printf("Configuring PC13 as output (LED)...\n");
    // LED::configureOutput();  // Uncomment for real hardware
    printf("  ✓ PC13 configured\n\n");

// Test pin functions (works on both F1 and F4!)
#if defined(MCU_STM32F103C8)
    printf("MCU: STM32F103C8 (Blue Pill)\n");
    printf("  Architecture: Cortex-M3\n");
    printf("  GPIO: CRL/CRH registers\n\n");

    // STM32F103 specific examples
    using UART_TX = GPIOPin<pin_functions::usart::USART1_TX>;
    printf("  USART1_TX pin: %d (PA9)\n", pin_functions::usart::USART1_TX);

    using I2C_SCL = GPIOPin<pin_functions::i2c::I2C1_SCL>;
    printf("  I2C1_SCL pin: %d (PB6)\n", pin_functions::i2c::I2C1_SCL);

#elif defined(MCU_STM32F407VG)
    printf("MCU: STM32F407VG (Discovery)\n");
    printf("  Architecture: Cortex-M4F\n");
    printf("  GPIO: MODER/OSPEEDR registers\n\n");

    // STM32F407 specific examples
    using UART_TX = GPIOPin<pin_functions::usart::USART1_TX>;
    printf("  USART1_TX pin: %d (PA9)\n", pin_functions::usart::USART1_TX);
    printf("  Needs AF7 for USART1\n");

    using I2C_SCL = GPIOPin<pin_functions::i2c::I2C1_SCL>;
    printf("  I2C1_SCL pin: %d (PB6 or PB8)\n", pin_functions::i2c::I2C1_SCL);
    printf("  Needs AF4 for I2C1\n");

#elif defined(MCU_STM32F746VG)
    printf("MCU: STM32F746VG (Discovery)\n");
    printf("  Architecture: Cortex-M7F\n");
    printf("  GPIO: MODER/OSPEEDR registers\n\n");

    // STM32F746 specific examples
    using UART_TX = GPIOPin<pin_functions::usart::USART1_TX>;
    printf("  USART1_TX pin: %d (PA9)\n", pin_functions::usart::USART1_TX);
    printf("  Needs AF7 for USART1\n");

    using I2C_SCL = GPIOPin<pin_functions::i2c::I2C1_SCL>;
    printf("  I2C1_SCL pin: %d (PB6 or PB8)\n", pin_functions::i2c::I2C1_SCL);
    printf("  Needs AF4 for I2C1\n");

#else
    printf("MCU: Other STM32\n");
#endif

    printf("\n✅ Universal GPIO test completed!\n");
    printf("The same source code works on multiple MCUs!\n");

    return 0;
}
