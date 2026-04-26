# board-bringup Specification

## Purpose
Boards are declarative, thin shims over the runtime.  A board declares which physical resources
(pins, peripheral instances, clock profiles) map to which logical roles, and calls
`board::init()` to bring the hardware up through the descriptor-driven runtime path.

Board headers expose typed helpers (`make_uart()`, `make_spi()`, etc.) backed by the public
HAL API.  They do not contain raw register sequences, handwritten startup glue, or
family-private APIs.  Board-level code that a user reads is a table of hardware choices, not
a driver.
## Requirements
### Requirement: Boards Shall Be Declarative

Boards SHALL declare local hardware choices and bring-up policy on top of shared runtime
primitives.

#### Scenario: Board LED declaration
- **WHEN** a board exposes an LED resource
- **THEN** it declares the selected pin and polarity
- **AND** it does not embed raw register initialization logic in the board header

### Requirement: Board Initialization Shall Use Runtime Bring-Up

`board::init()` SHALL orchestrate runtime bring-up using descriptor-driven runtime services.

#### Scenario: Foundational board initialization
- **WHEN** a foundational board initializes
- **THEN** clocks, startup services, connector setup, and default resources are brought up through
  the runtime path
- **AND** not through board-local raw register sequences

### Requirement: Canonical Examples Shall Follow the Official Runtime Path

Examples intended for users SHALL use the same public runtime path that production code is
expected to use.

#### Scenario: UART logger example
- **WHEN** the UART logger example is built
- **THEN** it configures UART through the public runtime API
- **AND** it does not bypass the runtime with raw register access

### Requirement: The Build SHALL Accept A Board Declared Outside The Runtime Tree

The runtime SHALL provide a documented bring-up path through which a consuming project
declares its own board outside the runtime tree without bypassing the descriptor-driven
runtime services or the public board contract.

#### Scenario: User project declares a custom board
- **WHEN** a downstream project sets `ALLOY_BOARD=custom` and provides the documented
  custom-board cache variables before adding alloy as a subproject
- **THEN** the runtime resolves the device contract, clocks, startup, and connectors
  for the supplied vendor/family/device tuple through the same descriptor-driven path
  used by in-tree boards
- **AND** the user-supplied board header and linker script are consumed without the
  runtime injecting any board logic of its own

### Requirement: The Custom Board Path SHALL Validate Its Inputs

The custom-board bring-up path SHALL fail fast at configure time when its contract is
not satisfied, with diagnostics that name the offending input.

#### Scenario: Missing required variable
- **WHEN** the custom-board branch is selected and a required cache variable is unset
- **THEN** configure fails with a single error line that names the missing variable
- **AND** descriptor resolution is not attempted

#### Scenario: Descriptor missing for declared device
- **WHEN** the custom-board branch is selected and the supplied vendor/family/device
  tuple has no descriptor under `ALLOY_DEVICES_ROOT`
- **THEN** configure fails with an error that names the missing descriptor path
- **AND** points the user at `alloy-devices` as the place to add support

#### Scenario: Invalid architecture or path inputs
- **WHEN** the user supplies `ALLOY_DEVICE_ARCH` outside the accepted set or a relative
  path for the board header or linker script
- **THEN** configure fails with an error that names the rejected value and the accepted
  values or expected form

### Requirement: The Custom Board Path SHALL Not Diverge From The In-Tree Bring-Up Contract

The custom-board branch SHALL emit the same set of outputs and use the same downstream
device-contract pipeline as in-tree foundational boards.

#### Scenario: Custom board uses the same device contract
- **WHEN** the custom-board branch and an in-tree branch resolve to the same
  vendor/family/device tuple
- **THEN** the device contract, startup behavior, and clock services produced for the
  custom board are identical to those produced for the in-tree board
- **AND** the runtime does not gate features on whether the board lives in the runtime
  tree

### Requirement: Dual-Core Boards SHALL Expose A Secondary-Core Launch Primitive

Boards whose target MCU has more than one core MUST expose a typed primitive
that releases the secondary core into a caller-supplied entry function. The
primitive MUST be opt-in: applications that ignore it MUST continue to
boot and run normally on the primary core. The primitive MUST NOT be called
automatically from `Reset_Handler`.

#### Scenario: ESP32 classic exposes board::start_app_cpu

- **WHEN** an application targets `boards/esp32_devkit/`
- **THEN** the board declares `void board::start_app_cpu(void (*fn)())`
- **AND** invoking it writes the trampoline address to `DPORT.APPCPU_CTRL_D`,
  enables the APP_CPU clock gate via `DPORT.APPCPU_CTRL_B` bit 0,
  un-stalls via `DPORT.APPCPU_CTRL_C` bit 0 = 0, and pulses reset via
  `DPORT.APPCPU_CTRL_A` bit 0
- **AND** the secondary-core trampoline initialises Xtensa register
  windows, sets `a1` to `_appcpu_stack_top`, and `call0`s a thunk that
  invokes the caller-supplied `fn`
