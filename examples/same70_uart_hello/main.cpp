/// UART Example for ATSAME70 Xplained
///
/// This example demonstrates:
/// - UART0 initialization and configuration
/// - GPIO configuration for UART pins
/// - Serial communication at 115200 baud
/// - Formatted text output
///
/// Hardware: Atmel SAME70 Xplained board
/// UART: UART0 on PA9 (RX) and PA10 (TX)
/// Baud Rate: 115200
/// Clock: 300MHz system, 150MHz peripheral

#include <cstdint>
#include <cstring>

// SAME70 Memory Map
namespace SAME70 {
// Base addresses
constexpr uint32_t PMC_BASE = 0x400E0600;      // Power Management Controller
constexpr uint32_t PIOA_BASE = 0x400E0E00;     // Parallel I/O Controller A
constexpr uint32_t UART0_BASE = 0x400E0800;    // UART0
constexpr uint32_t SYSTICK_BASE = 0xE000E010;  // ARM Cortex-M7 SysTick

// PMC Registers
struct PMC {
    volatile uint32_t SCER;   // +0x00 System Clock Enable Register
    volatile uint32_t SCDR;   // +0x04 System Clock Disable Register
    volatile uint32_t SCSR;   // +0x08 System Clock Status Register
    uint32_t _reserved0;      // +0x0C
    volatile uint32_t PCER0;  // +0x10 Peripheral Clock Enable Register 0
    volatile uint32_t PCDR0;  // +0x14 Peripheral Clock Disable Register 0
    volatile uint32_t PCSR0;  // +0x18 Peripheral Clock Status Register 0
};

// PIO Registers
struct PIO {
    volatile uint32_t PER;        // +0x00 PIO Enable Register
    volatile uint32_t PDR;        // +0x04 PIO Disable Register
    volatile uint32_t PSR;        // +0x08 PIO Status Register
    uint32_t _reserved0;          // +0x0C
    volatile uint32_t OER;        // +0x10 Output Enable Register
    volatile uint32_t ODR;        // +0x14 Output Disable Register
    volatile uint32_t OSR;        // +0x18 Output Status Register
    uint32_t _reserved1;          // +0x1C
    volatile uint32_t IFER;       // +0x20 Glitch Input Filter Enable
    volatile uint32_t IFDR;       // +0x24 Glitch Input Filter Disable
    volatile uint32_t IFSR;       // +0x28 Glitch Input Filter Status
    uint32_t _reserved2;          // +0x2C
    volatile uint32_t SODR;       // +0x30 Set Output Data Register
    volatile uint32_t CODR;       // +0x34 Clear Output Data Register
    volatile uint32_t ODSR;       // +0x38 Output Data Status Register
    volatile uint32_t PDSR;       // +0x3C Pin Data Status Register
    volatile uint32_t IER;        // +0x40 Interrupt Enable Register
    volatile uint32_t IDR;        // +0x44 Interrupt Disable Register
    volatile uint32_t IMR;        // +0x48 Interrupt Mask Register
    volatile uint32_t ISR;        // +0x4C Interrupt Status Register
    uint32_t _reserved3[4];       // +0x50-0x5C
    volatile uint32_t MDER;       // +0x60 Multi-driver Enable
    volatile uint32_t MDDR;       // +0x64 Multi-driver Disable
    volatile uint32_t MDSR;       // +0x68 Multi-driver Status
    uint32_t _reserved4;          // +0x6C
    volatile uint32_t PUDR;       // +0x70 Pull-up Disable Register
    volatile uint32_t PUER;       // +0x74 Pull-up Enable Register
    volatile uint32_t PUSR;       // +0x78 Pull-up Status Register
    uint32_t _reserved5;          // +0x7C
    volatile uint32_t ABCDSR[2];  // +0x80, +0x84 Peripheral ABCD Select
};

// UART Registers
struct UART {
    volatile uint32_t CR;    // +0x00 Control Register
    volatile uint32_t MR;    // +0x04 Mode Register
    volatile uint32_t IER;   // +0x08 Interrupt Enable Register
    volatile uint32_t IDR;   // +0x0C Interrupt Disable Register
    volatile uint32_t IMR;   // +0x10 Interrupt Mask Register
    volatile uint32_t SR;    // +0x14 Status Register
    volatile uint32_t RHR;   // +0x18 Receive Holding Register
    volatile uint32_t THR;   // +0x1C Transmit Holding Register
    volatile uint32_t BRGR;  // +0x20 Baud Rate Generator Register
};

// SysTick Registers
struct SysTick {
    volatile uint32_t CTRL;   // Control and Status
    volatile uint32_t LOAD;   // Reload Value
    volatile uint32_t VAL;    // Current Value
    volatile uint32_t CALIB;  // Calibration
};

inline PMC* pmc() {
    return reinterpret_cast<PMC*>(PMC_BASE);
}
inline PIO* pioa() {
    return reinterpret_cast<PIO*>(PIOA_BASE);
}
inline UART* uart0() {
    return reinterpret_cast<UART*>(UART0_BASE);
}
inline SysTick* systick() {
    return reinterpret_cast<SysTick*>(SYSTICK_BASE);
}

// Peripheral IDs
constexpr uint32_t ID_PIOA = 10;
constexpr uint32_t ID_UART0 = 7;
}  // namespace SAME70

