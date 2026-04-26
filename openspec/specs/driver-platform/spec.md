# driver-platform Specification

## Purpose
TBD - created by archiving change expand-driver-ecosystem. Update Purpose after archive.
## Requirements
### Requirement: Drivers SHALL Live Under A Stable Namespace And Directory Layout

Every alloy driver MUST live under `drivers/<category>/<name>/` and MUST
expose its public surface in the namespace
`alloy::drivers::<category>::<name>`. Categories MUST come from a small
controlled set: `sensor`, `display`, `power`, `memory`, `net`,
`filesystem`, `protocol`, `usb`. Headers MUST be self-contained
(`#pragma once`, all required includes named).

#### Scenario: Drivers are discoverable from the path

- **WHEN** a contributor adds a driver named `xyz` of category `sensor`
- **THEN** the header lives at `drivers/sensor/xyz/xyz.hpp`
- **AND** the public symbols live in `alloy::drivers::sensor::xyz`
- **AND** the path-to-namespace correspondence is enforced by review

#### Scenario: Categories are bounded

- **WHEN** the maintainer reviews the driver tree
- **THEN** every immediate child of `drivers/` matches one of the
  controlled category names
- **AND** new categories require an explicit OpenSpec change to extend
  the set, not an ad-hoc `mkdir`

### Requirement: Drivers SHALL Be Bus-Concept-Templated And Concept-Checked

Every driver that needs an external bus MUST template over the bus
type and accept any value satisfying the matching alloy HAL concept
(`alloy::hal::i2c::Bus`, `alloy::hal::spi::Bus`, `alloy::hal::uart::Bus`,
`alloy::hal::one_wire::Master`, etc.). Drivers MUST emit a
`static_assert` (or equivalent compile-time check) against a fake bus
type so concept conformance fails the build instead of silently
drifting. Drivers MUST NOT take raw vendor handles, peripheral indices,
or function pointers as substitute for a typed bus.

#### Scenario: Bus type is a template parameter

- **WHEN** any driver needs to talk to an external chip
- **THEN** the public type is templated over the bus
  (e.g. `Driver<I2cBus>`, `Driver<SpiBus, CsPolicy>`)
- **AND** instantiation against any conforming bus compiles without
  driver-internal `#ifdef`s

#### Scenario: Concept conformance is enforced

- **WHEN** a driver header is compiled
- **THEN** a `static_assert` checks the bus parameter against the
  matching HAL concept
- **AND** a regression in the concept (added requirement, removed
  method) fails the driver build, not just the HAL build

### Requirement: Drivers SHALL Be Heap-Free, Exception-Free, And Result-Typed

Every alloy driver MUST be `noexcept` throughout, MUST NOT call
`new` / `malloc` / `printf`, and MUST report all errors through typed
`core::Result<T, E>` values (driver-defined `enum class` or
`core::ErrorCode`). Buffer sizing MUST be expressed via template
parameters or `std::array` members; runtime allocation is forbidden.
Logging from inside a driver MUST NOT depend on host stdio; consumers
attach observers if needed.

#### Scenario: Zero-overhead release gate covers drivers

- **WHEN** the zero-overhead release gate compiles a build that pulls
  in a driver
- **THEN** no `malloc` / `new` / `printf` symbols appear in the linked
  output from driver code
- **AND** every driver function is `noexcept`

#### Scenario: Errors are typed Results

- **WHEN** a driver method can fail
- **THEN** the return type is `core::Result<…, ErrorCode>` with a
  driver-defined or shared error enum
- **AND** no driver call site uses errno / negative ints / sentinel
  values to signal failure

### Requirement: Drivers SHALL Carry A Datasheet-Anchored Header Comment And Public Constants

Every driver header MUST open with a top-of-file comment naming the
target chip(s), the datasheet revision, the bus interface, and the
contract level (seed driver vs full driver). Public constants
required to use the driver (default I2C address(es), expected chip-id
byte(s), key timing budgets) MUST be exposed via named `inline
constexpr` values, not magic numbers buried in the implementation.

#### Scenario: Header opens with anchored context

- **WHEN** any driver header is opened
- **THEN** the file's leading comment names the chip(s), datasheet
  revision (date or rev letter), and bus interface
- **AND** mentions whether the driver is a seed (probe + minimal
  read/write) or full (event hooks, advanced features)

#### Scenario: Default address and chip-id are public constants

- **WHEN** a driver targets a chip with a default bus address (e.g.
  `kPrimaryAddress`) and an expected chip-id byte (e.g.
  `kExpectedChipId`)
- **THEN** both are exposed as `inline constexpr` named constants in
  the driver namespace
- **AND** application code can probe the chip without copy-pasting
  hex literals

### Requirement: Drivers SHALL Have A Compile Test And A Probe Example

Every driver merged into `drivers/` MUST ship at least one compile
test under `tests/compile_tests/` that verifies concept conformance
against a fake bus, and at least one example under
`examples/driver_<name>_probe/` that exercises the canonical
init+read path against a representative board. The probe example
MUST be the smallest possible demo — chip-id read or one-shot
measurement — so it serves as documentation by example.

#### Scenario: Compile test pins the API surface

- **WHEN** a new driver is merged
- **THEN** `tests/compile_tests/test_driver_seed_<name>.cpp` (or
  similarly named) instantiates the driver against a fake bus and
  exercises every public method
- **AND** any breaking API change requires updating the compile test,
  surfacing the break to reviewers

#### Scenario: Probe example demonstrates canonical init

- **WHEN** a driver lands
- **THEN** an `examples/driver_<name>_probe/` directory exists with a
  minimal main that initialises the driver, performs the canonical
  read, and reports the result via the board's debug UART or LED
- **AND** the example builds at least for one in-tree foundational
  board

### Requirement: Drivers SHALL Be Catalogued In MANIFEST.json And DRIVERS.md

Every driver MUST appear as an entry in `drivers/MANIFEST.json` with
fields `name`, `category`, `interface`, `chips`, `status`, `example`,
and `notes`. The `status` field MUST take one of two values:
`compile-review` (concept + API verified at compile time, no
hardware run) or `hardware-validated` (run on a representative
board with the result archived). `docs/DRIVERS.md` MUST mirror the
manifest as a human-readable catalog grouped by category, and the
CLI's `info --drivers` MUST surface the same data.

#### Scenario: Manifest is the single source of truth for the catalog

- **WHEN** a driver is added, removed, or its status changes
- **THEN** `drivers/MANIFEST.json` is updated in the same change
- **AND** `docs/DRIVERS.md` and the `alloy info --drivers` output
  reflect the manifest content
- **AND** divergence between manifest and docs is treated as a bug

#### Scenario: Status change requires evidence

- **WHEN** a driver moves from `compile-review` to `hardware-validated`
- **THEN** the change references a runbook or test note showing the
  driver was run on real hardware (board, chip serial / part number,
  observed output)
- **AND** the support matrix is updated in the same change if the
  promotion affects the driver's class tier

