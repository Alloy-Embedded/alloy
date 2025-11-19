# Spec: SysTick Implementations

## ADDED Requirements

### Requirement: STM32F1 SysTick Implementation
**ID**: SYSTICK-IMPL-001
**Priority**: P0 (Critical)

The system SHALL implement SysTick for STM32F1 using the ARM Cortex-M3 SysTick peripheral.

#### Scenario: Initialize SysTick at 1MHz tick rate
```cpp
// Given STM32F1 running at 72MHz
Board::initialize();  // Configures clock to 72MHz

// When SysTick initializes
alloy::hal::st::stm32f1::SystemTick::init();

// Then SysTick configured with:
// - Clock source: CPU clock / 8 = 9MHz
// - Reload value: 9 (for 1MHz tick)
// - Interrupt enabled
// - Counter starts at 0

uint32_t time = alloy::systick::micros();
REQUIRE(time == 0);
```

#### Scenario: Microsecond precision timing
```cpp
// Given SysTick initialized on STM32F1
Board::initialize();

// When measuring 10ms delay
uint32_t start = alloy::systick::micros();
delay_ms(10);
uint32_t elapsed = alloy::systick::micros_since(start);

// Then elapsed time is accurate within 5%
REQUIRE(elapsed >= 9500);  // -5%
REQUIRE(elapsed <= 10500); // +5%
```

---

### Requirement: STM32F4 SysTick Implementation
**ID**: SYSTICK-IMPL-002
**Priority**: P0 (Critical)

The system SHALL implement SysTick for STM32F4 using the ARM Cortex-M4F SysTick peripheral.

#### Scenario: Initialize SysTick at 168MHz CPU frequency
```cpp
// Given STM32F4 running at 168MHz
Board::initialize();  // Configures clock to 168MHz

// When SysTick initializes
alloy::hal::st::stm32f4::SystemTick::init();

// Then SysTick configured with:
// - Clock source: CPU clock / 8 = 21MHz
// - Reload value: 21 (for 1MHz tick)
// - Interrupt enabled
// - Running

REQUIRE(alloy::hal::SystemTick::is_initialized());
```

#### Scenario: Sub-millisecond precision using counter interpolation
```cpp
// Given SysTick running at 1ms interrupt rate
Board::initialize();

// When reading time between interrupts
uint32_t t1 = alloy::systick::micros();
// ~100us passes
delay_cycles(16800);  // 168MHz * 0.0001s = 16800 cycles
uint32_t t2 = alloy::systick::micros();

// Then difference shows sub-millisecond precision
uint32_t diff = t2 - t1;
REQUIRE(diff >= 90);   // ~100us ±10%
REQUIRE(diff <= 110);
```

---

### Requirement: ESP32 SysTick Implementation
**ID**: SYSTICK-IMPL-003
**Priority**: P0 (Critical)

The system SHALL implement SysTick for ESP32 using Timer Group 0, Timer 0.

#### Scenario: Initialize ESP32 timer for microsecond counting
```cpp
// Given ESP32 running at 240MHz
Board::initialize();  // Configures clock to 240MHz

// When SysTick initializes
alloy::hal::espressif::esp32::SystemTick::init();

// Then Timer Group 0, Timer 0 configured:
// - Base clock: 80MHz (APB clock, independent of CPU)
// - Prescaler: 80 (for 1MHz tick rate)
// - Mode: Free-running up-counter
// - 64-bit counter
// - No interrupts (direct read)

REQUIRE(alloy::hal::SystemTick::is_initialized());
```

#### Scenario: Zero interrupt overhead on ESP32
```cpp
// Given SysTick initialized
Board::initialize();

// When reading micros() repeatedly
for (int i = 0; i < 1000; i++) {
    volatile uint32_t time = alloy::systick::micros();
}

// Then no SysTick interrupts occur
// And timer is read directly from hardware
// And lower 32 bits of 64-bit counter are returned
```

#### Scenario: ESP32 timer survives CPU frequency changes
```cpp
// Given SysTick running at 240MHz CPU
Board::initialize();
uint32_t start = alloy::systick::micros();

// When CPU frequency changes (power management)
// (hypothetical, not testing actual freq change)

// Then timer continues at same rate
// Because timer uses APB clock (80MHz fixed), not CPU clock
delay_ms(10);
uint32_t elapsed = alloy::systick::micros_since(start);
REQUIRE(elapsed >= 9500);
REQUIRE(elapsed <= 10500);
```

