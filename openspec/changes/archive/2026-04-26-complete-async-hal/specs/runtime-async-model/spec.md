## ADDED Requirements

### Requirement: Runtime SHALL Cover Every Foundational Peripheral With An Async Wrapper

The runtime MUST ship a header-only async wrapper per foundational
peripheral that returns
`core::Result<operation<EventToken>, core::ErrorCode>`. The set covers
UART (DMA), SPI (DMA), I2C (interrupt-driven), ADC (single conversion +
DMA scan), Timer (periodic + one-shot), and GPIO edge. Every wrapper
MUST be opt-in via its own header (`runtime/async_<peripheral>.hpp`)
and MUST be re-exported through the umbrella `async.hpp` for the
convenience case.

#### Scenario: Every foundational peripheral has its async header

- **WHEN** the runtime tree is reviewed
- **THEN** the following headers exist and define their respective
  free-function wrappers:
  - `runtime/async_uart.hpp` (`write_dma`, `read_dma`)
  - `runtime/async_spi.hpp` (`write_dma`, `read_dma`, `transfer_dma`)
  - `runtime/async_i2c.hpp` (`write`, `read`, `write_read`)
  - `runtime/async_adc.hpp` (`read`, `scan_dma`)
  - `runtime/async_timer.hpp` (`wait_period`, `delay`)
  - `runtime/async_gpio.hpp` (`wait_edge<Pin>`)
- **AND** the umbrella `async.hpp` includes every async_*.hpp
- **AND** the namespace `alloy::async` aliases `runtime::async` so
  callers reach every wrapper via `alloy::async::<peripheral>::<op>`

### Requirement: Each Async Wrapper SHALL Carry A Typed Per-Peripheral Completion Token

Async wrappers MUST return an operation parameterised by a typed
completion token whose static state is unique per
`(peripheral_kind, peripheral_id_or_pin_id)` tuple. The runtime MUST
ship distinct token namespaces — `dma_event::token<P, S>`,
`i2c_event::token<P>`, `adc_event::token<P>`,
`timer_event::token<P>`, `gpio_event::token<Pin>` — each built on the
existing generic `event::completion<Tag>` template so two async waits
on different instances cannot alias.

#### Scenario: Tokens for distinct instances do not alias

- **WHEN** two tasks issue async operations on different I2C instances
  (e.g. `I2C1` and `I2C2`)
- **THEN** they observe two distinct `i2c_event::token<…>` types
- **AND** signalling `token<I2C1>` does not wake the task waiting on
  `token<I2C2>`

#### Scenario: GPIO tokens are keyed on PinId

- **WHEN** an application waits for an edge on `PA0` and another on
  `PA1`
- **THEN** the two operations carry `gpio_event::token<PA0>` and
  `gpio_event::token<PA1>` respectively
- **AND** the EXTI ISR that signals one does not signal the other

### Requirement: Async Wrappers SHALL Be Concept-Checked At Compile Time

The runtime MUST ship at least one compile test that instantiates every
async wrapper (UART, SPI, I2C, ADC, Timer, GPIO) against a duck-typed
mock handle and confirms the return type satisfies the documented
contract. The compile test MUST be part of the device-contract smoke
build so a regression in any wrapper signature fails the build.

#### Scenario: A regression in a wrapper signature fails the contract smoke

- **WHEN** the contract smoke build runs against a board whose
  descriptor exposes the foundational peripheral set
- **THEN** every async wrapper is instantiated against a mock handle
- **AND** any change that removes / renames / re-types a wrapper
  argument fails compilation, surfacing the break to reviewers

### Requirement: Async Wrappers SHALL Honour The Blocking-Only Zero-Overhead Invariant

Async wrappers MUST stay opt-in: a build that does not include any
`async_<peripheral>.hpp` and does not name `alloy::async::` MUST link
zero async-wrapper symbols and MUST pay zero async-wrapper RAM cost,
even when the same project also consumes blocking HAL APIs and typed
completion tokens directly. The runtime MUST enforce this invariant
via a CI
guard that scans the canonical blocking-only example and the
blocking-only compile test for any include of an async wrapper or any
use of the `alloy::async::` / `runtime::async::` namespace.

#### Scenario: Blocking-only guard catches any async include

- **WHEN** a contributor adds `#include "runtime/async_<peripheral>.hpp"`
  to either `examples/async_uart_timeout/main.cpp` or
  `tests/compile_tests/test_blocking_only_completion_api.cpp`
- **THEN** `scripts/check_blocking_only_path.py` reports the violation
  with the file, line, and pattern
- **AND** the guard exits non-zero so the CI job fails

#### Scenario: Builds without async include link no async symbols

- **WHEN** the zero-overhead release gate compiles a board that does
  NOT include any `async_*.hpp` header
- **THEN** the resulting linked image contains no async-wrapper code
- **AND** no async-related `.bss` / `.data` cost is paid
