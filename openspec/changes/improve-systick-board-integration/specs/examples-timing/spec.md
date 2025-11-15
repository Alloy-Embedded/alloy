# Timing Examples Specification

## ADDED Requirements

### Requirement: Basic Delays Example

A basic_delays example SHALL demonstrate accurate millisecond and microsecond blocking delays using SysTick.

**Rationale**: Teaches fundamental timing operations and validates SysTick accuracy.

#### Scenario: Millisecond delay demonstration

- **GIVEN** the basic_delays example running on any board
- **WHEN** the example executes delay_ms(500)
- **THEN** LED SHALL toggle every 500ms ±5ms
- **AND** example SHALL print actual delay duration
- **AND** timing accuracy SHALL be documented in comments

#### Scenario: Microsecond delay demonstration

- **GIVEN** the basic_delays example running on any board
- **WHEN** the example executes delay_us(100)
- **THEN** a short pulse SHALL be generated
- **AND** pulse width SHALL be 100us ±2us
- **AND** example SHALL demonstrate measurement with GPIO toggle

#### Scenario: Example builds for all boards

- **GIVEN** nucleo_f401re, nucleo_f722ze, nucleo_g071rb, nucleo_g0b1re, and same70_xplained
- **WHEN** CMakeLists.txt is configured for each board
- **THEN** compilation SHALL succeed with zero errors
- **AND** example SHALL use BoardSysTick from each board
- **AND** behavior SHALL be consistent across boards

---

### Requirement: Timeout Patterns Example

A timeout_patterns example SHALL demonstrate non-blocking timeout handling for polling operations.

**Rationale**: Shows real-world pattern for communication timeouts and sensor polling.

#### Scenario: Blocking timeout with retry

- **GIVEN** a simulated sensor that responds after N attempts
- **WHEN** the example polls with 100ms timeout and 3 retries
- **THEN** example SHALL demonstrate timeout detection
- **AND** example SHALL show retry logic
- **AND** example SHALL report success or final timeout

#### Scenario: Non-blocking timeout pattern

- **GIVEN** a long-running operation (e.g., waiting for sensor data)
- **WHEN** using is_timeout_ms() for non-blocking check
- **THEN** example SHALL show periodic polling without blocking
- **AND** example SHALL demonstrate doing work while waiting
- **AND** example SHALL handle timeout gracefully

#### Scenario: Multiple concurrent timeouts

- **GIVEN** two independent operations with different timeouts (50ms and 100ms)
- **WHEN** both are monitored simultaneously
- **THEN** example SHALL track each timeout independently
- **AND** shorter timeout SHALL trigger first
- **AND** longer timeout SHALL trigger second
- **AND** example SHALL demonstrate state machine pattern

---

### Requirement: Performance Measurement Example

A performance example SHALL demonstrate measuring function execution time and collecting timing statistics.

**Rationale**: Shows how to profile code and measure worst-case execution time.

#### Scenario: Function execution time measurement

- **GIVEN** a function that performs calculations
- **WHEN** the performance example measures its execution time
- **THEN** elapsed time SHALL be reported in microseconds
- **AND** example SHALL demonstrate micros() before/after pattern
- **AND** multiple runs SHALL be averaged for accuracy

#### Scenario: Worst-case execution time tracking

- **GIVEN** a function called 1000 times
- **WHEN** the example tracks min, max, and average execution time
- **THEN** worst-case time SHALL be identified
- **AND** timing statistics SHALL be displayed
- **AND** example SHALL demonstrate WCET analysis pattern

#### Scenario: Interrupt latency measurement

- **GIVEN** a GPIO interrupt triggering on external event
- **WHEN** the performance example measures time from event to ISR
- **THEN** interrupt latency SHALL be reported
- **AND** typical latency SHALL be < 10us on Cortex-M4
- **AND** example SHALL demonstrate high-resolution timing with micros()

---

