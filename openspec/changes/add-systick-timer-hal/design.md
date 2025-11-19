# Design: SysTick Timer HAL

## Architecture Overview

### High-Level Design
```
┌─────────────────────────────────────────────────┐
│           User Application Code                 │
│                                                  │
│  alloy::systick::micros()  ← Simple API         │
│  SystemTick::instance().micros()  ← OOP API     │
└───────────────────┬─────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────┐
│        src/hal/interface/systick.hpp            │
│  ┌──────────────────────────────────────────┐   │
│  │  SysTick Concept + Global Functions     │   │
│  │  - concept SystemTick<T>                 │   │
│  │  - namespace systick { micros(), ... }  │   │
│  └──────────────────────────────────────────┘   │
└───────────────────┬─────────────────────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
┌───────▼──────┐       ┌───────▼──────┐
│  STM32F1/F4  │       │    ESP32     │  ... (5 families)
│              │       │              │
│  Uses        │       │  Uses        │
│  SysTick     │       │  ESP Timer   │
│  Peripheral  │       │  Group 0     │
└──────────────┘       └──────────────┘
```

### Component Breakdown

#### 1. Interface Layer (`src/hal/interface/systick.hpp`)
**Purpose**: Define platform-agnostic SysTick interface

**Key Components**:
- `SystemTick<T>` concept - Defines required methods
- Global functions namespace - `alloy::systick::micros()`
- Configuration struct - `SysTickConfig`
- Helper functions - `micros_since()`, `is_timeout()`

**Design Decisions**:
- **Concept-based**: Compile-time polymorphism, zero runtime overhead
- **Dual API**: Both namespace (simple) and instance (testable)
- **Header-only**: No link-time dependencies, better optimization

#### 2. ARM Cortex-M Implementation (`src/hal/st/stm32f1/systick.hpp`, `stm32f4/systick.hpp`)
**Purpose**: Use ARM SysTick peripheral for time tracking

**Hardware**:
- 24-bit down-counter clocked at CPU frequency
- Generates exception on underflow
- Present in all ARM Cortex-M cores

**Implementation Strategy**:
```cpp
// SysTick configured to interrupt every 1us
// ISR increments 32-bit counter
volatile uint32_t systick_micros = 0;

void SysTick_Handler() {
    systick_micros++;  // Atomic on ARM Cortex-M
}
```

**Pros**: Dedicated peripheral, precise, no timer conflicts
**Cons**: 24-bit counter requires frequent interrupts at high CPU speeds

**Optimization**: Configure reload value based on CPU frequency to interrupt every ~1ms, track sub-millisecond in software.

#### 3. ESP32 Implementation (`src/hal/espressif/esp32/systick.hpp`)
**Purpose**: Use ESP32 high-resolution timer

**Hardware**:
- 64-bit hardware timer at 80MHz (APB clock)
- No SysTick peripheral (Xtensa architecture)
- Use Timer Group 0, Timer 0

**Implementation Strategy**:
```cpp
// Use hardware timer in auto-reload mode
// Read counter directly (no ISR needed for micros())
uint32_t micros() {
    return (uint32_t)(timer_get_counter_value(TIMER_GROUP_0, TIMER_0));
}
```

**Pros**: No interrupt overhead, hardware does everything
**Cons**: Uses one of limited hardware timers

#### 4. RP2040 Implementation (`src/hal/raspberrypi/rp2040/systick.hpp`)
**Purpose**: Use RP2040 64-bit timer

**Hardware**:
- 64-bit microsecond timer running at 1MHz
- Always running, no configuration needed
- Accessed via MMIO reads

**Implementation Strategy**:
```cpp
// Read lower 32 bits directly from hardware
uint32_t micros() {
    return *((volatile uint32_t*)0x40054028); // TIMELR register
}
```

**Pros**: Zero overhead, perfect 1MHz resolution
**Cons**: None - ideal implementation

#### 5. SAMD21 Implementation (`src/hal/microchip/samd21/systick.hpp`)
**Purpose**: Use TC (Timer Counter) peripheral

**Hardware**:
- Has ARM SysTick but we use TC3 for consistency
- 32-bit counter mode available
- Clocked from GCLK (48MHz typically)

**Implementation Strategy**:
```cpp
// Configure TC3 in 32-bit counter mode
// Prescaler /16 = 3MHz (333ns resolution)
// ISR increments software counter every ~1ms
volatile uint32_t systick_micros = 0;

void TC3_Handler() {
    systick_micros += 1000; // Increment by 1ms
    TC3->COUNT32.COUNT.reg;  // Read counter for sub-ms precision
}
```

**Pros**: Flexible, precise
**Cons**: Uses TC3, not available for other purposes

### Initialization Sequence

```cpp
// In Board::initialize()
Board::initialize() {
    // 1. Configure system clock (already done)
    clock.set_frequency(168'000'000);

    // 2. Initialize SysTick (new)
    SystemTick::init();  // Auto-configures based on CPU frequency

    // 3. Enable peripheral clocks
    clock.enable_peripheral(Peripheral::GpioA);
    // ...
}

// SysTick::init() implementation
void SystemTick::init() {
    // Get CPU frequency from clock singleton
    auto freq = SystemClock::instance().get_frequency();

    // Configure hardware timer for 1MHz ticks (1us resolution)
    configure_hardware_timer(freq);

    // Mark as initialized
    initialized_ = true;
}
```

