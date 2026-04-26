## ADDED Requirements

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