### Requirement: SysTick Demo Example

A systick_demo example SHALL showcase different SysTick tick rates and their trade-offs.

**Rationale**: Educates users on tick resolution selection and demonstrates RTOS integration patterns.

#### Scenario: 1ms tick mode demonstration

- **GIVEN** SysTick configured for 1ms ticks
- **WHEN** the example runs for 10 seconds
- **THEN** example SHALL report total ticks (approximately 10,000)
- **AND** CPU overhead SHALL be displayed (< 0.1%)
- **AND** example SHALL demonstrate typical RTOS configuration

#### Scenario: 100us tick mode demonstration

- **GIVEN** SysTick configured for 100us ticks
- **WHEN** the example runs for 10 seconds
- **THEN** example SHALL report total ticks (approximately 100,000)
- **AND** CPU overhead SHALL be displayed (< 1%)
- **AND** example SHALL demonstrate high-resolution timing
- **AND** increased interrupt rate impact SHALL be documented

#### Scenario: Tick resolution comparison

- **GIVEN** the systick_demo example
- **WHEN** both 1ms and 100us modes are demonstrated
- **THEN** example SHALL show timing resolution difference
- **AND** example SHALL document when to use each mode
- **AND** example SHALL demonstrate resolution trade-off with overhead

#### Scenario: RTOS tick integration pattern

- **GIVEN** the systick_demo example with RTOS integration section
- **WHEN** examining the SysTick_Handler code
- **THEN** example SHALL show conditional RTOS::tick() call
- **AND** example SHALL document integration pattern
- **AND** example SHALL explain tick forwarding to scheduler

---

### Requirement: Example Documentation

Each timing example SHALL include comprehensive README with expected behavior, timing accuracy, and hardware requirements.

**Rationale**: Ensures examples are self-explanatory and educational.

#### Scenario: README includes expected output

- **GIVEN** any timing example directory
- **WHEN** the README.md is opened
- **THEN** it SHALL include expected console output
- **AND** it SHALL document expected LED behavior
- **AND** it SHALL show sample timing measurements

#### Scenario: README documents timing accuracy

- **GIVEN** any timing example README
- **WHEN** accuracy section is examined
- **THEN** it SHALL document expected accuracy (±1%)
- **AND** it SHALL explain sources of timing error
- **AND** it SHALL provide troubleshooting tips for inaccurate timing

#### Scenario: README explains build and flash process

- **GIVEN** any timing example README
- **WHEN** build instructions are examined
- **THEN** it SHALL show CMake configuration command
- **AND** it SHALL show build command
- **AND** it SHALL show flash command
- **AND** it SHALL list supported boards

---

### Requirement: Example Portability

All timing examples SHALL compile and run correctly on all supported boards without modification.

**Rationale**: Demonstrates the portability of the HAL abstraction.

#### Scenario: Example uses board-agnostic APIs

- **GIVEN** any timing example source code
- **WHEN** the code is examined
- **THEN** it SHALL use board:: namespace for hardware access
- **AND** it SHALL use SysTickTimer with BoardSysTick template
- **AND** it SHALL NOT contain board-specific #ifdefs
- **AND** it SHALL work on all ARM Cortex-M boards

#### Scenario: CMakeLists.txt supports all boards

- **GIVEN** any timing example CMakeLists.txt
- **WHEN** ALLOY_BOARD is set to any supported board
- **THEN** configuration SHALL succeed
- **AND** compilation SHALL succeed
- **AND** resulting binary SHALL be flashable
- **AND** example SHALL run correctly

#### Scenario: Different clock frequencies handled automatically

- **GIVEN** timing examples running on F401RE (84MHz) and F722ZE (216MHz)
- **WHEN** delay_ms(100) is called on both
- **THEN** both SHALL delay for 100ms ±1%
- **AND** timing functions SHALL automatically adjust for clock frequency
- **AND** no code changes SHALL be required