### API Design

#### Global Namespace API (Recommended for Users)
```cpp
#include "hal/systick.hpp"

void my_function() {
    uint32_t start = alloy::systick::micros();

    do_something();

    uint32_t elapsed = alloy::systick::micros_since(start);

    if (alloy::systick::is_timeout(start, 1000)) {
        // Timeout after 1ms
    }
}
```

**Pros**: Simple, familiar (like Arduino), minimal typing
**Cons**: Global state, harder to mock for testing

#### Instance API (For Advanced Users / Testing)
```cpp
#include "hal/systick.hpp"

void my_function() {
    auto& timer = alloy::hal::SystemTick::instance();

    uint32_t start = timer.micros();
    do_something();
    uint32_t elapsed = timer.micros() - start;
}
```

**Pros**: Testable, no globals, OOP style
**Cons**: More verbose

#### Both APIs Delegate to Same Implementation
```cpp
namespace alloy::systick {
    inline uint32_t micros() {
        return hal::SystemTick::instance().micros();
    }
}
```

### Thread Safety

**32-bit Atomic Reads on ARM Cortex-M**:
- Single `LDR` instruction is naturally atomic
- No locking needed for reads
- ISR only writes, user code only reads

**Critical Section for Other Architectures**:
```cpp
uint32_t micros() {
    #ifdef CORTEX_M
        return systick_counter;  // Atomic
    #else
        __disable_irq();
        uint32_t value = systick_counter;
        __enable_irq();
        return value;
    #endif
}
```

### Overflow Handling

**Problem**: 32-bit microsecond counter overflows after ~71 minutes

**Solutions**:
1. **Document clearly**: Users must handle overflow in long-running apps
2. **Provide helper**: `micros_since()` handles wraparound
3. **Future RTOS**: Will reset counter periodically (tasks scheduled in <71min windows)

```cpp
// Handles wraparound correctly
uint32_t micros_since(uint32_t start) {
    uint32_t now = micros();
    // This works even if overflow occurred
    return now - start;  // Unsigned arithmetic wraps correctly
}

// Example: Timeout check with wraparound handling
bool is_timeout(uint32_t start, uint32_t timeout_us) {
    return (micros() - start) >= timeout_us;
}
```

### Memory Footprint

**Per MCU Family** (worst case):
- `systick_counter`: 4 bytes (shared volatile variable)
- `initialized_flag`: 1 byte (bool)
- **Total**: ~5 bytes RAM

**Code Size**: ~200-500 bytes per family (ISR + init function)

**When Not Used**: Linker eliminates all code (0 bytes)

### Performance

**Time to Read Counter**:
- ARM Cortex-M: 1-2 CPU cycles (single LDR)
- ESP32: ~10-20 cycles (MMIO read)
- RP2040: ~5-10 cycles (MMIO read)
- SAMD21: 1-2 CPU cycles (single LDR)

**Interrupt Overhead**:
- ARM SysTick: ~20-50 cycles per interrupt (depends on frequency)
- ESP32: None (read directly from hardware)
- RP2040: None (read directly from hardware)
- SAMD21: ~20-50 cycles per interrupt

**Optimization**: Interrupt as infrequently as possible while maintaining precision.

### Error Handling

**No Runtime Errors Possible**:
- Timer always initialized during `Board::initialize()`
- Hardware timer always available
- No configuration parameters to validate

**Compile-Time Safety**:
```cpp
static_assert(hal::SystemTick<STM32F4SystemTick>,
              "STM32F4SystemTick must implement SystemTick concept");
```

### Testing Strategy

1. **Unit Tests**: Mock timer, verify arithmetic (overflow, wraparound)
2. **Integration Tests**: Real hardware, verify microsecond accuracy
3. **Benchmark**: Measure overhead of `micros()` call
4. **Stress Test**: Run for >71 minutes, verify overflow handling

### Platform-Specific Notes

#### STM32F1 (72MHz)
- SysTick at 72MHz/72 = 1MHz
- Reload value: 1000 (1ms interrupts)
- Sub-millisecond from counter read

#### STM32F4 (168MHz)
- SysTick at 168MHz/168 = 1MHz
- Reload value: 1000 (1ms interrupts)
- Sub-millisecond from counter read

#### ESP32 (240MHz)
- Timer Group 0 at 80MHz (APB clock)
- Prescaler: /80 = 1MHz
- Direct 64-bit read, mask to 32-bit

#### RP2040 (125MHz)
- Hardware timer at 1MHz (perfect!)
- Direct 32-bit read from TIMELR
- Zero configuration needed

#### SAMD21 (48MHz)
- TC3 at 48MHz/16 = 3MHz
- Count to 3000 for 1ms interrupt
- Sub-millisecond from counter interpolation

### Future Extensions (Out of Scope)

For the future RTOS spec:
- Add `systick::register_tick_callback()` for scheduler
- Implement tickless idle mode (stop timer during sleep)
- Add 64-bit `micros64()` for long-running tasks
- Implement tick rate adjustment for power saving

### References
- ARM SysTick: Configured as 24-bit down counter, interrupts on reload
- ESP32: `esp_timer_get_time()` equivalent implementation
- RP2040: Section 4.6 of datasheet - timer is always running
- SAMD21: TC module in 32-bit mode with prescaler
