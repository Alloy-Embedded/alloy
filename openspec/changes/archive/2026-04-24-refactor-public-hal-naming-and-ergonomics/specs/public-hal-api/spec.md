## ADDED Requirements

### Requirement: Public HAL Shall Expose Ergonomic Selected-Device Aliases

The public HAL SHALL expose concise selected-device aliases for normal user code while preserving
the canonical generated ids behind the runtime boundary.

#### Scenario: User opens a GPIO pin through the public HAL

- **WHEN** a user writes normal application code against the public HAL
- **THEN** they can name pins and peripherals through concise public aliases such as
  `alloy::dev::pin::PA5` and `alloy::dev::periph::USART2`
- **AND** they do not need to spell `alloy::device::PinId::*` or
  `alloy::device::PeripheralId::*` as the taught first-choice syntax

### Requirement: Public HAL Shall Prefer Role-First Route Syntax

The public HAL SHALL prefer concise, role-first route syntax for UART, SPI, I2C, and similar
peripherals rather than teaching the raw connector kernel as normal usage.

#### Scenario: User connects a UART through the public HAL

- **WHEN** a user defines a UART route on the public HAL surface
- **THEN** they can express it through a route type such as `uart::route<...>` with role aliases
  like `tx<...>` and `rx<...>`
- **AND** the runtime keeps `connection::connector<...>` as an internal or expert-level kernel
  rather than the primary taught API shape

### Requirement: Public HAL Shall Preserve Expert Signal Control For Ambiguous Cases

The ergonomic public layer SHALL preserve an explicit expert escape hatch when a role/pin choice is
not uniquely resolvable from the published semantic traits.

#### Scenario: User configures a route with an ambiguous signal role

- **WHEN** a selected pin and peripheral support more than one plausible signal mapping for the
  same public role
- **THEN** the compile-time path reports the ambiguity and directs the user to the published
  connector guidance exposed by `alloyctl explain`
- **AND** the user can still spell an explicit expert-level signal choice through a concise public
  expert alias or equivalent canonical override

### Requirement: Public HAL Shall Avoid Teaching Structural Wrapper Noise

The public HAL SHALL avoid teaching structural wrapper types that only expose internal runtime
shape without adding user-facing hardware meaning.

#### Scenario: User reads a GPIO example

- **WHEN** a user reads an official raw HAL example for GPIO
- **THEN** the example prefers user-facing aliases instead of teaching
  `device::pin<>` and `gpio::pin_handle<...>` as the normal first syntax
- **AND** the underlying runtime still resolves to the same descriptor-driven implementation
