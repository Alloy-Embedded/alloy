# Spec: SysTick Interface

## ADDED Requirements

### Requirement: SysTick Concept Definition
**ID**: SYSTICK-INT-001
**Priority**: P0 (Critical)

The system SHALL provide a C++20 concept that defines the interface requirements for all SysTick implementations.

#### Scenario: Compile-time validation of SysTick implementation
```cpp
// Given a SysTick implementation for STM32F4
class STM32F4SystemTick {
public:
    static void init();
    static uint32_t micros();
    static void reset();
    static bool is_initialized();
};

// When validating against the concept
static_assert(alloy::hal::SystemTick<STM32F4SystemTick>,
              "STM32F4SystemTick must satisfy SystemTick concept");

// Then the static assertion passes
// And all required methods are verified at compile time
```

#### Scenario: Detect missing methods at compile time
```cpp
// Given an incomplete SysTick implementation
class IncompleteSysTick {
public:
    static uint32_t micros();  // Missing init(), reset(), is_initialized()
};

// When validating against the concept
static_assert(alloy::hal::SystemTick<IncompleteSysTick>,
              "IncompleteSysTick must satisfy SystemTick concept");

// Then compilation fails with clear error message
// And developer knows exactly which methods are missing
```

---

### Requirement: Global Namespace API
**ID**: SYSTICK-INT-002
**Priority**: P0 (Critical)

The system SHALL provide global namespace functions for simple, Arduino-style access to timing functions.

#### Scenario: Get current microseconds
```cpp
// Given SysTick is initialized
alloy::hal::SystemTick::init();

// When user calls micros()
uint32_t now = alloy::systick::micros();

// Then returns current time in microseconds
// And value increases monotonically (modulo overflow)
```

#### Scenario: Measure elapsed time with overflow handling
```cpp
// Given a start time near overflow
uint32_t start = 0xFFFFFF00;  // Near max uint32

// When some time passes and overflow occurs
delay_ms(1);
uint32_t elapsed = alloy::systick::micros_since(start);

// Then elapsed time is calculated correctly despite overflow
REQUIRE(elapsed > 0);
REQUIRE(elapsed < 2000);  // Approximately 1ms
```

#### Scenario: Check for timeout
```cpp
// Given a start time and timeout period
uint32_t start = alloy::systick::micros();
uint32_t timeout_us = 1000;  // 1ms timeout

// When checking if timeout occurred
bool timed_out = alloy::systick::is_timeout(start, timeout_us);

// Then returns false initially
REQUIRE(!timed_out);

// And returns true after timeout period
delay_ms(2);
timed_out = alloy::systick::is_timeout(start, timeout_us);
REQUIRE(timed_out);
```

---

### Requirement: Instance-Based API
**ID**: SYSTICK-INT-003
**Priority**: P1 (High)

The system SHALL provide an instance-based API for object-oriented access and testability.

#### Scenario: Access SysTick via singleton
```cpp
// Given SysTick is initialized
alloy::hal::SystemTick::init();

// When getting instance
auto& timer = alloy::hal::SystemTick::instance();

// Then returns valid singleton instance
// And can call methods
uint32_t now = timer.micros();
REQUIRE(now >= 0);
```

#### Scenario: Check initialization state
```cpp
// Given SysTick not yet initialized
// When checking if initialized
bool init = alloy::hal::SystemTick::is_initialized();

// Then returns false
REQUIRE(!init);

// And after initialization
alloy::hal::SystemTick::init();
init = alloy::hal::SystemTick::is_initialized();

// Then returns true
REQUIRE(init);
```

---

### Requirement: Thread-Safe Access
**ID**: SYSTICK-INT-004
**Priority**: P0 (Critical)

The system SHALL ensure thread-safe access to the microsecond counter.

#### Scenario: Atomic reads on ARM Cortex-M
```cpp
// Given SysTick running on ARM Cortex-M
// When reading micros() from ISR and main loop simultaneously
// ISR:
void some_isr() {
    uint32_t time = alloy::systick::micros();  // Read
}

// Main:
uint32_t time = alloy::systick::micros();  // Concurrent read

// Then both reads return consistent values
// And no data corruption occurs
// And no locking overhead (single LDR instruction is atomic)
```

#### Scenario: Protected reads on non-ARM platforms
```cpp
// Given SysTick running on platform without atomic 32-bit reads
// When reading micros()
uint32_t time = alloy::systick::micros();

// Then critical section protects the read
// And interrupts are disabled during read
// And interrupts are restored after read
```

---

### Requirement: Overflow Handling
**ID**: SYSTICK-INT-005
**Priority**: P1 (High)

