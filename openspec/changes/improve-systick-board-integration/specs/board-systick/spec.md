# Board SysTick Integration Specification

## ADDED Requirements

### Requirement: Standard SysTick Type Definition

Each board SHALL define a `BoardSysTick` type alias using the platform-specific SysTick template with the board's system clock frequency.

**Rationale**: Provides compile-time type safety and clock frequency validation.

#### Scenario: Type alias defined in board header

- **GIVEN** a board with system clock frequency defined in ClockConfig
- **WHEN** the board header is included
- **THEN** BoardSysTick type SHALL be defined as `SysTick<ClockConfig::system_clock_hz>`
- **AND** the type SHALL be usable in all board code

#### Scenario: Clock frequency validation at compile-time

- **GIVEN** a board with invalid system clock frequency (e.g., 0 Hz or > 400 MHz)
- **WHEN** the code is compiled
- **THEN** a static_assert SHALL fail with a clear error message
- **AND** compilation SHALL not proceed

#### Scenario: Multiple boards use consistent pattern

- **GIVEN** nucleo_f401re, nucleo_f722ze, nucleo_g071rb, nucleo_g0b1re, and same70_xplained boards
- **WHEN** each board header is examined
- **THEN** all SHALL define BoardSysTick using the same pattern
- **AND** only the clock frequency parameter SHALL differ

---

### Requirement: Unified SysTick Interrupt Handler

Each board SHALL implement a SysTick_Handler() function that calls `BoardSysTick::increment_tick()` and optionally integrates with RTOS if enabled.

**Rationale**: Provides consistent interrupt handling pattern and RTOS integration point.

#### Scenario: Basic interrupt handler implementation

- **GIVEN** a board with SysTick initialized
- **WHEN** SysTick timer overflows
- **THEN** SysTick_Handler() SHALL be called automatically
- **AND** BoardSysTick::increment_tick() SHALL be invoked
- **AND** the tick counter SHALL increment by one

#### Scenario: RTOS integration when enabled

- **GIVEN** a board with ALLOY_RTOS_ENABLED defined
- **WHEN** SysTick_Handler() is called
- **THEN** BoardSysTick::increment_tick() SHALL be called first
- **AND** RTOS::tick() SHALL be called second
- **AND** context switch SHALL occur if needed

#### Scenario: Handler is extern C linkable

- **GIVEN** a SysTick_Handler implementation
- **WHEN** the handler is defined
- **THEN** it SHALL use `extern "C"` linkage
- **AND** it SHALL override the weak default from startup code
- **AND** it SHALL be callable from assembly/interrupt vector

---

### Requirement: SysTick Initialization in Board Init

Each board SHALL initialize SysTick during board::init() using a standard pattern with 1ms default tick period.

**Rationale**: Ensures timing functions work immediately after board initialization.

#### Scenario: SysTick initialized with 1ms period

- **GIVEN** a board that has not been initialized
- **WHEN** board::init() is called
- **THEN** SysTickTimer::init_ms<BoardSysTick>(1) SHALL be invoked
- **AND** SysTick SHALL be configured for 1ms interrupts
- **AND** timing functions SHALL become operational

#### Scenario: Initialization is idempotent

- **GIVEN** a board that is already initialized
- **WHEN** board::init() is called again
- **THEN** SysTick SHALL NOT be reinitialized
- **AND** existing tick counter SHALL be preserved
- **AND** no errors SHALL occur

#### Scenario: Initialization order is correct

- **GIVEN** board::init() sequence
- **WHEN** the function executes
- **THEN** system clock SHALL be configured BEFORE SysTick
- **AND** SysTick SHALL be initialized BEFORE enabling global interrupts
- **AND** LED initialization SHALL occur AFTER SysTick

---

### Requirement: Timing API Availability

After board initialization, all SysTick timing APIs SHALL be available and functional through the SysTickTimer interface.

**Rationale**: Provides complete timing capabilities for application code.

#### Scenario: Millisecond delay function works

- **GIVEN** a board with SysTick initialized at 1ms period
- **WHEN** SysTickTimer::delay_ms<BoardSysTick>(100) is called
- **THEN** execution SHALL block for approximately 100ms ±1%
- **AND** tick counter SHALL advance by approximately 100 ticks
- **AND** function SHALL return after delay completes

#### Scenario: Microsecond timing is accurate

- **GIVEN** a board with SysTick initialized
- **WHEN** SysTickTimer::micros<BoardSysTick>() is called
- **THEN** it SHALL return the current time in microseconds since initialization
- **AND** the value SHALL monotonically increase
- **AND** accuracy SHALL be within ±1% of actual time

#### Scenario: Timeout checking works correctly

- **GIVEN** a timeout of 50ms starting at time T0
- **WHEN** SysTickTimer::is_timeout_ms<BoardSysTick>(T0, 50) is called at T0 + 60ms
- **THEN** the function SHALL return true
- **AND** when called at T0 + 40ms, it SHALL return false
- **AND** wraparound SHALL be handled correctly

---

### Requirement: Compile-Time Period Validation

SysTick configuration SHALL validate that requested tick periods are achievable with the 24-bit counter at compile-time.

**Rationale**: Prevents runtime failures due to counter overflow.

#### Scenario: Valid 1ms period at various clock speeds

- **GIVEN** system clock frequencies of 48MHz, 84MHz, 180MHz, and 216MHz
- **WHEN** init_ms(1) is called
- **THEN** compilation SHALL succeed for all frequencies
- **AND** reload value SHALL fit in 24 bits (< 16,777,216)
- **AND** no static_assert failures SHALL occur

#### Scenario: Invalid period triggers compile error

- **GIVEN** a system clock of 216MHz
- **WHEN** init_ms(100) is requested (reload = 21,600,000 > 24-bit max)
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL indicate period is too large
- **AND** suggested maximum period SHALL be provided

#### Scenario: Microsecond period validation

- **GIVEN** a system clock of 180MHz
- **WHEN** init_us(50) is requested (reload = 9,000)
- **THEN** compilation SHALL succeed
- **AND** SysTick SHALL be configured for 50us interrupts
- **AND** timing functions SHALL use 50us resolution

---

### Requirement: Documentation and Comments

Each board's SysTick implementation SHALL include comprehensive documentation explaining configuration, usage, and RTOS integration.

**Rationale**: Makes the pattern easy to understand and replicate for new boards.

#### Scenario: SysTick_Handler has clear documentation

- **GIVEN** any board's board.cpp file
- **WHEN** the SysTick_Handler function is examined
- **THEN** it SHALL have a Doxygen comment block
- **AND** the comment SHALL explain when it's called (every 1ms)
- **AND** it SHALL document the increment_tick() call
- **AND** it SHALL note that it overrides the weak default

#### Scenario: Board initialization documents SysTick setup

- **GIVEN** any board's board::init() function
- **WHEN** the SysTick initialization line is examined
- **THEN** a comment SHALL explain the tick period (1ms)
- **AND** a comment SHALL reference the SysTick_Handler
- **AND** a comment SHALL note timing function availability

#### Scenario: Board header documents BoardSysTick type

- **GIVEN** any board's board.hpp or board_config.hpp
- **WHEN** the BoardSysTick type alias is examined
- **THEN** a comment SHALL explain the type's purpose
- **AND** it SHALL document the clock frequency parameter
- **AND** it SHALL provide usage examples
