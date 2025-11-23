# Spec: Abstraction Tier System

## ADDED Requirements

### Requirement: Three abstraction tiers consistently implemented
All peripherals MUST offer Simple, Fluent, and Expert tier APIs.

#### Scenario: GPIO abstraction tiers
```cpp
// ========== SIMPLE TIER ==========
// For beginners, minimal configuration
namespace ucore::simple {
    auto led = gpio::output(Pin::PA5);
    led.on();
    led.off();
    led.toggle();
}

// ========== FLUENT TIER ==========
// For productivity, method chaining
namespace ucore::fluent {
    Gpio::configure(Pin::PA5)
        .as_output()
        .push_pull()
        .high_speed()
        .initial_low()
        .build()
        .set_high();
}

// ========== EXPERT TIER ==========
// For performance, direct policy access
namespace ucore::expert {
    using LedPolicy = GpioHardwarePolicy<
        Port::A, 5,
        OutputMode::PushPull,
        Speed::High
    >;

    LedPolicy::set_high();  // Single instruction
    LedPolicy::read();      // Direct register read
}
```

**Expected**: Consistent pattern across all peripherals
**Rationale**: Different use cases, same philosophy

#### Scenario: Choose tier for use case
```cpp
// Beginner learning embedded: Use Simple
simple::gpio::output(Pin::PA5).on();

// Application developer: Use Fluent
fluent::Uart::configure(Uart1)
    .baudrate(115200)
    .parity(None)
    .flow_control(None)
    .build()
    .write("Hello");

// Library developer: Use Expert
expert::SpiPolicy<Spi1, Mode0>::transfer(0x42);
```

**Expected**: Clear progression path for users
**Rationale**: Grow with the library

### Requirement: Tier selection guide in documentation
Documentation MUST explain when to use each tier.

#### Scenario: Developer reads tier guide
```markdown
# Choosing an Abstraction Tier

## Simple Tier
**Use when:**
- Learning embedded programming
- Prototyping quickly
- Configuration doesn't change

**Characteristics:**
- Minimal syntax
- Runtime initialization
- Easiest to understand
- Slight overhead (~2-3 instructions)

**Example:** Blink LED tutorial

## Fluent Tier
**Use when:**
- Writing application code
- Need readable configuration
- Moderate performance requirements

**Characteristics:**
- Chainable methods
- Compile-time optimization
- Balance of clarity and performance
- Zero overhead after initialization

**Example:** UART communication, sensor drivers

## Expert Tier
**Use when:**
- Writing HAL implementations
- Maximum performance critical
- Every cycle counts (ISR, bit-banging)

**Characteristics:**
- Direct hardware policy access
- Fully inlined, single instruction
- Requires understanding of hardware
- Zero overhead, zero abstraction cost

**Example:** High-frequency PWM, protocol bit-banging
```

**Expected**: Users make informed choices
**Rationale**: Right tool for the job

### Requirement: All tiers achieve zero-overhead
Simple tier MAY have initialization overhead, but no runtime overhead for operations.

#### Scenario: Verify zero-overhead in disassembly
```cpp
// Simple tier
simple::gpio::output(Pin::PA5).on();

// Compiles to same assembly as Expert tier:
//   str r1, [r0, #24]    ; GPIOA->BSRR = (1 << 5)
//   bx lr                ; return
```

**Expected**: Identical assembly for runtime operations
**Rationale**: Philosophy of zero-overhead abstractions

#### Scenario: Benchmark tier performance
```cpp
// tools/benchmarks/tier_comparison.cpp
void benchmark_simple() {
    auto led = simple::gpio::output(Pin::PA5);
    for(int i = 0; i < 1000000; i++) {
        led.toggle();
    }
}

void benchmark_expert() {
    for(int i = 0; i < 1000000; i++) {
        expert::GpioPolicy<PortA, 5>::toggle();
    }
}

// Results:
// Simple:  1.234 MHz toggle rate
// Expert:  1.234 MHz toggle rate  (identical!)
```

**Expected**: Performance parity for runtime operations
**Rationale**: Validates zero-overhead claim

## MODIFIED Requirements

### Requirement: Examples demonstrate tier usage
Extend existing examples MUST show different tiers.

#### Scenario: Blink example uses Simple tier
```cpp
// examples/blink_simple/main.cpp
#include "ucore/simple/gpio.hpp"

int main() {
    using namespace ucore::simple;

    auto led = gpio::output(board::led_pin);

    while(1) {
        led.toggle();
        delay_ms(500);
    }
}
```

**Expected**: Beginner-friendly entry point
**Rationale**: Approachable first example

#### Scenario: Advanced example uses Expert tier
```cpp
// examples/ws2812_driver/main.cpp
#include "ucore/expert/gpio.hpp"

using DataPin = expert::GpioPolicy<PortA, 5, ...>;

void send_bit(bool bit) {
    // Timing-critical: use Expert tier
    DataPin::set_high();
    if(bit) __delay_cycles(8);  // 800ns
    else    __delay_cycles(4);  // 400ns
    DataPin::set_low();
}
```

**Expected**: Demonstrates Expert tier necessity
**Rationale**: Shows when each tier is appropriate

## REMOVED Requirements

None. This adds new tier system to existing APIs.
