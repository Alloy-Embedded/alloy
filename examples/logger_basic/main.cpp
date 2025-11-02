/// Basic Logger Example for STM32F103C8 (Blue Pill)
///
/// This example demonstrates the CoreZero universal logger system:
/// - Zero-cost compile-time log filtering
/// - Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR)
/// - Automatic timestamps from SysTick (microsecond precision)
/// - Source location (file:line)
/// - UART output via sink architecture
///
/// Hardware: STM32F103C8 (Blue Pill)
/// UART: USART1 (PA9=TX, PA10=RX) @ 115200 baud
/// LED: PC13 (built-in LED, active LOW)
///
/// Output format:
/// [timestamp] LEVEL [file:line] message
/// Example: [0.123456] INFO  [main.cpp:42] Application started

#include "stm32f103c8/board.hpp"
#include "hal/st/stm32f1/uart.hpp"
#include "hal/st/stm32f1/gpio.hpp"
#include "logger/logger.hpp"
#include "logger/platform/uart_sink.hpp"
#include "core/types.hpp"

using namespace alloy;
using namespace alloy::hal::stm32f1;

// USART1 type alias
using Uart1 = UartDevice<UsartId::USART1>;

// GPIO pins for USART1
using UartTx = GpioPinOutput<GpioPort::PA, 9>;  // PA9 = USART1_TX
using UartRx = GpioPinInput<GpioPort::PA, 10>; // PA10 = USART1_RX

int main() {
    // Initialize board (also initializes SysTick timer automatically)
    Board::initialize();

    // Initialize LED
    Board::Led::init();
    Board::Led::on();  // LED on during setup

    // Configure UART pins (alternate function push-pull for TX)
    UartTx::init();
    UartTx::set_mode(GpioMode::OUTPUT_50MHZ);
    UartTx::set_config(GpioConfig::OUT_ALT_PUSH_PULL);

    UartRx::init();
    UartRx::set_mode(GpioMode::INPUT);
    UartRx::set_config(GpioConfig::IN_FLOATING);

    // Initialize UART1
    auto uart_result = Uart1::init();
    if (uart_result.is_error()) {
        // Blink LED rapidly on error
        while (true) {
            Board::Led::toggle();
            alloy::systick::delay_us(100000);  // 100ms
        }
    }

    // Configure UART: 115200 baud, 8N1
    hal::UartConfig config{
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = hal::Parity::None
    };

    auto config_result = Uart1::configure(config);
    if (config_result.is_error()) {
        // Blink LED rapidly on error
        while (true) {
            Board::Led::toggle();
            alloy::systick::delay_us(100000);  // 100ms
        }
    }

    // Create UART sink and register with logger
    logger::UartSink<Uart1> uart_sink;
    logger::Logger::add_sink(&uart_sink);

    // Configure logger
    logger::Logger::set_level(logger::Level::Trace);  // Show all logs

    // LED off - setup complete
    Board::Led::off();

    // Log startup message
    LOG_INFO("=== CoreZero Logger Example ===");
    LOG_INFO("Board: STM32F103C8 (Blue Pill)");
    LOG_INFO("Logger initialized successfully");

    // Demonstrate all log levels
    LOG_TRACE("This is a TRACE message - very detailed");
    LOG_DEBUG("This is a DEBUG message - for development");
    LOG_INFO("This is an INFO message - normal operation");
    LOG_WARN("This is a WARN message - potential issue");
    LOG_ERROR("This is an ERROR message - something failed");

    // Demonstrate formatted logging
    core::u32 counter = 0;
    float temperature = 23.5f;
    const char* status = "OK";

    LOG_INFO("Counter: %d, Temp: %.1f°C, Status: %s", counter, temperature, status);

    // Main loop: log periodically and blink LED
    while (true) {
        core::u32 start = alloy::systick::micros();

        // Blink LED
        Board::Led::toggle();

        // Log counter value
        LOG_INFO("Loop iteration: %d", counter);

        // Every 10 iterations, show different log levels
        if (counter % 10 == 0) {
            LOG_DEBUG("Debug info at iteration %d", counter);
        }

        if (counter % 20 == 0) {
            LOG_WARN("Warning: counter reached %d", counter);
        }

        // Simulate temperature reading
        temperature += 0.1f;
        if (temperature > 30.0f) {
            temperature = 20.0f;
        }

        LOG_DEBUG("Temperature: %.2f°C", temperature);

        counter++;

        // Wait 1 second (1,000,000 microseconds)
        while (!alloy::systick::is_timeout(start, 1000000));
    }

    return 0;
}

// Weak symbols for startup code
extern "C" {
    void SystemInit() {
        // Optional: Configure clocks here
        // For now, running on default HSI (8MHz)
    }
}
