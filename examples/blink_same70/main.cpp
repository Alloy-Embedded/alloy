/// Blink LED Example for ATSAME70 Xplained
///
/// This example demonstrates GPIO and basic timing on the SAME70.
///
/// Hardware: Atmel SAME70 Xplained board
/// LED: PC8 (LED0, built-in LED)
/// Clock: 300MHz Cortex-M7
///
/// Features used:
/// - GPIO (PIOC) for LED control
/// - System tick timer for delays

#include <cstdint>

// SAME70 Memory Map
namespace SAME70 {
    // Base addresses
    constexpr uint32_t PMC_BASE = 0x400E0600;      // Power Management Controller
    constexpr uint32_t PIOC_BASE = 0x400E1200;     // Parallel I/O Controller C
    constexpr uint32_t SYSTICK_BASE = 0xE000E010;  // ARM Cortex-M7 SysTick

    // PMC Registers
    struct PMC {
        volatile uint32_t SCER;       // System Clock Enable Register
        volatile uint32_t SCDR;       // System Clock Disable Register
        volatile uint32_t SCSR;       // System Clock Status Register
        uint32_t _reserved0;
        volatile uint32_t PCER0;      // Peripheral Clock Enable Register 0
        volatile uint32_t PCDR0;      // Peripheral Clock Disable Register 0
        volatile uint32_t PCSR0;      // Peripheral Clock Status Register 0
        volatile uint32_t UCKR;       // UTMI Clock Register
        volatile uint32_t MOR;        // Main Oscillator Register
        volatile uint32_t MCFR;       // Main Clock Frequency Register
        volatile uint32_t PLLAR;      // PLLA Register
        uint32_t _reserved1;
        volatile uint32_t MCKR;       // Master Clock Register
    };

    // PIO Registers
    struct PIO {
        volatile uint32_t PER;        // PIO Enable Register
        volatile uint32_t PDR;        // PIO Disable Register
        volatile uint32_t PSR;        // PIO Status Register
        uint32_t _reserved0;
        volatile uint32_t OER;        // Output Enable Register
        volatile uint32_t ODR;        // Output Disable Register
        volatile uint32_t OSR;        // Output Status Register
        uint32_t _reserved1;
        volatile uint32_t IFER;       // Glitch Input Filter Enable Register
        volatile uint32_t IFDR;       // Glitch Input Filter Disable Register
        volatile uint32_t IFSR;       // Glitch Input Filter Status Register
        uint32_t _reserved2;
        volatile uint32_t SODR;       // Set Output Data Register
        volatile uint32_t CODR;       // Clear Output Data Register
        volatile uint32_t ODSR;       // Output Data Status Register
        volatile uint32_t PDSR;       // Pin Data Status Register
    };

    // SysTick Registers
    struct SysTick {
        volatile uint32_t CTRL;       // Control and Status
        volatile uint32_t LOAD;       // Reload Value
        volatile uint32_t VAL;        // Current Value
        volatile uint32_t CALIB;      // Calibration
    };

    inline PMC* pmc() { return reinterpret_cast<PMC*>(PMC_BASE); }
    inline PIO* pioc() { return reinterpret_cast<PIO*>(PIOC_BASE); }
    inline SysTick* systick() { return reinterpret_cast<SysTick*>(SYSTICK_BASE); }
}

// LED on PC8
namespace Led {
    constexpr uint32_t PIN = (1 << 8);  // PC8
    constexpr uint32_t PIOC_ID = 13;    // PIOC peripheral ID

    void init() {
        // Enable PIOC clock
        SAME70::pmc()->PCER0 = (1 << PIOC_ID);

        // Configure PC8 as output
        SAME70::pioc()->PER = PIN;   // Enable PIO control
        SAME70::pioc()->OER = PIN;   // Enable output
    }

    void on() {
        SAME70::pioc()->SODR = PIN;  // Set output high
    }

    void off() {
        SAME70::pioc()->CODR = PIN;  // Clear output low
    }

    void toggle() {
        if (SAME70::pioc()->ODSR & PIN) {
            off();
        } else {
            on();
        }
    }
}

// Simple delay using SysTick
namespace Delay {
    volatile uint32_t tick_count = 0;

    void init() {
        // Configure SysTick for 1ms tick (300MHz / 1000 = 300000)
        SAME70::systick()->LOAD = 300000 - 1;
        SAME70::systick()->VAL = 0;
        SAME70::systick()->CTRL = 0x07;  // Enable, interrupt, use processor clock
    }

    void ms(uint32_t milliseconds) {
        uint32_t start = tick_count;
        while ((tick_count - start) < milliseconds) {
            __asm__ volatile("nop");
        }
    }
}

// SysTick interrupt handler
extern "C" void SysTick_Handler() {
    Delay::tick_count++;
}

int main() {
    // Initialize peripherals
    Led::init();
    Delay::init();

    // Blink LED forever
    while (true) {
        Led::on();
        Delay::ms(500);

        Led::off();
        Delay::ms(500);
    }

    return 0;
}
