/**
 * @file same70_xplained_peripherals_demo.cpp
 * @brief Complete peripheral demonstration for SAME70 Xplained Ultra
 *
 * This example demonstrates the OpenSpec template-based peripheral pattern
 * with type aliases for all major peripherals.
 *
 * Features demonstrated:
 * - UART serial communication
 * - SPI communication
 * - I2C communication
 * - Timer periodic interrupts
 * - ADC analog reading
 * - GPIO pins
 * - Board LED and button control
 *
 * Hardware connections:
 * - UART0: TX=PA10, RX=PA9 (USB virtual COM port)
 * - SPI0: MISO=PD20, MOSI=PD21, SCK=PD22, CS0=PB2
 * - I2C0: SDA=PA3, SCL=PA4
 * - ADC0: Channel 0
 * - LED0: PC8 (onboard LED)
 * - Button SW0: PA11 (onboard button)
 *
 * @note This example uses the board type aliases from board_config.hpp
 * @note All peripherals follow OpenSpec REQ-TP-001 to REQ-TP-008
 */

#include "boards/same70_xplained/board.hpp"
#include "hal/uart_expert.hpp"
#include "hal/timer_expert.hpp"

using namespace alloy::boards::same70_xplained;
using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Global State
// ============================================================================

volatile bool timer_tick = false;
volatile uint32_t tick_count = 0;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Send string via UART
 */
void uart_print(const char* str) {
    while (*str) {
        Uart0::write(static_cast<u8>(*str++));
    }
}

/**
 * @brief Send string with newline
 */
void uart_println(const char* str) {
    uart_print(str);
    uart_print("\r\n");
}

// ============================================================================
// Demo Functions
// ============================================================================

/**
 * @brief Demo 1: UART Communication
 */
void demo_uart() {
    uart_println("=== UART Demo ===");

    // UART0 already initialized in setup()
    uart_println("UART0 working!");
    uart_print("Baud rate: 115200\r\n");
    uart_print("Data bits: 8, Parity: None, Stop bits: 1\r\n");

    uart_println("");
}

/**
 * @brief Demo 2: SPI Communication
 */
void demo_spi() {
    uart_println("=== SPI Demo ===");

    // Initialize SPI0
    Spi0 spi;
    auto result = spi.open();

    if (result.is_ok()) {
        uart_println("SPI0 initialized");

        // Configure chip select 0
        spi.configureChipSelect(SpiChipSelect::CS0, 10, SpiMode::Mode0);

        // Test transfer
        u8 tx[4] = {0xAA, 0xBB, 0xCC, 0xDD};
        u8 rx[4] = {0};

        spi.transfer(tx, rx, 4, SpiChipSelect::CS0);

        uart_print("Sent: 0xAA 0xBB 0xCC 0xDD\r\n");
        // In real hardware, rx would contain response

        spi.close();
    } else {
        uart_println("SPI0 init failed");
    }

    uart_println("");
}

/**
 * @brief Demo 3: I2C Communication
 */
void demo_i2c() {
    uart_println("=== I2C Demo ===");

    // Initialize I2C0
    I2c0 i2c;
    auto result = i2c.open();

    if (result.is_ok()) {
        uart_println("I2C0 initialized");

        // Set speed to 100 kHz
        i2c.setSpeed(I2cSpeed::Standard100kHz);

        // Example: Scan I2C bus for devices (0x08 to 0x77)
        uart_println("Scanning I2C bus...");
        bool found = false;

        for (u8 addr = 0x08; addr < 0x78; addr++) {
            u8 dummy = 0;
            auto write_result = i2c.write(addr, &dummy, 0);  // Test ACK

            if (write_result.is_ok()) {
                uart_print("Device found at 0x");
                // Print hex address (simplified)
                found = true;
            }
        }

        if (!found) {
            uart_println("No I2C devices found");
        }

        i2c.close();
    } else {
        uart_println("I2C0 init failed");
    }

    uart_println("");
}

/**
 * @brief Demo 4: Timer Periodic Interrupt
 */
void demo_timer() {
    uart_println("=== Timer Demo ===");

    // Configure Timer0 for 100ms periodic interrupt
    auto timer_config = TimerExpertConfig::periodic_interrupt(
        PeripheralId::TIMER0,
        100000  // 100ms in microseconds
    );

    Timer0 timer;
    auto result = timer.initialize(timer_config);

    if (result.is_ok()) {
        uart_println("Timer0 configured for 100ms ticks");

        // Start timer
        timer.start();

        uart_println("Timer started");
        uart_println("(Timer ISR would increment tick_count)");

        // In real implementation, ISR would be:
        // void TIMER0_Handler() { tick_count++; timer_tick = true; }
    } else {
        uart_println("Timer0 init failed");
    }

    uart_println("");
}

/**
 * @brief Demo 5: ADC Analog Reading
 */
