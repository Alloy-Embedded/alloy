/**
 * @file main.cpp
 * @brief UART Logger Example
 *
 * Demonstrates how to use the logger system with UART output on embedded systems.
 *
 * ## What This Example Shows
 *
 * - Board initialization
 * - UART configuration for logging output
 * - Logger setup with UART sink
 * - Different log levels (INFO, WARN, ERROR, DEBUG)
 * - Periodic logging with timestamps
 *
 * ## Hardware Requirements
 *
 * - **Board:** SAME70 Xplained Ultra
 * - **UART:** Debug UART (EDBG virtual COM port)
 * - **Baud Rate:** 115200
 * - **Connection:** USB cable to PC (EDBG connector)
 *
 * ## Expected Behavior
 *
 * The example outputs log messages to UART every second:
 * - System startup message
 * - Periodic INFO messages with uptime
 * - Occasional WARN and ERROR messages for demonstration
 * - LED blinks in sync with logging
 *
 * ## Viewing Output
 *
 * 1. Connect board via USB (EDBG port)
 * 2. Open serial terminal (115200 baud, 8N1)
 * 3. Flash and run the example
 * 4. Observe formatted log messages with timestamps
 *
 * @note UART TX pin must be configured correctly for your board.
 *       Consult board documentation for UART pin mapping.
 */

#include "same70_xplained/board.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/api/uart_simple.hpp"
#include "hal/vendors/arm/same70/gpio.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"
#include "logger/logger.hpp"
#include "logger/sinks/uart_sink.hpp"

using namespace alloy::hal;
using namespace alloy::hal::same70;
using namespace alloy::hal::atmel::same70;
using namespace alloy::logger;
using namespace alloy::generated::atsame70q21b;