---

### Requirement: RP2040 SysTick Implementation
**ID**: SYSTICK-IMPL-004
**Priority**: P0 (Critical)

The system SHALL implement SysTick for RP2040 using the hardware microsecond timer.

#### Scenario: Use RP2040's built-in microsecond timer
```cpp
// Given RP2040 running at 125MHz
Board::initialize();

// When SysTick initializes
alloy::hal::raspberrypi::rp2040::SystemTick::init();

// Then uses hardware timer:
// - Timer peripheral address: 0x40054000
// - TIMELR register: 0x40054028 (lower 32 bits)
// - Already running at 1MHz
// - No configuration needed
// - Zero interrupt overhead

REQUIRE(alloy::hal::SystemTick::is_initialized());
```

#### Scenario: Direct hardware read with zero overhead
```cpp
// Given RP2040 SysTick initialized
Board::initialize();

// When reading micros()
uint32_t time = alloy::systick::micros();

// Then implementation is just:
// return *((volatile uint32_t*)0x40054028);

// And takes ~3-5 CPU cycles
// And no function call overhead (inline)
// And no interrupts
```

#### Scenario: Perfect 1MHz resolution on RP2040
```cpp
// Given RP2040 SysTick running
Board::initialize();

// When measuring exactly 1000 cycles at 1MHz
uint32_t start = alloy::systick::micros();
// Wait exactly 1000us using timer
while (alloy::systick::micros_since(start) < 1000);
uint32_t elapsed = alloy::systick::micros_since(start);

// Then elapsed is exactly 1000 (or 1001 due to loop overhead)
REQUIRE(elapsed >= 1000);
REQUIRE(elapsed <= 1001);
```

---

### Requirement: SAMD21 SysTick Implementation
**ID**: SYSTICK-IMPL-005
**Priority**: P0 (Critical)

The system SHALL implement SysTick for SAMD21 using TC3 (Timer Counter 3) peripheral.

#### Scenario: Configure TC3 for microsecond timing
```cpp
// Given SAMD21 running at 48MHz
Board::initialize();

// When SysTick initializes
alloy::hal::microchip::samd21::SystemTick::init();

// Then TC3 configured:
// - Mode: 32-bit counter
// - Clock: GCLK0 (48MHz)
// - Prescaler: DIV16 (3MHz tick rate)
// - Match/Compare: 3000 (1ms interrupt)
// - Interrupt enabled

REQUIRE(alloy::hal::SystemTick::is_initialized());
```

#### Scenario: Sub-millisecond precision via counter interpolation
```cpp
// Given TC3 interrupting every 1ms
Board::initialize();

// When reading time with sub-ms precision
uint32_t t1 = alloy::systick::micros();
delay_us(500);  // Half a millisecond
uint32_t t2 = alloy::systick::micros();

// Then difference shows microsecond precision
uint32_t diff = t2 - t1;
REQUIRE(diff >= 450);  // ±10%
REQUIRE(diff <= 550);

// And implementation reads:
// - Software counter (milliseconds)
// - Hardware TC3 count register (sub-millisecond)
// - Combines: (ms_counter * 1000) + (tc3_count / 3)
```

---

### Requirement: Consistent Initialization Order
**ID**: SYSTICK-IMPL-006
**Priority**: P0 (Critical)

The system SHALL initialize SysTick in Board::initialize() after clock configuration and before peripheral clock enables.

#### Scenario: Initialization order in Board::initialize()
```cpp
void Board::initialize() {
    // 1. Configure system clock (MUST happen first)
    static SystemClock clock;
    clock.set_frequency(168'000'000);

    // 2. Initialize SysTick (MUST happen after clock, before peripherals)
    SystemTick::init();

    // 3. Enable peripheral clocks (can use SysTick for delays now)
    clock.enable_peripheral(Peripheral::GpioA);
    // ...
}

// Then SysTick available for use immediately after Board::initialize()
```

---

### Requirement: Hardware Resource Management
**ID**: SYSTICK-IMPL-007
**Priority**: P1 (High)

