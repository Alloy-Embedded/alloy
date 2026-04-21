# Hardware Spot-Check Strategy

`tests/hardware/` owns the silicon-validation runbooks for Alloy.

These checks answer the last question that compile smoke, host MMIO, ELF inspection, and Renode
cannot answer alone:

- does the representative board actually boot on silicon and expose the expected external behavior?

## Scope

Hardware validation is representative, not exhaustive.

The initial board set is:

| Family | Board | Mandatory smoke | Extended smoke | Primary observables |
|---|---|---|---|---|
| Microchip SAME70 | `same70_xplained` | `blink`, `uart_logger` | `dma_probe` | LED, debug UART, DMA binding log |
| STM32G0 | `nucleo_g071rb` | `blink`, `uart_logger` | `watchdog_probe`, `rtc_probe`, `timer_pwm_probe`, `analog_probe` | LED, ST-LINK virtual COM |
| STM32F4 | `nucleo_f401re` | `blink`, `uart_logger` | `watchdog_probe`, `rtc_probe`, `timer_pwm_probe`, `analog_probe`, `dma_probe` | LED, ST-LINK virtual COM, DMA binding log |

## Execution Model

Hardware runbooks are board-lab procedures, not default CI tests.

When a board family has enough public examples, it should expose one build target that assembles
the full firmware bundle for the runbook. This is a build convenience only; flashing and pass/fail
observation still remain manual lab steps.

Every runbook follows the same flow:

1. Configure a board-specific build directory with the ARM toolchain.
2. Build the firmware targets listed in the runbook.
3. Flash the generated `.elf`/`.hex`/`.bin` with the probe flow already used in the lab.
4. Observe LED and UART behavior.
5. Record pass/fail and any anomalies in the PR, issue, or lab notebook.

Representative configure pattern:

```bash
cmake -S . -B build/hw/<board> \
  -DALLOY_BOARD=<board> \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
```

## Acceptance Rules

### `blink`

- board reaches `main()` and `board::init()`
- onboard LED begins a visible 1 Hz blink within roughly two seconds after reset
- no stuck-off/stuck-on state after repeated resets

### `uart_logger`

- UART opens at `115200 8N1`
- `uart logger ready` appears once after boot
- `heartbeat loop=<n>` continues at roughly one line per second
- LED continues to toggle while UART traffic is active

### `dma_probe`

- `dma probe ready` appears after boot
- both TX and RX binding lines print once
- no obvious hard-fault/reset loop after DMA configuration

## Failure Triage

Use the layered validation ladder to localize failures:

- `blink` fails but compile/host MMIO pass:
  - suspect startup, clock tree, reset release, or board pin binding
- `blink` passes but `uart_logger` fails:
  - suspect UART route, clock gating, baud configuration, or board debug-UART helper
- `uart_logger` passes but `dma_probe` fails:
  - suspect DMA binding metadata, channel selection, or DMA runtime integration
- hardware fails while Renode passes:
  - suspect silicon-specific clock/reset assumptions, board wiring, or missing real-world delay requirements

## Deliberate Non-Goals

- no fake "hardware CTest" that pretends to flash boards from arbitrary developer machines
- no exhaustive matrix across every board and vendor
- no duplication of `alloy-devices` facts in handwritten hardware scripts
