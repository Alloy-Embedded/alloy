/**
 * Pin Configuration Example for STM32F103
 *
 * Demonstrates how to configure pins for different functions:
 * - GPIO (digital input/output)
 * - ADC (analog input)
 * - PWM (timer output)
 * - UART (serial communication)
 * - I2C (communication bus)
 * - SPI (communication bus)
 */

// Single include - gets everything!
#include "../../src/hal/st/stm32f103/generated/stm32f103c8/gpio.hpp"

#include <cstdio>

// Use the convenience namespace alias
using namespace gpio;

// Example 1: Configure pin as GPIO output (LED)
void example_gpio_output() {
    printf("Example 1: GPIO Output (LED on PC13)\n");

    // PC13 is commonly used for the onboard LED on STM32F103 boards
    using LED = GPIOPin<pins::PC13>;

    // Note: Actual hardware access commented out for host compilation
    // LED::configureOutput();
    // LED::set();    // Turn LED on
    // LED::reset();  // Turn LED off
    // LED::toggle(); // Toggle LED

    printf("  ✓ PC13 configured as output\n");
    printf("    Pin value: %d, Port: %d, Pin num: %d\n", pins::PC13, LED::PORT, LED::PIN_NUM);
    printf("    Use LED::configureOutput(), set(), reset(), toggle()\n\n");
}

// Example 2: Configure pin as GPIO input (button)
void example_gpio_input() {
    printf("Example 2: GPIO Input (Button on PA0)\n");

    using Button = GPIOPin<pins::PA0>;

    // Button::configureInput(2);  // Pull-up/down mode
    // bool button_state = Button::read();

    printf("  ✓ PA0 configured as input\n");
    printf("    Pin value: %d, Port: %d, Pin num: %d\n", pins::PA0, Button::PORT, Button::PIN_NUM);
    printf("    Use Button::configureInput(config), read()\n\n");
}

// Example 3: Configure pin for ADC (analog input)
void example_adc() {
    printf("Example 3: ADC Input (Analog sensor on PA1)\n");

    // PA1 is ADC12_IN1 (from pin_functions.hpp)
    constexpr uint8_t adc_pin = pin_functions::adc::ADC12_IN1;
    using ADCInput = GPIOPin<adc_pin>;

    // ADCInput::configureAnalog();

    printf("  ✓ PA1 (ADC12_IN1) configured for analog input\n");
    printf("    Pin number: %d (references pins::PA1 = %d)\n", adc_pin, pins::PA1);
    printf("    Use ADCInput::configureAnalog()\n\n");
}

// Example 4: Configure pins for UART (serial communication)
void example_uart() {
    printf("Example 4: UART Configuration (USART1 on PA9/PA10)\n");

    // USART1_TX on PA9, USART1_RX on PA10 (from pin_functions.hpp)
    constexpr uint8_t uart_tx = pin_functions::usart::USART1_TX;
    constexpr uint8_t uart_rx = pin_functions::usart::USART1_RX;

    using UART_TX = GPIOPin<uart_tx>;
    using UART_RX = GPIOPin<uart_rx>;

    // UART_TX::configureAlternateFunction(false);  // TX = push-pull
    // UART_RX::configureInput(1);                  // RX = floating input

    printf("  ✓ PA9 (USART1_TX) pin %d configured as alternate function\n", uart_tx);
    printf("  ✓ PA10 (USART1_RX) pin %d configured as floating input\n\n", uart_rx);
}

// Example 5: Configure pins for I2C (communication bus)
void example_i2c() {
    printf("Example 5: I2C Configuration (I2C1 on PB6/PB7)\n");

    // I2C1_SCL on PB6, I2C1_SDA on PB7 (from pin_functions.hpp)
    constexpr uint8_t i2c_scl = pin_functions::i2c::I2C1_SCL;
    constexpr uint8_t i2c_sda = pin_functions::i2c::I2C1_SDA;

    // I2C requires open-drain configuration
    // I2C_SCL::configureAlternateFunction(true);   // Open-drain
    // I2C_SDA::configureAlternateFunction(true);   // Open-drain

    printf("  ✓ PB6 (I2C1_SCL) pin %d configured as alternate function (open-drain)\n", i2c_scl);
    printf("  ✓ PB7 (I2C1_SDA) pin %d configured as alternate function (open-drain)\n\n", i2c_sda);
}

