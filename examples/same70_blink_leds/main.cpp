/// Advanced LED Blink Example for ATSAME70 Xplained
///
/// This example demonstrates:
/// - Multiple LED control (LED chaser pattern)
/// - GPIO port manipulation
/// - Precise timing with SysTick
/// - Bit manipulation techniques
///
/// Hardware: Atmel SAME70 Xplained board
/// LEDs: PC8 (LED0)
/// Pattern: Knight Rider / Cylon effect
///
/// Since the SAME70 Xplained only has one LED (PC8), we'll create
/// a breathing/fading effect by toggling it at different speeds

#include <cstdint>

// SAME70 Memory Map
namespace SAME70 {
// Base addresses
constexpr uint32_t PMC_BASE = 0x400E0600;      // Power Management Controller
constexpr uint32_t PIOC_BASE = 0x400E1200;     // Parallel I/O Controller C
constexpr uint32_t SYSTICK_BASE = 0xE000E010;  // ARM Cortex-M7 SysTick

// PMC Registers
struct PMC {
    volatile uint32_t SCER;  // System Clock Enable Register
    volatile uint32_t SCDR;  // System Clock Disable Register
    volatile uint32_t SCSR;  // System Clock Status Register
    uint32_t _reserved0;
    volatile uint32_t PCER0;  // Peripheral Clock Enable Register 0
    volatile uint32_t PCDR0;  // Peripheral Clock Disable Register 0
    volatile uint32_t PCSR0;  // Peripheral Clock Status Register 0
};

// PIO Registers
struct PIO {
    volatile uint32_t PER;  // PIO Enable Register
    volatile uint32_t PDR;  // PIO Disable Register
    volatile uint32_t PSR;  // PIO Status Register
    uint32_t _reserved0;
    volatile uint32_t OER;  // Output Enable Register
    volatile uint32_t ODR;  // Output Disable Register
    volatile uint32_t OSR;  // Output Status Register
    uint32_t _reserved1;
    volatile uint32_t IFER;  // Glitch Input Filter Enable Register
    volatile uint32_t IFDR;  // Glitch Input Filter Disable Register
    volatile uint32_t IFSR;  // Glitch Input Filter Status Register
    uint32_t _reserved2;
    volatile uint32_t SODR;  // Set Output Data Register
    volatile uint32_t CODR;  // Clear Output Data Register
    volatile uint32_t ODSR;  // Output Data Status Register
    volatile uint32_t PDSR;  // Pin Data Status Register
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
inline PIO* pioc() {
    return reinterpret_cast<PIO*>(PIOC_BASE);
}
inline SysTick* systick() {
    return reinterpret_cast<SysTick*>(SYSTICK_BASE);
}
}  // namespace SAME70

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

void us(uint32_t microseconds) {
    // Busy-wait loop for microsecond delays
    // At 300MHz, each loop iteration is ~3-4 cycles
    volatile uint32_t count = microseconds * 75;  // Approximately 300 cycles per µs
    while (count--) {
        __asm__ volatile("nop");
    }
}
}  // namespace Delay

// LED breathing/PWM effect using software PWM
namespace LED {
constexpr uint32_t PIN = (1 << 8);  // PC8
constexpr uint32_t PIOC_ID = 13;    // PIOC peripheral ID

void init() {
    // Enable PIOC clock
    SAME70::pmc()->PCER0 = (1 << PIOC_ID);

    // Configure PC8 as output
    SAME70::pioc()->PER = PIN;  // Enable PIO control
    SAME70::pioc()->OER = PIN;  // Enable output
}

void on() {
    SAME70::pioc()->SODR = PIN;
}

void off() {
    SAME70::pioc()->CODR = PIN;
}

void toggle() {
    if (SAME70::pioc()->ODSR & PIN) {
        off();
    } else {
        on();
    }
}

// Software PWM - brightness from 0 (off) to 100 (full)
void set_brightness(uint8_t brightness) {
    constexpr uint32_t PWM_PERIOD = 100;  // 100 steps

    if (brightness == 0) {
        off();
    } else if (brightness >= 100) {
        on();
    } else {
        // Software PWM cycle
        on();
        Delay::us(brightness * 10);  // On time proportional to brightness
        off();
        Delay::us((100 - brightness) * 10);  // Off time
    }
}
}  // namespace LED

// SysTick interrupt handler
extern "C" void SysTick_Handler() {
    Delay::tick_count++;
}

// Breathing effect - LED fades in and out
void breathing_effect() {
    // Fade in
    for (uint8_t brightness = 0; brightness <= 100; brightness++) {
        for (uint32_t i = 0; i < 50; i++) {  // Repeat for smooth effect
            LED::set_brightness(brightness);
        }
    }

    // Fade out
    for (uint8_t brightness = 100; brightness > 0; brightness--) {
        for (uint32_t i = 0; i < 50; i++) {
            LED::set_brightness(brightness);
        }
    }
}

// Morse code SOS pattern
void morse_sos() {
    // S (dot dot dot)
    for (int i = 0; i < 3; i++) {
        LED::on();
        Delay::ms(200);
        LED::off();
        Delay::ms(200);
    }

    Delay::ms(400);  // Letter gap

    // O (dash dash dash)
    for (int i = 0; i < 3; i++) {
        LED::on();
        Delay::ms(600);
        LED::off();
        Delay::ms(200);
    }

    Delay::ms(400);  // Letter gap

    // S (dot dot dot)
    for (int i = 0; i < 3; i++) {
        LED::on();
        Delay::ms(200);
        LED::off();
        Delay::ms(200);
    }

    Delay::ms(1000);  // Word gap
}

// Heartbeat pattern
void heartbeat_pattern() {
    // First beat
    LED::on();
    Delay::ms(100);
    LED::off();
    Delay::ms(100);

    // Second beat
    LED::on();
    Delay::ms(100);
    LED::off();
    Delay::ms(600);  // Pause
}

int main() {
    // Initialize peripherals
    LED::init();
    Delay::init();

    uint8_t pattern = 0;

    // Cycle through different patterns
    while (true) {
        switch (pattern) {
            case 0:
                // Simple blink
                LED::on();
                Delay::ms(500);
                LED::off();
                Delay::ms(500);
                break;

            case 1:
                // Breathing effect
                breathing_effect();
                break;

            case 2:
                // Morse code SOS
                morse_sos();
                break;

            case 3:
                // Heartbeat
                heartbeat_pattern();
                break;

            case 4:
                // Fast blink
                LED::on();
                Delay::ms(100);
                LED::off();
                Delay::ms(100);
                break;
        }

        // Change pattern every ~5 seconds
        static uint32_t last_change = 0;
        if (Delay::tick_count - last_change > 5000) {
            pattern = (pattern + 1) % 5;
            last_change = Delay::tick_count;
        }
    }

    return 0;
}