void demo_adc() {
    uart_println("=== ADC Demo ===");

    // Initialize ADC0
    Adc0 adc;
    auto result = adc.open();

    if (result.is_ok()) {
        uart_println("ADC0 initialized");

        // Read channel 0
        auto read_result = adc.read(0);

        if (read_result.is_ok()) {
            u16 value = read_result.unwrap();
            uart_print("ADC Ch0 value: ");
            // Print value (0-4095 for 12-bit)
            uart_println("(12-bit value)");

            // Convert to voltage (assuming 3.3V reference)
            // voltage = (value / 4095.0) * 3.3
        } else {
            uart_println("ADC read failed");
        }

        adc.close();
    } else {
        uart_println("ADC0 init failed");
    }

    uart_println("");
}

/**
 * @brief Demo 6: GPIO Pins
 */
void demo_gpio() {
    uart_println("=== GPIO Demo ===");

    // LED control (already initialized by board::init())
    uart_println("LED control:");
    board::led::on();
    uart_print("  LED ON... ");
    board::delay_ms(500);

    board::led::off();
    uart_println("OFF");

    // Button reading
    uart_println("Button test:");
    uart_println("  Press SW0 button...");

    uint32_t timeout = 5000;  // 5 second timeout
    uint32_t start = board::micros();

    while ((board::micros() - start) < timeout * 1000) {
        if (board::button::read()) {
            uart_println("  Button PRESSED!");
            board::led::toggle();
            board::delay_ms(20);  // Debounce

            while (board::button::read()) {}  // Wait release
            board::delay_ms(20);  // Debounce
            break;
        }
    }

    uart_println("");
}

/**
 * @brief Demo 7: Multiple Peripheral Instances
 */
void demo_multiple_instances() {
    uart_println("=== Multiple Instances Demo ===");

    // Demonstrate using multiple UART instances simultaneously
    uart_println("This board has 3 UART instances:");
    uart_println("  - Uart0 (currently in use)");
    uart_println("  - Uart1 (available)");
    uart_println("  - Uart2 (available)");

    uart_println("Each can be used independently:");
    uart_println("  Uart0::write('A');");
    uart_println("  Uart1::write('B');");
    uart_println("  Uart2::write('C');");

    uart_println("");

    uart_println("Similarly for SPI:");
    uart_println("  Spi0, Spi1");

    uart_println("And I2C:");
    uart_println("  I2c0, I2c1, I2c2");

    uart_println("All with ZERO runtime overhead!");

    uart_println("");
}

/**
 * @brief Demo 8: Type Aliases Benefits
 */
void demo_type_aliases() {
    uart_println("=== Type Aliases Benefits ===");

    uart_println("OpenSpec Pattern Advantages:");
    uart_println("  1. Zero overhead (compile-time resolution)");
    uart_println("  2. Type-safe (compiler validates)");
    uart_println("  3. Multiple instances (Uart0, Uart1, Uart2)");
    uart_println("  4. Clean syntax (no template params)");
    uart_println("  5. Portable (just change board header)");

    uart_println("");

    uart_println("Example comparison:");
    uart_println("  Old: Uart<PeripheralId::USART0>::write('H');");
    uart_println("  New: Uart0::write('H');");

    uart_println("");
}

// ============================================================================
// Setup and Main
// ============================================================================

void setup() {
    // Initialize board (clocks, systick, GPIO)
    board::init();

    // Initialize UART0 for serial communication
    UartExpertConfig uart_config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    Uart0::initialize(uart_config);

    // Small delay for UART to stabilize
    board::delay_ms(100);
}

int main() {
    setup();

    uart_println("");
    uart_println("========================================");
    uart_println("  SAME70 Peripherals Demo");
    uart_println("  OpenSpec Template Pattern");
    uart_println("========================================");
    uart_println("");

    board::delay_ms(500);

    // Run all demos
    demo_uart();
    board::delay_ms(500);

    demo_spi();
    board::delay_ms(500);

    demo_i2c();
    board::delay_ms(500);

    demo_timer();
    board::delay_ms(500);

    demo_adc();
    board::delay_ms(500);

    demo_gpio();
    board::delay_ms(500);

    demo_multiple_instances();
    board::delay_ms(500);

    demo_type_aliases();
    board::delay_ms(500);

    uart_println("========================================");
    uart_println("  All Demos Complete!");
    uart_println("========================================");
    uart_println("");

    // Main loop - blink LED
    while (true) {
        board::led::toggle();
        board::delay_ms(1000);

        // Print heartbeat
        uart_println("Heartbeat...");
    }

    return 0;
}

/**
 * Build instructions:
 *
 * cmake -DBOARD=same70_xplained -DCMAKE_BUILD_TYPE=Release ..
 * make same70_xplained_peripherals_demo
 *
 * Flash to board:
 * make flash-same70_xplained_peripherals_demo
 *
 * Monitor serial output:
 * screen /dev/ttyACM0 115200
 */