// Example 6: Configure pins for SPI (communication bus)
void example_spi() {
    printf("Example 6: SPI Configuration (SPI1 on PA5/PA6/PA7)\n");

    // SPI1_SCK on PA5, SPI1_MISO on PA6, SPI1_MOSI on PA7 (from pin_functions.hpp)
    constexpr uint8_t spi_sck = pin_functions::spi::SPI1_SCK;
    constexpr uint8_t spi_miso = pin_functions::spi::SPI1_MISO;
    constexpr uint8_t spi_mosi = pin_functions::spi::SPI1_MOSI;

    // SPI_SCK::configureAlternateFunction(false);   // SCK = push-pull
    // SPI_MISO::configureInput(1);                  // MISO = floating input
    // SPI_MOSI::configureAlternateFunction(false);  // MOSI = push-pull

    printf("  ✓ PA5 (SPI1_SCK) pin %d configured as alternate function\n", spi_sck);
    printf("  ✓ PA6 (SPI1_MISO) pin %d configured as floating input\n", spi_miso);
    printf("  ✓ PA7 (SPI1_MOSI) pin %d configured as alternate function\n\n", spi_mosi);
}

// Example 7: Configure pin for PWM (timer output)
void example_pwm() {
    printf("Example 7: PWM Configuration (TIM2_CH1 on PA0)\n");

    // TIM2_CH1_ETR on PA0 (from pin_functions.hpp)
    constexpr uint8_t pwm_pin = pin_functions::tim::TIM2_CH1_ETR;

    // PWM::configureAlternateFunction(false);  // Push-pull for PWM output

    printf("  ✓ PA0 (TIM2_CH1) pin %d configured for PWM output\n\n", pwm_pin);
}

// Example 8: Demonstrate pin remapping (CAN on different pins)
void example_remap() {
    printf("Example 8: Pin Remapping (CAN on different locations)\n");

    // CAN can be on different pins depending on remap setting
    printf("  CAN_RX default location: pin %d (PA11)\n", pin_functions::can::CAN_RX);
    printf("  CAN_RX remap1 location:  pin %d (PB8)\n", pin_functions::can::CAN_RX_REMAP1);
    printf("  CAN_RX remap2 location:  pin %d (PD0)\n\n", pin_functions::can::CAN_RX_REMAP2);
}

// Example 9: Compile-time validation
void example_compile_time_validation() {
    printf("Example 9: Compile-Time Pin Validation\n");

    // This compiles because PA0 is valid
    static_assert(pins::is_valid_pin_v<pins::PA0>, "PA0 should be valid");

    // This compiles because PC13 is valid
    static_assert(pins::is_valid_pin_v<pins::PC13>, "PC13 should be valid");

    // Uncomment to see compile error for invalid pin:
    // static_assert(pins::is_valid_pin_v<255>, "Invalid pin!");

    printf("  ✓ Compile-time validation ensures only valid pins are used\n\n");
}

// Example 10: MCU traits usage
void example_mcu_traits() {
    printf("Example 10: MCU Traits Information\n");

    using Traits = Traits;  // Already in gpio namespace

    printf("  Device: %s\n", Traits::DEVICE_NAME);
    printf("  Package: %s with %d GPIO pins\n", Traits::PACKAGE, Traits::PIN_COUNT);
    printf("  Flash: %d KB, RAM: %d KB\n",
           Traits::FLASH_SIZE / 1024, Traits::SRAM_SIZE / 1024);
    printf("  Peripherals:\n");
    printf("    - %d UART/USART\n", Traits::Peripherals::UART_COUNT);
    printf("    - %d I2C\n", Traits::Peripherals::I2C_COUNT);
    printf("    - %d SPI\n", Traits::Peripherals::SPI_COUNT);
    printf("    - %d ADC with %d channels\n",
           Traits::Peripherals::ADC_COUNT, Traits::Peripherals::ADC_CHANNELS);
    printf("    - USB: %s\n", Traits::Peripherals::HAS_USB ? "Yes" : "No");
    printf("    - CAN: %s\n\n", Traits::Peripherals::HAS_CAN ? "Yes" : "No");
}

int main() {
    printf("========================================\n");
    printf("STM32F103C8 Pin Configuration Examples\n");
    printf("========================================\n\n");

    example_gpio_output();
    example_gpio_input();
    example_adc();
    example_uart();
    example_i2c();
    example_spi();
    example_pwm();
    example_remap();
    example_compile_time_validation();
    example_mcu_traits();

    printf("========================================\n");
    printf("✅ All examples completed successfully!\n");
    printf("========================================\n");

    return 0;
}