int main() {
    // Step 1: Initialize board hardware
    board::init();

    // Debug: Blink 3 times to show board init OK
    for (int i = 0; i < 3; i++) {
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(200);
        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(200);
    }

    // Step 2: Configure UART for logging (115200 baud, TX-only)
    // SAME70 Xplained Ultra: UART0 is connected to EDBG (virtual COM port)
    // TX pin: PA10 (UART0_TXD), Base: 0x400E0800, IRQ: 7
    using UartTxPin = GpioPin<peripherals::PIOA, 10>;
    using UartPolicy = Same70UARTHardwarePolicy<0x400E0800, 7>;

    SimpleUartConfigTxOnly<UartTxPin, UartPolicy> uart{
        PeripheralId::UART0,
        BaudRate{115200},
        8,  // data bits
        UartParity::NONE,
        1   // stop bits
    };

    // Step 3: Configure GPIO pin for UART peripheral function
    // PA10 needs to be configured for UART0_TXD (peripheral A function)
    // Enable clocks for PIOA and UART0
    volatile uint32_t* PMC_PCER0 = reinterpret_cast<volatile uint32_t*>(0x400E0610);
    *PMC_PCER0 = (1 << 10);  // Enable PIOA clock (ID 10)
    *PMC_PCER0 = (1 << 7);   // Enable UART0 clock (ID 7 - IRQ ID)

    // Configure PA10 for peripheral A (UART0_TXD)
    // Peripheral select: ABCDSR[1:0] -> 00=A, 01=B, 10=C, 11=D
    volatile uint32_t* PIOA_PDR = reinterpret_cast<volatile uint32_t*>(peripherals::PIOA + 0x04);   // Disable PIO
    volatile uint32_t* PIOA_ABCDSR1 = reinterpret_cast<volatile uint32_t*>(peripherals::PIOA + 0x70);
    volatile uint32_t* PIOA_ABCDSR2 = reinterpret_cast<volatile uint32_t*>(peripherals::PIOA + 0x74);

    *PIOA_PDR = (1 << 10);          // Disable PIO control on PA10
    *PIOA_ABCDSR1 &= ~(1 << 10);    // ABCDSR1 bit = 0
    *PIOA_ABCDSR2 &= ~(1 << 10);    // ABCDSR2 bit = 0 -> Select peripheral A

    // Debug: Blink 2 times to show GPIO config OK
    for (int i = 0; i < 2; i++) {
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(200);
        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(200);
    }

    // Initialize UART
    auto uart_result = uart.initialize();
    if (!uart_result.is_ok()) {
        // UART initialization failed - blink LED rapidly to indicate error
        while (true) {
            board::led::toggle();
            SysTickTimer::delay_ms<board::BoardSysTick>(100);
        }
    }

    // Debug: Blink 1 time to show UART init OK
    board::led::on();
    SysTickTimer::delay_ms<board::BoardSysTick>(500);
    board::led::off();
    SysTickTimer::delay_ms<board::BoardSysTick>(500);

    // Step 4: Send test directly to UART hardware (raw register access)
    volatile uint32_t* UART0_CR = reinterpret_cast<volatile uint32_t*>(0x400E0800);   // Control Register
    volatile uint32_t* UART0_MR = reinterpret_cast<volatile uint32_t*>(0x400E0804);   // Mode Register
    volatile uint32_t* UART0_BRGR = reinterpret_cast<volatile uint32_t*>(0x400E0820); // Baud Rate Generator
    volatile uint32_t* UART0_SR = reinterpret_cast<volatile uint32_t*>(0x400E0814);   // Status Register
    volatile uint32_t* UART0_THR = reinterpret_cast<volatile uint32_t*>(0x400E081C);  // Transmit Holding Register

    // Reset and configure UART0 manually
    *UART0_CR = (1 << 2) | (1 << 3);  // Reset TX and RX
    *UART0_CR = (1 << 6) | (1 << 7);  // Disable TX and RX

    // Configure 8N1 mode (no parity)
    *UART0_MR = (4 << 9);  // PAR field = 4 (no parity)

    // Set baud rate: CD = MCK / (16 * baudrate) = 12000000 / (16 * 115200) = 6.51 ≈ 7
    *UART0_BRGR = 7;

    // Enable TX
    *UART0_CR = (1 << 6);  // TXEN

    // Wait for UART to stabilize and EDBG to be ready
    SysTickTimer::delay_ms<board::BoardSysTick>(500);

    // Check if TXRDY is set (if not, UART is not working)
    uint32_t status = *UART0_SR;
    if (!(status & (1 << 1))) {
        // TXRDY not ready - blink LED 10 times fast to indicate error
        for (int i = 0; i < 10; i++) {
            board::led::toggle();
            SysTickTimer::delay_ms<board::BoardSysTick>(50);
        }
        // Hang here
        while(true) {
            SysTickTimer::delay_ms<board::BoardSysTick>(1000);
        }
    }

    // TXRDY is ready - blink 3 times to confirm
    for (int i = 0; i < 3; i++) {
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(100);
        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(100);
    }

    // Send test message continuously
    const char* msg = "UART0 TEST\r\n";
    for (int loop = 0; loop < 100; loop++) {
        for (const char* p = msg; *p != '\0'; p++) {
            // Wait for TX ready
            while (!((*UART0_SR) & (1 << 1))) {
                // Busy wait
            }
            *UART0_THR = *p;
        }

        // Blink LED to show message sent
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(200);
    }

    // Step 5: Main loop - send test messages continuously
    u32 loop_count = 0;

    while (true) {
        // Send simple counter message directly via UART (bypass logger)
        const char* msg = "Test ";
        for (const char* p = msg; *p != '\0'; p++) {
            uart.write_byte(*p);
        }

        // Send loop count as decimal number
        char num[12];
        u32 n = loop_count;
        int i = 0;
        if (n == 0) {
            num[i++] = '0';
        } else {
            char temp[12];
            int j = 0;
            while (n > 0) {
                temp[j++] = '0' + (n % 10);
                n /= 10;
            }
            for (int k = j - 1; k >= 0; k--) {
                num[i++] = temp[k];
            }
        }
        num[i] = '\0';

        for (int k = 0; num[k] != '\0'; k++) {
            uart.write_byte(num[k]);
        }

        uart.write_byte('\r');
        uart.write_byte('\n');

        // Blink LED
        board::led::toggle();

        // Wait 1 second
        SysTickTimer::delay_ms<board::BoardSysTick>(1000);

        loop_count++;
    }

    return 0;
}
