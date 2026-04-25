# startup-runtime Spec Delta: Multi-Core Startup

## ADDED Requirements

### Requirement: Startup Runtime SHALL Support Multi-Core Bring-Up From Descriptor Data

The startup runtime SHALL bring up every core declared by the active device descriptor,
using algorithms in alloy and bring-up data from `alloy-devices`, when the descriptor
declares more than one core. Boards and examples SHALL NOT carry handwritten cross-core
start sequences.

#### Scenario: ESP32 classic dual-core start
- **WHEN** the runtime starts on an ESP32-classic target
- **THEN** the primary core (PRO_CPU) completes the existing single-core startup
  sequence
- **AND** the runtime then triggers the secondary core (APP_CPU) using the start
  vector, sync register, and cache-flush facts published by the device descriptor
- **AND** the secondary core enters the runtime through the same typed entry point
  used by the primary core

#### Scenario: Single-core targets are unaffected
- **WHEN** the runtime starts on a single-core target
- **THEN** the multi-core bring-up path is not invoked
- **AND** the startup sequence is byte-for-byte equivalent to the pre-change behaviour

### Requirement: Startup Runtime SHALL Expose A Vendor-Neutral Current-Core Surface

The runtime SHALL expose a vendor-neutral public surface for application code to
identify the running core and dispatch work to a specific core, without leaking ESP
or vendor terminology into the public header.

#### Scenario: Application reads current core
- **WHEN** application code calls `alloy::runtime::current_core()`
- **THEN** it returns one of the values defined by the public `alloy::runtime::Core`
  enum (`primary` or `secondary`)
- **AND** the public header does not refer to `PRO_CPU`, `APP_CPU`, or any other
  vendor-specific core name

#### Scenario: Application launches work on a chosen core
- **WHEN** application code calls
  `alloy::runtime::launch_on(alloy::runtime::Core::secondary, fn)` on a multi-core
  target
- **THEN** the runtime schedules `fn` to run on the secondary core through the
  startup runtime's documented dispatch path
- **AND** the call is rejected at compile time on a single-core target, or returns
  a typed error at runtime, depending on the chosen implementation strategy
  (decision recorded in design.md)