The system SHALL handle 32-bit counter overflow correctly using unsigned arithmetic wraparound.

#### Scenario: micros_since() handles wraparound
```cpp
// Given counter near maximum
uint32_t start = 0xFFFFFFF0;  // 16us before overflow

// When time passes through overflow
delay_us(100);  // Counter wraps to ~84

uint32_t elapsed = alloy::systick::micros_since(start);

// Then elapsed time is correct despite wrap
REQUIRE(elapsed >= 90);
REQUIRE(elapsed <= 110);
```

#### Scenario: is_timeout() works after overflow
```cpp
// Given start time before overflow
uint32_t start = 0xFFFFFF00;

// When checking timeout after overflow
delay_us(2000);
bool timed_out = alloy::systick::is_timeout(start, 1000);

// Then correctly detects timeout
REQUIRE(timed_out);
```

---

### Requirement: Zero-Cost Abstraction
**ID**: SYSTICK-INT-006
**Priority**: P1 (High)

The system SHALL implement SysTick as a zero-cost abstraction with no runtime overhead when not used.

#### Scenario: Unused SysTick code eliminated
```cpp
// Given application that doesn't use SysTick
int main() {
    Board::initialize();  // SysTick auto-inits but unused
    blink_led();
    return 0;
}

// When linking final binary
// Then linker eliminates unused SysTick code
// And binary size not affected by SysTick presence
```

#### Scenario: Inline expansion of hot path
```cpp
// Given hot loop reading time
for (int i = 0; i < 1000; i++) {
    uint32_t time = alloy::systick::micros();
    process(time);
}

// When compiler optimizes
// Then micros() is inlined (no function call overhead)
// And assembly shows direct memory load
// And no function call overhead
```

---

### Requirement: Auto-Initialization
**ID**: SYSTICK-INT-007
**Priority**: P0 (Critical)

The system SHALL auto-initialize SysTick during Board::initialize() with sensible defaults.

#### Scenario: SysTick ready after Board::initialize()
```cpp
// Given fresh boot
int main() {
    // When initializing board
    Board::initialize();

    // Then SysTick is ready to use
    uint32_t time = alloy::systick::micros();
    REQUIRE(time >= 0);

    // And is_initialized() returns true
    REQUIRE(alloy::hal::SystemTick::is_initialized());
}
```

#### Scenario: SysTick reconfigurable after init
```cpp
// Given SysTick auto-initialized
Board::initialize();

// When user reconfigures
alloy::hal::SystemTick::instance().reset();
alloy::hal::SystemTick::init();  // Re-init with custom config

// Then reconfiguration succeeds
// And timer continues working
uint32_t time = alloy::systick::micros();
REQUIRE(time >= 0);
```

---

### Requirement: Platform Independence
**ID**: SYSTICK-INT-008
**Priority**: P0 (Critical)

The system SHALL provide identical API across all platforms, hiding hardware differences.

#### Scenario: Same code works on STM32 and ESP32
```cpp
// Given application code
void measure_performance() {
    uint32_t start = alloy::systick::micros();
    do_work();
    uint32_t elapsed = alloy::systick::micros_since(start);
    log("Work took %u us", elapsed);
}

// When compiled for STM32F4
// Then uses ARM SysTick peripheral

// When compiled for ESP32
// Then uses ESP32 timer hardware

// And behavior is identical on both platforms
// And API doesn't change
```

---

### Requirement: Documentation
**ID**: SYSTICK-INT-009
**Priority**: P2 (Medium)

The system SHALL provide comprehensive documentation with usage examples and best practices.

#### Scenario: Quick start example in docs
```cpp
// Example in documentation:
#include "board.hpp"

int main() {
    Board::initialize();  // SysTick auto-initialized

    uint32_t start = alloy::systick::micros();

    // Your code here
    delay_ms(10);

    uint32_t elapsed = alloy::systick::micros_since(start);
    // elapsed ≈ 10000 microseconds
}
```

#### Scenario: Overflow handling documented
```markdown
# Documentation excerpt:
## Overflow Handling

The 32-bit microsecond counter overflows after approximately 71 minutes.
For long-running applications, use `micros_since()` which handles wraparound:

```cpp
uint32_t start = alloy::systick::micros();

// Even if counter overflows during this time
long_running_operation();

// This correctly calculates elapsed time
uint32_t elapsed = alloy::systick::micros_since(start);
```

For applications running longer than 71 minutes, implement periodic resets
or use the future RTOS scheduler which handles this automatically.
```

## MODIFIED Requirements
None - This is a new feature.

## REMOVED Requirements
None - This is a new feature.
