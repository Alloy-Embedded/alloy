# Timer Usage Guide

**Version**: 1.0
**Date**: 2025-01-21
**Status**: Complete

---

## Table of Contents

1. [Overview](#overview)
2. [SysTick Timer](#systick-timer)
3. [Basic Delays](#basic-delays)
4. [Non-Blocking Timing](#non-blocking-timing)
5. [Hardware Timers](#hardware-timers)
6. [RTOS Integration](#rtos-integration)
7. [Best Practices](#best-practices)
8. [Performance Considerations](#performance-considerations)
9. [Troubleshooting](#troubleshooting)

---

## Overview

Alloy provides multiple timing mechanisms for embedded systems:

### Timing Options

| Type | Use Case | Accuracy | Overhead | RTOS-Safe |
|------|----------|----------|----------|-----------|
| **SysTick** | Millisecond delays | ±1% | Low | Yes |
| **SysTick (us)** | Microsecond delays | ±5% | Low | Careful |
| **Hardware Timers** | PWM, capture, compare | Excellent | Medium | Yes |
| **RTOS Delays** | Task delays | Good | Low | Yes |

### Key Features

- **Zero-cost abstraction**: All timing compiled away at build time
- **Board-portable**: Use `board::BoardSysTick` for portability
- **Type-safe**: Compile-time clock frequency validation
- **RTOS-compatible**: Integrates with FreeRTOS, ThreadX, etc.

---

## SysTick Timer

The ARM Cortex-M SysTick timer provides system-wide timing services.

### Quick Start

```cpp
#include "board/board.hpp"
#include "hal/api/systick_simple.hpp"

using namespace alloy::hal;

int main() {
    board::init();  // Initializes SysTick automatically

    // 1-second delay
    SysTickTimer::delay_ms<board::BoardSysTick>(1000);

    // Get current time
    u32 now = SysTickTimer::millis<board::BoardSysTick>();
}
```

### SysTick API

```cpp
class SysTickTimer {
public:
    // Blocking delays
    static void delay_ms<Policy>(u32 milliseconds);
    static void delay_us<Policy>(u32 microseconds);

    // Get current time
    static u32 millis<Policy>();      // Milliseconds since startup
    static u32 micros<Policy>();      // Microseconds since startup

    // Configuration
    static Result<void, ErrorCode> initialize<Policy>(u32 tick_rate_hz);
};
```

### Configuration

**Standard 1ms Tick (RTOS-compatible)**:
```cpp
// In board initialization
SysTickTimer::initialize<board::BoardSysTick>(1000);  // 1000 Hz = 1ms tick
```

**High-Resolution 100us Tick**:
```cpp
// For precise timing (higher CPU overhead)
SysTickTimer::initialize<board::BoardSysTick>(10000);  // 10000 Hz = 100us tick
```

---

## Basic Delays

### Millisecond Delays

**Simple Delay**:
```cpp
// Delay for 1 second
SysTickTimer::delay_ms<board::BoardSysTick>(1000);
```

**LED Blink Pattern**:
```cpp
void blink_led(u32 on_time_ms, u32 off_time_ms, u32 count) {
    for (u32 i = 0; i < count; i++) {
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(on_time_ms);

        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(off_time_ms);
    }
}

// Usage
blink_led(500, 500, 10);  // Blink 10 times, 500ms each
```

### Microsecond Delays

**Short Pulse Generation**:
```cpp
void generate_pulse_us(u32 duration_us) {
    board::pin::set_high();
    SysTickTimer::delay_us<board::BoardSysTick>(duration_us);
    board::pin::set_low();
}

// 10us pulse for DHT22 sensor
generate_pulse_us(10);
```

**Protocol Timing**:
```cpp
void ws2812_send_bit(bool bit) {
    board::data_pin::set_high();

    if (bit) {
        SysTickTimer::delay_us<board::BoardSysTick>(800);  // T1H: 800ns
        board::data_pin::set_low();
        SysTickTimer::delay_us<board::BoardSysTick>(450);  // T1L: 450ns
    } else {
        SysTickTimer::delay_us<board::BoardSysTick>(400);  // T0H: 400ns
        board::data_pin::set_low();
        SysTickTimer::delay_us<board::BoardSysTick>(850);  // T0L: 850ns
    }
}
```

### Limitations

⚠️ **Blocking delays**:
- CPU is idle during delay
- No other code executes
- Not suitable for RTOS tasks (use `vTaskDelay` instead)

⚠️ **Accuracy**:
- Millisecond delays: ±1% typical
- Microsecond delays: ±5% typical
- Affected by interrupts and CPU speed

---

## Non-Blocking Timing

For responsive applications, use non-blocking timing patterns.

### Timeout Pattern

```cpp
class Timeout {
public:
    Timeout(u32 timeout_ms)
        : start_time_(SysTickTimer::millis<board::BoardSysTick>())
        , timeout_ms_(timeout_ms) {}

    bool expired() const {
        u32 now = SysTickTimer::millis<board::BoardSysTick>();
        return (now - start_time_) >= timeout_ms_;
    }

    u32 remaining_ms() const {
        u32 now = SysTickTimer::millis<board::BoardSysTick>();
        u32 elapsed = now - start_time_;
        return (elapsed < timeout_ms_) ? (timeout_ms_ - elapsed) : 0;
    }

private:
    u32 start_time_;
    u32 timeout_ms_;
};

// Usage
Result<u8, ErrorCode> wait_for_uart_byte() {
    Timeout timeout(1000);  // 1 second timeout

    while (!timeout.expired()) {
        if (uart::is_rx_ready()) {
            return uart::read();
        }
    }

    return Err(ErrorCode::TIMEOUT);
}
```

### Periodic Task Pattern

```cpp
class PeriodicTask {
public:
    PeriodicTask(u32 period_ms)
        : period_ms_(period_ms)
        , last_run_(SysTickTimer::millis<board::BoardSysTick>()) {}

    bool should_run() {
        u32 now = SysTickTimer::millis<board::BoardSysTick>();
        if ((now - last_run_) >= period_ms_) {
            last_run_ = now;
            return true;
        }
        return false;
    }

private:
    u32 period_ms_;
    u32 last_run_;
};

// Usage
PeriodicTask sensor_read(100);  // Every 100ms
PeriodicTask led_blink(500);    // Every 500ms
PeriodicTask log_output(1000);  // Every 1 second

void main_loop() {
    while (true) {
        if (sensor_read.should_run()) {
            read_sensors();
        }

        if (led_blink.should_run()) {
            toggle_led();
        }

        if (log_output.should_run()) {
            print_status();
        }
    }
}
```

### Debounce Pattern

```cpp
class Button {
public:
    Button(u32 debounce_ms = 50)
        : debounce_ms_(debounce_ms)
        , last_change_(0)
        , state_(false) {}

    bool read() {
        bool current = board::button::is_pressed();
        u32 now = SysTickTimer::millis<board::BoardSysTick>();

        if (current != state_) {
            if ((now - last_change_) >= debounce_ms_) {
                state_ = current;
                last_change_ = now;
            }
        }

        return state_;
    }

private:
    u32 debounce_ms_;
    u32 last_change_;
    bool state_;
};

// Usage
Button button(50);  // 50ms debounce

void main_loop() {
    while (true) {
        if (button.read()) {
            handle_button_press();
        }
    }
}
```

---

## Hardware Timers

Hardware timers provide advanced features like PWM, input capture, and output compare.

### Timer Overview

| Feature | STM32 | SAME70 | Use Case |
|---------|-------|---------|----------|
| **Basic Timer** | TIM6, TIM7 | TC | Simple timing |
| **General-Purpose** | TIM2-TIM5 | TC | PWM, capture |
| **Advanced** | TIM1, TIM8 | PWM | Motor control |

### PWM Generation

```cpp
// Note: Hardware timer templates coming soon
// For now, use direct register access

#include "hal/vendors/st/stm32f4/generated/registers/tim2_registers.hpp"

void setup_pwm(u32 frequency_hz, u8 duty_percent) {
    using namespace alloy::hal::st::stm32f4;

    // Enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Configure timer for PWM
    auto* tim2 = reinterpret_cast<TIM2_TypeDef*>(0x40000000);

    // Set prescaler for desired frequency
    u32 timer_clock = 84000000;  // 84 MHz
    u32 prescaler = (timer_clock / (frequency_hz * 65536)) + 1;
    tim2->PSC = prescaler - 1;

    // Set auto-reload for period
    u32 period = (timer_clock / (prescaler * frequency_hz)) - 1;
    tim2->ARR = period;

    // Set duty cycle
    tim2->CCR1 = (period * duty_percent) / 100;

    // Configure PWM mode 1 on channel 1
    tim2->CCMR1 = (0x6 << 4);  // PWM mode 1
    tim2->CCER = (1 << 0);     // Enable output

    // Start timer
    tim2->CR1 |= (1 << 0);  // Enable counter
}

// Generate 1kHz PWM at 50% duty
setup_pwm(1000, 50);
```

### Input Capture

```cpp
// Measure pulse width
u32 measure_pulse_width() {
    // Configure TIM2 channel 1 for input capture
    // Capture on rising edge
    TIM2->CCMR1 = (0x1 << 0);   // CC1 as input, IC1 mapped to TI1
    TIM2->CCER = (1 << 0);      // Capture on rising edge
    TIM2->CR1 |= (1 << 0);      // Enable counter

    // Wait for capture
    while (!(TIM2->SR & (1 << 1)));  // Wait for CC1IF
    u32 start = TIM2->CCR1;
    TIM2->SR &= ~(1 << 1);  // Clear flag

    // Wait for second capture (falling edge)
    TIM2->CCER = (1 << 1);  // Capture on falling edge
    while (!(TIM2->SR & (1 << 1)));
    u32 end = TIM2->CCR1;

    return end - start;  // Pulse width in timer ticks
}
```

---

## RTOS Integration

### Tick Hook

```cpp
// In your board initialization
extern "C" void SysTick_Handler() {
    // Increment Alloy tick counter
    alloy::hal::SysTickTimer::tick<board::BoardSysTick>();

    // Call RTOS tick handler
    #if USE_FREERTOS
        xPortSysTickHandler();
    #elif USE_THREADX
        _tx_timer_interrupt();
    #endif
}
```

### RTOS-Safe Delays

⚠️ **Never use blocking delays in RTOS tasks**:

```cpp
// ❌ WRONG - Blocks entire system
void sensor_task(void* param) {
    while (true) {
        read_sensor();
        SysTickTimer::delay_ms<board::BoardSysTick>(100);  // BAD!
    }
}

// ✅ CORRECT - Use RTOS delay
void sensor_task(void* param) {
    while (true) {
        read_sensor();
        vTaskDelay(pdMS_TO_TICKS(100));  // GOOD!
    }
}
```

### Timeout in RTOS

```cpp
// Use RTOS timers for timeouts
Result<u8, ErrorCode> wait_for_uart_rtos() {
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(1000);

    while ((xTaskGetTickCount() - start) < timeout_ticks) {
        if (uart::is_rx_ready()) {
            return uart::read();
        }
        vTaskDelay(1);  // Yield to other tasks
    }

    return Err(ErrorCode::TIMEOUT);
}
```

---

## Best Practices

### DO ✅

1. **Use board-portable timing**:
```cpp
// Good - portable
SysTickTimer::delay_ms<board::BoardSysTick>(100);

// Bad - platform-specific
SysTickTimer::delay_ms<STM32F4SysTick>(100);
```

2. **Prefer non-blocking patterns**:
```cpp
// Good - responsive
if (timeout.expired()) {
    return Err(ErrorCode::TIMEOUT);
}

// Bad - blocks everything
SysTickTimer::delay_ms<board::BoardSysTick>(1000);
if (!ready()) {
    return Err(ErrorCode::TIMEOUT);
}
```

3. **Validate timing accuracy**:
```cpp
u32 start = SysTickTimer::millis<board::BoardSysTick>();
operation();
u32 elapsed = SysTickTimer::millis<board::BoardSysTick>() - start;
// Log or assert elapsed time
```

### DON'T ❌

1. **Don't use delays in ISRs**:
```cpp
// ❌ NEVER do this
extern "C" void UART_IRQHandler() {
    SysTickTimer::delay_ms<board::BoardSysTick>(10);  // DISASTER!
}
```

2. **Don't assume exact timing**:
```cpp
// ❌ Assumes 1000ms is exact
for (int i = 0; i < 60; i++) {
    SysTickTimer::delay_ms<board::BoardSysTick>(1000);
}
// This is NOT guaranteed to be 60 seconds!

// ✅ Calculate total elapsed time
u32 start = SysTickTimer::millis<board::BoardSysTick>();
while ((SysTickTimer::millis<board::BoardSysTick>() - start) < 60000) {
    // Do work
}
```

3. **Don't ignore overflow**:
```cpp
// ❌ Overflow after 49 days at 1ms tick
u32 now = SysTickTimer::millis<board::BoardSysTick>();
if (now > last_time) {  // Fails on overflow!
    // ...
}

// ✅ Handles overflow correctly
u32 now = SysTickTimer::millis<board::BoardSysTick>();
if ((now - last_time) > threshold) {  // Works with overflow!
    // ...
}
```

---

## Performance Considerations

### Tick Rate Trade-offs

| Tick Rate | Resolution | Overhead | Use Case |
|-----------|-----------|----------|----------|
| 100 Hz | 10ms | 0.1% | Low-power applications |
| 1000 Hz | 1ms | 1% | **Standard (RTOS)** |
| 10000 Hz | 100us | 10% | High-precision timing |

### Interrupt Overhead

**1ms tick @ 72 MHz**:
- ISR execution: ~20 cycles = 0.28us
- Overhead: 0.028% CPU time
- **Acceptable for most applications**

**100us tick @ 72 MHz**:
- ISR execution: ~20 cycles = 0.28us
- Overhead: 0.28% CPU time
- **Use only if needed**

### Delay Accuracy

```cpp
// Measure delay accuracy
void measure_delay_accuracy() {
    constexpr u32 TEST_DELAY_MS = 1000;
    constexpr u32 ITERATIONS = 10;

    u32 total_error = 0;

    for (u32 i = 0; i < ITERATIONS; i++) {
        u32 start = SysTickTimer::millis<board::BoardSysTick>();
        SysTickTimer::delay_ms<board::BoardSysTick>(TEST_DELAY_MS);
        u32 actual = SysTickTimer::millis<board::BoardSysTick>() - start;

        i32 error = static_cast<i32>(actual) - static_cast<i32>(TEST_DELAY_MS);
        total_error += (error < 0) ? -error : error;
    }

    u32 avg_error_ms = total_error / ITERATIONS;
    // Typical: 0-2ms error per second
}
```

---

## Troubleshooting

### "Timing is inaccurate"

**Symptoms**: Delays are shorter/longer than expected

**Causes**:
1. Incorrect clock configuration
2. High interrupt rate
3. Compiler optimization issues

**Solutions**:
```cpp
// 1. Verify clock frequency
static_assert(board::SYSTEM_CLOCK_HZ == 72000000, "Check board.hpp");

// 2. Measure actual timing
u32 start = SysTickTimer::millis<board::BoardSysTick>();
SysTickTimer::delay_ms<board::BoardSysTick>(1000);
u32 actual = SysTickTimer::millis<board::BoardSysTick>() - start;
// actual should be ~1000 ±1%

// 3. Check SysTick configuration
// Ensure SysTick is configured for correct frequency
```

### "System locks up in delay"

**Symptoms**: System stops responding during delay

**Causes**:
1. SysTick not initialized
2. Interrupts disabled
3. Using delay in ISR

**Solutions**:
```cpp
// 1. Ensure board::init() called
board::init();  // This initializes SysTick

// 2. Never disable interrupts during delay
// ❌ BAD:
__disable_irq();
SysTickTimer::delay_ms<board::BoardSysTick>(100);  // HANGS!

// 3. Never use delays in ISRs
// ❌ BAD:
extern "C" void TIM2_IRQHandler() {
    SysTickTimer::delay_ms<board::BoardSysTick>(10);  // HANGS!
}
```

### "millis() overflows"

**Symptoms**: Time comparisons fail after ~49 days

**Solution**: Use subtraction for comparisons:
```cpp
// ❌ Fails on overflow
u32 now = SysTickTimer::millis<board::BoardSysTick>();
if (now > target_time) { ... }

// ✅ Works with overflow
u32 now = SysTickTimer::millis<board::BoardSysTick>();
if ((now - start_time) > timeout) { ... }
```

---

## Examples

See complete working examples in:
- `examples/timing/basic_delays/` - Millisecond and microsecond delays
- `examples/timing/timeout_patterns/` - Non-blocking timeout patterns
- `examples/systick_demo/` - SysTick modes and RTOS integration

---

## Next Steps

- Learn about [ADC Usage](ADC_USAGE_GUIDE.md)
- See [API Reference](API_REFERENCE.md) for complete API
- Check [RTOS Integration](RTOS_QUICK_START.md) for multi-tasking

---

**Updated**: 2025-01-21
**Version**: 1.0
