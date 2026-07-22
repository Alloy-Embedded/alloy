# alloy

A from-scratch C++23 framework for microcontrollers: one portable app source, any supported
board, compile-time safety, no RTOS required, no IDE required.

```cpp
// src/main.cpp — identical bytes on every supported board
#include <alloy/board.hpp>
using namespace alloy::literals;

int main() {
    board::init();
    while (true) {
        board::led.toggle();
        alloy::sleep_for(500ms);
    }
}
```

```console
$ alloy new blinky --board nucleo_g071rb && cd blinky
$ alloy run                      # build + flash + monitor
$ alloy board set same70_xplained
$ alloy run                      # same code, different MCU
```

## Layout

| Path            | What                                                              |
| --------------- | ----------------------------------------------------------------- |
| `src/alloy/`    | Hand-written C++: concepts, HAL drivers (one per peripheral IP version), arch support |
| `tools/alloy/`  | The Python CLI + code generator (single `alloy` package)          |
| `boards/`       | One `board.json` per board — data only, zero hand-written C++     |
| `examples/`     | Portable examples — zero preprocessor conditionals, enforced by CI |
| `cmake/`        | Internal CMake templates rendered by the CLI (users never see CMake) |
| `scripts/`      | Contract gates (`check_contract.sh`) and dev helpers              |

Device data (register maps per IP version, per-chip instances, pin routes, clock trees)
lives in the sibling [`alloy-devices`](../alloy-devices) repo and reaches this repo only
through generated headers.

Read [NORTH_STAR.md](NORTH_STAR.md) before contributing — it is the contract that keeps
this rebuild from repeating the old ecosystem's failures.

## Status

Greenfield rebuild in progress. Walking skeleton target: `blink` building end-to-end for
Nucleo-G071RB through the full data → codegen → HAL → CLI loop.

License: MIT OR Apache-2.0 (same as the previous alloy).
