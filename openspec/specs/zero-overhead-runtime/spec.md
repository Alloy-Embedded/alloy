# zero-overhead-runtime Specification

## Purpose
Zero-overhead runtime defines the performance contract for foundational hot paths in `alloy`.

Board bring-up and foundational peripheral operations must collapse to direct register work derived
from the selected typed runtime contract. Optional layers, generic reflection helpers, and async
infrastructure cannot leak hidden cost into blocking-only hot paths.
## Requirements
### Requirement: Foundational Runtime Paths Shall Not Require Family-Wide Descriptor Scans

Normal foundational driver paths in `alloy` SHALL NOT require generic scans over family-wide
generated descriptor tables to resolve one selected peripheral instance or route.

#### Scenario: Opening board debug UART

- **WHEN** board code opens its selected debug UART through the public HAL
- **THEN** the implementation resolves instance, clock/reset, and route data through generated
  runtime compile-time constructs
- **AND** it does not depend on a generic family-wide table walk at runtime

### Requirement: Runtime-Lite Consumption Shall Be Zero-Overhead By Construction

The runtime SHALL treat generated runtime refs, traits, and operation packs as the primary
implementation boundary for foundational bring-up paths.

#### Scenario: GPIO mode setup on a foundational board

- **WHEN** a foundational board configures a GPIO pin through the public HAL
- **THEN** the compiled path can be reduced to direct register writes implied by the selected
  generated runtime data

### Requirement: Optional Async Support Shall Not Penalize Blocking-Only Hot Paths

The runtime SHALL keep blocking-only foundational hot paths free from async-layer overhead when
the async layer is not selected.

#### Scenario: Blocking SPI transaction in non-async build
- **WHEN** a foundational SPI transaction is compiled in a blocking-only build
- **THEN** the generated code does not pay for optional async infrastructure
- **AND** zero-overhead expectations remain enforced

### Requirement: Zero-Overhead Claims Shall Be Backed By Explicit Validation Gates

Claims that foundational runtime paths are zero-overhead SHALL be validated by dedicated gates and
not by architectural intent alone.

#### Scenario: Changing a foundational GPIO or UART hot path

- **WHEN** a change affects a foundational GPIO or UART bring-up path
- **THEN** assembly or size verification proves that the hot path still collapses to the expected
  low-level operations
- **AND** host MMIO or equivalent behavioral validation proves that the same path still performs
  the intended register effects