The system SHALL document which hardware resources are used by SysTick and mark them as reserved.

#### Scenario: Document reserved timer resources
```cpp
// In each implementation's header:

/// STM32F1/F4 SysTick Implementation
///
/// **Hardware Resources Used**:
/// - ARM Cortex-M SysTick peripheral (exclusive use)
/// - SysTick IRQ (vector 15)
///
/// **Not Available for Other Use**:
/// - SysTick peripheral cannot be used for anything else
///
/// @note STM32 general-purpose timers (TIM1-TIM14) remain available

// ESP32:
/// **Hardware Resources Used**:
/// - Timer Group 0, Timer 0
///
/// **Not Available for Other Use**:
/// - TG0_T0_LEVEL_INT interrupt
///
/// @note TG0_T1, TG1_T0, TG1_T1 remain available

// RP2040:
/// **Hardware Resources Used**:
/// - Built-in microsecond timer (read-only)
///
/// **Not Available for Other Use**:
/// - None (timer is read-only, doesn't conflict with other uses)
///
/// @note All 4 RP2040 PWM timers remain available

// SAMD21:
/// **Hardware Resources Used**:
/// - TC3 (Timer Counter 3)
/// - TC3 overflow interrupt
///
/// **Not Available for Other Use**:
/// - TC3 cannot be used for PWM or other timing
///
/// @note TC0, TC1, TC2, TC4, TC5 remain available
```

---

### Requirement: Power Consumption Awareness
**ID**: SYSTICK-IMPL-008
**Priority**: P2 (Medium)

The system SHALL minimize power consumption of SysTick by reducing interrupt frequency where possible.

#### Scenario: Minimize interrupt frequency on ARM platforms
```cpp
// Given SysTick needs 1MHz resolution
// When configuring ARM SysTick
SystemTick::init();

// Then interrupt configured for 1ms (not 1us)
// - Interrupt every 1ms: 1000 interrupts/sec
// - Software counter incremented in ISR
// - Sub-millisecond from counter read
// - 99.9% fewer interrupts than naïve approach

// Naïve: 1MHz interrupt = 1,000,000 interrupts/sec (bad!)
// Optimized: 1kHz interrupt = 1,000 interrupts/sec (good!)
```

#### Scenario: Zero interrupts on platforms with good hardware
```cpp
// Given ESP32 or RP2040
// When reading time
uint32_t time = alloy::systick::micros();

// Then zero interrupts used
// And timer read directly from hardware
// And minimal power consumption
```

---

### Requirement: Accuracy Specification
**ID**: SYSTICK-IMPL-009
**Priority**: P1 (High)

The system SHALL maintain timing accuracy within ±5% over short intervals (<1 second).

#### Scenario: 1ms delay accuracy across all platforms
```cpp
// Given any platform
Board::initialize();

// When measuring 1ms delay 100 times
std::array<uint32_t, 100> measurements;
for (int i = 0; i < 100; i++) {
    uint32_t start = alloy::systick::micros();
    delay_ms(1);
    measurements[i] = alloy::systick::micros_since(start);
}

// Then all measurements within ±5% of 1000us
for (uint32_t m : measurements) {
    REQUIRE(m >= 950);   // -5%
    REQUIRE(m <= 1050);  // +5%
}

// And average is very close to 1000us
uint32_t avg = std::accumulate(measurements.begin(), measurements.end(), 0u) / 100;
REQUIRE(avg >= 980);
REQUIRE(avg <= 1020);
```

---

### Requirement: Interrupt Priority Configuration
**ID**: SYSTICK-IMPL-010
**Priority**: P2 (Medium)

The system SHALL configure SysTick interrupt at appropriate priority for RTOS compatibility.

#### Scenario: SysTick at lower priority than critical ISRs
```cpp
// Given SysTick initialization
SystemTick::init();

// Then SysTick IRQ configured:
// - Priority: 15 (lowest on 4-bit priority systems)
// - Can be preempted by higher-priority interrupts
// - Compatible with future RTOS context switching

// And critical interrupts (UART, SPI, etc.) can preempt SysTick
// And SysTick ISR is fast (just increment counter)
```

## MODIFIED Requirements
None - This is a new feature.

## REMOVED Requirements
None - This is a new feature.