- **AND** an application that never calls `start_app_cpu` boots normally
  on PRO_CPU with no APP_CPU activity

#### Scenario: RP2040 exposes board::launch_core1

- **WHEN** an application targets `boards/raspberry_pi_pico/`
- **THEN** the board declares `void board::launch_core1(void (*fn)())`
- **AND** the implementation performs the SIO FIFO 5-word handshake
  (flush ×2, vector_table = 0, stack_top = `_core1_stack_top`,
  entry = `fn`) and blocks until the ROM acknowledges
- **AND** `boards/raspberry_pi_pico/rp2040.ld` reserves a `.core1_stack`
  NOLOAD section sized for the second core's stack with
  `_core1_stack_top` exported as the symbol the handshake uses

#### Scenario: Single-core boards do not expose the primitive

- **WHEN** an application targets a single-core board (SAME70 Xplained,
  STM32 Nucleo, AVR-DA Curiosity Nano, ESP32-C3 DevKitM, ESP32-S3 DevKitC
  in single-core configuration)
- **THEN** the board does NOT declare a secondary-core launch primitive
- **AND** the build does not link any code that touches secondary-core
  control registers

### Requirement: Dual-Core Examples SHALL Compile Standalone Without The Descriptor Pipeline

Dual-core demonstration examples MUST be buildable from a minimal source
tree that exercises only the multi-core launch primitive plus raw MMIO,
WITHOUT requiring the alloy-devices descriptor pipeline outputs
(`selected_config.hpp`, generated startup, generated linker include) to
be present. This guarantees the SMP primitives can be exercised on
boards whose descriptor pipeline is still partial.

#### Scenario: rp2040_dual_core builds with only board_multicore.hpp

- **WHEN** `examples/rp2040_dual_core/main.cpp` is compiled
- **THEN** it includes `boards/raspberry_pi_pico/board_multicore.hpp`
  (a header that declares only `board::launch_core1`) — NOT
  `boards/raspberry_pi_pico/board.hpp`
- **AND** the build does not require `alloy/device/selected_config.hpp`
  to be generated
- **AND** the example links against `boards/raspberry_pi_pico/startup.cpp`
  + `board_multicore.cpp` only

#### Scenario: esp32_dual_core builds with only board_uart_raw.hpp + board.hpp

- **WHEN** `examples/esp32_dual_core/main.cpp` is compiled
- **THEN** the example links against the ESP32 board startup and the raw
  UART helpers without depending on a descriptor-derived runtime layer
  for the multi-core path itself
- **AND** the example exercises `board::start_app_cpu` and
  `alloy::tasks::CrossCoreChannel` end-to-end

### Requirement: ESP32 Classic Boards SHALL Follow The Same Declarative Contract As Other Boards

The `esp_wrover_kit` and `esp32_devkit` boards SHALL be declared with the same
manifest shape (vendor, family, device, arch, MCU, flash size, board header path,
optional linker script) used by every other in-tree board. The arch MUST be
`xtensa-lx6` (distinct from `xtensa-lx7` used by ESP32-S2/S3) so consumers can
branch on the variant without parsing MCU strings.

#### Scenario: ESP32 classic board declared in manifest

- **WHEN** the board manifest is queried for `esp_wrover_kit` or `esp32_devkit`
- **THEN** the manifest returns vendor `espressif`, family `esp32`, device `esp32`
  (or `esp32-wroom32` for the WROVER variant), arch `xtensa-lx6`, and the board
  header path under `boards/<name>/board.hpp`
- **AND** the entry follows the declarative shape; no custom CMake logic is required

#### Scenario: ESP32 classic boards do not silently change tier

- **WHEN** the catalog gains the ESP32-classic entries
- **THEN** `docs/SUPPORT_MATRIX.md` records both at a `compile-only` tier and
  explicitly does not claim hardware validation
- **AND** any future tier change for these boards must update both the catalog
  and the support matrix in the same change

### Requirement: ESP32 Classic Boards SHALL Expose The Secondary-Core Launch Primitive Opt-In

ESP32 classic boards SHALL expose `board::start_app_cpu(void(*fn)())` as the
opt-in primitive for releasing APP_CPU. The primitive SHALL NOT be invoked
automatically from `Reset_Handler`; applications that ignore it boot normally
on PRO_CPU. The data-driven version of this primitive (sourced from descriptor
`AppCpuControlPlane` typed register ids) is a follow-up tracked by the
alloy-codegen change `expose-xtensa-dual-core-facts`.

#### Scenario: Secondary-core launch primitive is declared per-board

- **WHEN** an application targets `boards/esp32_devkit/` or
  `boards/esp_wrover_kit/`
- **THEN** the board declares `void board::start_app_cpu(void (*fn)())`
- **AND** the primitive's body is currently sourced from board-private register
  constants (DPORT.APPCPU_CTRL_*); migrating those constants behind a typed
  descriptor surface is the explicit out-of-scope follow-up

