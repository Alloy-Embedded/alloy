/// UART Echo Example for Blue Pill (STM32F103C8T6)
///
/// This example demonstrates UART usage on the STM32F103 (Blue Pill board).
/// It echoes back any characters received on USART1.
///
/// Hardware:
/// - Blue Pill board (STM32F103C8T6)
/// - USB-to-Serial adapter connected to:
///   - PA9 (USART1_TX)
///   - PA10 (USART1_RX)
///   - GND
///
/// Serial settings: 115200 baud, 8N1
///
/// Build:
///   cmake -B build -DALLOY_BOARD=bluepill
///   cmake --build build --target uart_echo_bluepill
///
/// Flash:
///   st-flash write build/uart_echo_bluepill.bin 0x8000000
///
/// Test:
///   screen /dev/ttyUSB0 115200
///   (or use any serial terminal)

#include "hal/stm32f1/uart.hpp"
#include "hal/stm32f1/gpio.hpp"
#include "hal/stm32f1/delay.hpp"

using namespace alloy::hal;
using namespace alloy::core;

int main() {
    // Configure USART1 pins (PA9=TX, PA10=RX)
    // PA9 = (0 * 16) + 9 = 9
    // PA10 = (0 * 16) + 10 = 10
    stm32f1::GpioPin<9> tx_pin;
    stm32f1::GpioPin<10> rx_pin;

    // Configure TX as alternate function
    tx_pin.configure(PinMode::Alternate);

    // Configure RX as input (floating)
    rx_pin.configure(PinMode::Input);

    // Create UART device
    stm32f1::UartDevice<stm32f1::UsartId::USART1> uart;

    // Optional: Configure for specific baud rate
    UartConfig config{baud_rates::Baud115200};
    uart.configure(config);

    // Send welcome message
    const char* welcome = "Alloy UART Echo - Blue Pill\r\n";
    const char* prompt = "Type something (it will be echoed back):\r\n";

    for (const char* p = welcome; *p; ++p) {
        // Wait until we can send
        while (uart.write_byte(static_cast<u8>(*p)).is_error()) {
            stm32f1::delay_ms(1);
        }
    }

    for (const char* p = prompt; *p; ++p) {
        while (uart.write_byte(static_cast<u8>(*p)).is_error()) {
            stm32f1::delay_ms(1);
        }
    }

    // Echo loop
    while (true) {
        // Check if data is available
        if (uart.available() > 0) {
            // Read byte
            auto result = uart.read_byte();
            if (result.is_ok()) {
                u8 received = result.value();

                // Echo it back
                // Keep trying until we can send
                while (uart.write_byte(received).is_error()) {
                    stm32f1::delay_ms(1);
                }
            }
        }

        // Small delay to prevent busy-waiting
        stm32f1::delay_ms(1);
    }

    return 0;
}
