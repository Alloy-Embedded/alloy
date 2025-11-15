# Board Support Specification Delta

## MODIFIED Requirements

### Requirement: Board Initialization Sequence

Each board SHALL implement a standardized initialization sequence in board::init() that configures hardware in the correct order.

**Rationale**: Ensures reliable hardware startup and prevents initialization race conditions.

#### Scenario: Complete initialization sequence

- **GIVEN** an uninitialized board
- **WHEN** board::init() is called
- **THEN** the following SHALL occur in order:
  1. System clock configuration (to maximum frequency)
  2. GPIO peripheral clock enable
  3. **SysTick timer initialization (1ms period)** ⬅️ ADDED
  4. Board peripheral initialization (LEDs, buttons)
  5. Global interrupt enable
- **AND** board_initialized flag SHALL be set to true

#### Scenario: SysTick available after init

- **GIVEN** a board that has completed initialization
- **WHEN** timing functions are called
- **THEN** SysTickTimer::millis<BoardSysTick>() SHALL return valid time
- **AND** SysTickTimer::delay_ms<BoardSysTick>(X) SHALL work correctly
- **AND** timing accuracy SHALL be within ±1%

#### Scenario: Idempotent initialization includes SysTick

- **GIVEN** a board that is already initialized
- **WHEN** board::init() is called again
- **THEN** system clock SHALL NOT be reconfigured
- **AND** SysTick SHALL NOT be reinitialized ⬅️ ADDED
- **AND** tick counter SHALL NOT be reset ⬅️ ADDED
- **AND** peripheral state SHALL be preserved