// Delay functions
namespace Delay {
volatile uint32_t tick_count = 0;

void init() {
    SAME70::systick()->LOAD = 300000 - 1;  // 1ms @ 300MHz
    SAME70::systick()->VAL = 0;
    SAME70::systick()->CTRL = 0x07;
}

void ms(uint32_t milliseconds) {
    uint32_t start = tick_count;
    while ((tick_count - start) < milliseconds) {
        __asm__ volatile("nop");
    }
}
}  // namespace Delay

// UART driver
namespace UART {
void init(uint32_t baudrate = 115200) {
    // Enable PIOA and UART0 clocks
    SAME70::pmc()->PCER0 = (1 << SAME70::ID_PIOA) | (1 << SAME70::ID_UART0);

    // Configure PA9 (URXD0) and PA10 (UTXD0) for UART peripheral A
    SAME70::pioa()->PDR = (1 << 9) | (1 << 10);            // Disable PIO control
    SAME70::pioa()->ABCDSR[0] &= ~((1 << 9) | (1 << 10));  // Select peripheral A
    SAME70::pioa()->ABCDSR[1] &= ~((1 << 9) | (1 << 10));

    // Reset and disable UART0
    SAME70::uart0()->CR = (1 << 2) | (1 << 3);  // RSTRX | RSTTX
    SAME70::uart0()->CR = (1 << 5) | (1 << 6);  // RXDIS | TXDIS

    // Configure UART mode: 8N1 (8 bits, no parity, 1 stop bit)
    SAME70::uart0()->MR = (0x4 << 9);  // PAR = 0x4 (no parity)

    // Configure baud rate
    // BRGR = MCK / (16 * baudrate)
    // Assuming peripheral clock = 150MHz
    constexpr uint32_t MCK = 150000000;
    uint32_t cd = MCK / (16 * baudrate);
    SAME70::uart0()->BRGR = cd;

    // Enable transmitter and receiver
    SAME70::uart0()->CR = (1 << 4) | (1 << 6);  // RXEN | TXEN
}

void putc(char c) {
    // Wait for transmitter ready
    while (!(SAME70::uart0()->SR & (1 << 1))) {  // TXRDY bit
        __asm__ volatile("nop");
    }
    SAME70::uart0()->THR = c;
}

void puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            putc('\r');  // Add carriage return for newline
        }
        putc(*str++);
    }
}

char getc() {
    // Wait for receiver ready
    while (!(SAME70::uart0()->SR & (1 << 0))) {  // RXRDY bit
        __asm__ volatile("nop");
    }
    return static_cast<char>(SAME70::uart0()->RHR);
}

// Simple formatted output
void printf(const char* format, uint32_t value) {
    char buffer[32];
    int idx = 0;

    while (*format) {
        if (*format == '%' && *(format + 1) == 'd') {
            // Convert integer to string
            if (value == 0) {
                buffer[idx++] = '0';
            } else {
                char temp[16];
                int temp_idx = 0;
                uint32_t val = value;

                while (val > 0) {
                    temp[temp_idx++] = '0' + (val % 10);
                    val /= 10;
                }

                // Reverse
                while (temp_idx > 0) {
                    buffer[idx++] = temp[--temp_idx];
                }
            }
            format += 2;
        } else {
            buffer[idx++] = *format++;
        }
    }
    buffer[idx] = '\0';
    puts(buffer);
}
}  // namespace UART

// SysTick interrupt handler
extern "C" void SysTick_Handler() {
    Delay::tick_count++;
}

int main() {
    // Initialize peripherals
    Delay::init();
    UART::init(115200);

    // Send startup message
    UART::puts("\n\n");
    UART::puts("========================================\n");
    UART::puts("  ATSAME70 Xplained - UART Example\n");
    UART::puts("========================================\n");
    UART::puts("  MCU: ATSAME70Q21\n");
    UART::puts("  Core: ARM Cortex-M7 @ 300MHz\n");
    UART::puts("  UART: UART0 @ 115200 baud\n");
    UART::puts("  Pins: PA9 (RX), PA10 (TX)\n");
    UART::puts("========================================\n\n");

    uint32_t counter = 0;

    while (true) {
        // Send counter value
        UART::puts("Counter: ");
        UART::printf("%d", counter);
        UART::puts("\n");

        counter++;

        // Send uptime
        UART::puts("Uptime: ");
        UART::printf("%d", Delay::tick_count / 1000);
        UART::puts(" seconds\n\n");

        // Wait 1 second
        Delay::ms(1000);

        // Echo any received characters
        if (SAME70::uart0()->SR & (1 << 0)) {  // Check RXRDY
            char c = UART::getc();
            UART::puts("Received: ");
            UART::putc(c);
            UART::puts("\n\n");
        }
    }

    return 0;
}
