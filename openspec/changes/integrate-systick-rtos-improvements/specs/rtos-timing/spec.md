# RTOS Timing and SysTick Integration Specification

## ADDED Requirements

### Requirement: Standardized RTOS Tick Source

All ARM-based boards SHALL use SysTick as the RTOS tick source with standardized integration pattern.

**Rationale**: Ensures consistent timing behavior and portability across all boards.

#### Scenario: BoardSysTick type for RTOS

- **GIVEN** a board with system clock configured
- **WHEN** board header is examined
- **THEN** BoardSysTick type SHALL be defined using SysTick<CLOCK_HZ>
- **AND** tick period SHALL be configured for 1ms (standard for RTOS)
- **AND** type SHALL be usable by both timing APIs and RTOS

```cpp
// In board.hpp or board_config.hpp
using BoardSysTick = SysTick<ClockConfig::system_clock_hz>;
static_assert(BoardSysTick::tick_period_ms == 1,
              "RTOS requires 1ms tick period");
```

#### Scenario: SysTick_Handler forwards to RTOS

- **GIVEN** RTOS is enabled (#ifdef ALLOY_RTOS_ENABLED)
- **WHEN** SysTick interrupt fires every 1ms
- **THEN** SysTick_Handler SHALL call board::BoardSysTick::increment_tick()
- **AND** SysTick_Handler SHALL call RTOS::tick()
- **AND** context switch SHALL occur if needed

```cpp
extern "C" void SysTick_Handler() {
    board::BoardSysTick::increment_tick();

    #ifdef ALLOY_RTOS_ENABLED
        RTOS::tick().unwrap();  // Forward to scheduler
    #endif
}
```

#### Scenario: RTOS tick disabled when not needed

- **GIVEN** bare-metal application without RTOS
- **WHEN** SysTick interrupt fires
- **THEN** only BoardSysTick::increment_tick() SHALL be called
- **AND** RTOS::tick() SHALL NOT be called
- **AND** no RTOS overhead SHALL occur

---

### Requirement: RTOS Tick Validation

RTOS SHALL validate at compile-time that tick source provides 1ms period required for scheduling.

**Rationale**: Prevents runtime timing issues from incorrect tick configuration.

#### Scenario: Tick period compile-time check

- **GIVEN** SysTick configured with period other than 1ms
- **WHEN** RTOS is initialized
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL indicate RTOS requires 1ms tick

```cpp
template <typename TickSource>
concept RTOSTickSource = requires {
    requires TickSource::tick_period_ms == 1;
    { TickSource::get_tick_count() } -> std::same_as<u32>;
    { TickSource::micros() } -> std::same_as<u64>;
};

template <RTOSTickSource TickSource>
void RTOS::initialize() {
    // Guaranteed 1ms tick
}

// Example failure:
using FastTick = SysTick<100>;  // 100us period
RTOS::initialize<FastTick>();
// Error: "RTOS requires 1ms tick period (TickSource provides 100us)"
```

#### Scenario: Clock frequency validation

- **GIVEN** board with system clock frequency
- **WHEN** SysTick is configured for RTOS
- **THEN** reload value SHALL be calculated for 1ms period
- **AND** reload value SHALL fit in 24-bit counter
- **AND** compile error SHALL occur if impossible

```cpp
// F401RE @ 84MHz: Reload = 84,000 - 1 = 83,999 ✓ (< 16,777,215)
// F722ZE @ 216MHz: Reload = 216,000 - 1 = 215,999 ✓
// Hypothetical @ 400MHz: Reload = 400,000 - 1 = 399,999 ✓
// Extreme @ 17MHz: Reload = 16,777,216 ✗ (overflow!)
//   Error: "1ms tick period impossible at 17MHz (use /8 prescaler)"
```

---

### Requirement: RTOS Tick with Result<T,E>

RTOS::tick() SHALL return Result<void, RTOSError> for consistent error handling.

**Rationale**: Aligns with HAL error handling and enables error propagation.

#### Scenario: Successful tick returns Ok

- **GIVEN** RTOS running normally
- **WHEN** RTOS::tick() is called from SysTick_Handler
- **THEN** function SHALL return Ok(void)
- **AND** scheduler SHALL update delayed tasks
- **AND** context switch SHALL trigger if needed

```cpp
extern "C" void SysTick_Handler() {
    board::BoardSysTick::increment_tick();

    #ifdef ALLOY_RTOS_ENABLED
        auto result = RTOS::tick();
        if (result.is_err()) {
            handle_rtos_error(result.unwrap_err());
        }
    #endif
}
```

#### Scenario: Tick error handling

- **GIVEN** RTOS encounters error during tick (rare)
- **WHEN** RTOS::tick() is called
- **THEN** function SHALL return Err(RTOSError)
- **AND** error SHALL be propagated to handler
- **AND** system SHALL enter safe state or log error

```cpp
Result<void, RTOSError> RTOS::tick() {
    // Update delayed tasks
    auto wake_result = scheduler::wake_delayed_tasks();
    if (wake_result.is_err()) {
        return wake_result;  // Propagate error
    }

    // Trigger context switch if needed
    if (scheduler::need_context_switch()) {
        scheduler::reschedule();
    }

    return Ok();
}
```

---

### Requirement: RTOS Timing APIs

RTOS SHALL provide timing APIs that use the same tick source as scheduler for consistency.

**Rationale**: Ensures delays and timeouts use same time base as task scheduling.

#### Scenario: RTOS delay uses tick count

- **GIVEN** task wants to delay for N milliseconds
- **WHEN** RTOS::delay(N) is called
- **THEN** task SHALL be blocked for N tick periods
- **AND** other tasks SHALL run during delay
- **AND** task SHALL wake exactly after N ticks (±1 tick jitter)

```cpp
void sensor_task() {
    while (1) {
        read_sensor();
        RTOS::delay(100);  // Delay 100ms using RTOS tick
        // Other tasks run during this delay
    }
}
```

#### Scenario: RTOS absolute delay

- **GIVEN** task wants to run at precise intervals
- **WHEN** RTOS::delay_until(wake_time) is used
- **THEN** task SHALL wake at exact tick count
- **AND** jitter SHALL be eliminated
- **AND** interval SHALL be consistent

```cpp
void periodic_task() {
    u32 next_wake = RTOS::get_tick_count();

    while (1) {
        do_work();

        next_wake += 10;  // Next run in 10ms
        RTOS::delay_until(next_wake);
        // Runs exactly every 10ms, no drift
    }
}
```

#### Scenario: Timeout consistency

- **GIVEN** mutex with timeout in milliseconds
- **WHEN** timeout is checked
- **THEN** timeout SHALL use RTOS tick count
- **AND** timeout SHALL be consistent with RTOS::delay()
- **AND** same time base SHALL be used throughout

```cpp
// Both use same tick source
RTOS::delay(100);           // 100ms delay
mutex.lock(100);            // 100ms timeout
queue.receive(data, 100);   // 100ms timeout
// All use RTOS::get_tick_count() internally
```

---

### Requirement: Tick Accuracy Validation

RTOS tick SHALL maintain ±1% accuracy under normal load conditions.

**Rationale**: Ensures timing-sensitive applications behave correctly.

#### Scenario: Measure tick accuracy over time

- **GIVEN** RTOS running for extended period (minutes)
- **WHEN** tick count is compared to real-time reference
- **THEN** error SHALL be within ±1% over 1 minute
- **AND** error SHALL not accumulate over time
- **AND** drift SHALL be bounded

```cpp
// Test: Run for 60 seconds, compare to reference timer
u32 start_tick = RTOS::get_tick_count();
// ... wait 60 seconds using external reference ...
u32 end_tick = RTOS::get_tick_count();
u32 elapsed_ms = end_tick - start_tick;

// Expected: 60,000ms ±600ms (±1%)
assert(elapsed_ms >= 59400 && elapsed_ms <= 60600);
```

#### Scenario: Tick accuracy under interrupt load

- **GIVEN** system with multiple high-frequency interrupts
- **WHEN** RTOS tick fires
- **THEN** tick SHALL not be delayed by more than 1 tick period
- **AND** scheduler SHALL account for missed ticks
- **AND** tasks SHALL not be starved

---

### Requirement: Tickless Idle Support (Future)

RTOS SHALL provide hooks for tickless idle implementation to reduce power consumption.

**Rationale**: Enables low-power applications by stopping tick during idle.

#### Scenario: Tickless idle hook defined

- **GIVEN** application wants to implement tickless idle
- **WHEN** all tasks are blocked
- **THEN** RTOS SHALL call tickless_enter() hook
- **AND** application SHALL disable SysTick
- **AND** application SHALL configure wakeup source

```cpp
// Future API (not implemented yet)
namespace RTOS {
    // Called when entering idle with all tasks blocked
    __attribute__((weak)) void tickless_enter(u32 expected_idle_ticks) {
        // User can override this
        // 1. Disable SysTick
        // 2. Configure wakeup timer for expected_idle_ticks
        // 3. Enter low-power mode
    }

    // Called when exiting idle
    __attribute__((weak)) u32 tickless_exit() {
        // User can override this
        // 1. Enable SysTick
        // 2. Return actual ticks spent idle
        // 3. Scheduler adjusts tick count
        return 0;
    }
}
```

#### Scenario: Tick count adjustment after idle

- **GIVEN** system woke from tickless idle
- **WHEN** tickless_exit() returns actual idle ticks
- **THEN** RTOS SHALL add idle ticks to tick count
- **AND** delayed tasks SHALL wake if time expired
- **AND** timing SHALL remain accurate
