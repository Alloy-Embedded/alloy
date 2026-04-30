# Proposal: Declarative Board Manifest (JSON)

## Status
`open` — strategic priority #3.

## Problem

`cmake/board_manifest.cmake` is a flat `if/elseif` chain with 12 boards.
Adding a new board requires editing this file. With 5 000+ MCUs the chain
becomes unmaintainable and creates merge conflicts on every contribution.

Secondary problem: board capabilities (LED pins, debug UART, connectors, clock
profile) are duplicated between `board_manifest.cmake`, `alloyctl.py` (BOARDS
dict), and the board's `board_config.hpp`. Three sources of truth diverge.

## Proposed Solution

### board.json — the single source of truth

Each board directory contains a `board.json` that fully describes the board:

```json
{
  "$schema": "https://alloy-rs.dev/schemas/board-manifest/v1.json",
  "board_id": "nucleo_g071rb",
  "display_name": "Nucleo-G071RB",
  "vendor": "st",
  "family": "stm32g0",
  "device": "stm32g071rb",
  "arch": "cortex-m0plus",
  "linker_script": "STM32G071RBT6.ld",
  "board_header": "boards/nucleo_g071rb/board.hpp",
  "toolchain": "arm-none-eabi",
  "debug": {
    "probe": "stlink",
    "openocd_cfg": "interface/stlink.cfg target/stm32g0x.cfg",
    "stm32_programmer": true
  },
  "uart": {
    "debug": { "peripheral": "USART2", "tx": "PA2", "rx": "PA3", "baud": 115200 }
  },
  "serial_globs": ["/dev/cu.usbmodem*", "/dev/ttyACM*"],
  "leds": [
    { "name": "ld4", "pin": "PA5", "active_high": true }
  ],
  "buttons": [
    { "name": "b1", "pin": "PC13", "active_low": true }
  ],
  "clock_profiles": ["default_pll_64mhz"],
  "firmware_targets": ["blink", "time_probe", "uart_logger", "watchdog_probe",
                        "rtc_probe", "timer_pwm_probe", "analog_probe"],
  "bundle_target": "stm32g0_hardware_validation_bundle",
  "build_dir_suffix": "hw/g071",
  "mcuboot": {
    "primary_offset": "0x08008000",
    "primary_size_bytes": 45056,
    "secondary_offset": "0x08013000",
    "boot_size_bytes": 32768
  },
  "tier": 1,
  "notes": "Official ST Nucleo board; ST-LINK v3 onboard; VCP exposed on USB."
}
```

### CMake auto-discovery

```cmake
# cmake/board_manifest.cmake (new implementation)
function(alloy_resolve_board_manifest board_name)
    # Discover all board.json files under the boards/ directory
    file(GLOB_RECURSE _manifests
        "${CMAKE_SOURCE_DIR}/boards/*/board.json"
        "${CMAKE_SOURCE_DIR}/boards/custom/*/board.json"
    )
    foreach(_m ${_manifests})
        file(READ "${_m}" _json)
        # CMake 3.25+ json() functions
        string(JSON _id GET "${_json}" "board_id")
        if("${_id}" STREQUAL "${board_name}")
            # Extract all fields into parent-scope variables
            string(JSON ALLOY_DEVICE_SELECTED_VENDOR GET "${_json}" "vendor")
            string(JSON ALLOY_DEVICE_SELECTED_FAMILY GET "${_json}" "family")
            string(JSON ALLOY_DEVICE_SELECTED_NAME   GET "${_json}" "device")
            string(JSON ALLOY_DEVICE_SELECTED_ARCH   GET "${_json}" "arch")
            # ... etc
            return(PROPAGATE ALLOY_DEVICE_SELECTED_VENDOR ...)
        endif()
    endforeach()
    message(FATAL_ERROR "Unknown board: ${board_name}. Add a board.json under boards/.")
endfunction()
```

### alloyctl.py auto-discovery

The BOARDS dict in `alloyctl.py` is eliminated. `BoardConfig` is constructed by
parsing `board.json` at startup:

```python
def _discover_boards(root: Path) -> dict[str, BoardConfig]:
    boards: dict[str, BoardConfig] = {}
    for manifest in root.glob("boards/**/board.json"):
        data = json.loads(manifest.read_text())
        cfg = BoardConfig(
            board=data["board_id"],
            build_dir=root / "build" / data["build_dir_suffix"],
            bundle_target=data["bundle_target"],
            firmware_targets=tuple(data["firmware_targets"]),
            # ...
        )
        boards[data["board_id"]] = cfg
    return boards

BOARDS = _discover_boards(ROOT)  # replaces the hardcoded dict
```

### JSON Schema validation

A JSON Schema file at `cmake/schemas/board-manifest/v1.json` validates each
`board.json` at CMake configure time. Invalid manifests produce a clear error:

```
CMake Error: Invalid board manifest at boards/my_board/board.json:
  - Required field 'device' missing
  - Field 'arch' must be one of: cortex-m0plus, cortex-m4, cortex-m7, riscv32, xtensa, avr
```

### Custom board path

```cmake
# Custom board not in the alloy tree:
set(ALLOY_BOARD my_custom_board)
set(ALLOY_CUSTOM_BOARD_DIR /path/to/my_board)   # must contain board.json
```

`alloy_resolve_board_manifest()` also scans `ALLOY_CUSTOM_BOARD_DIR` before the
built-in boards. This preserves the existing custom board escape hatch.

### Board schema version

The `$schema` field carries the manifest schema version. Future breaking
changes increment the version; the CMake reader checks compatibility.

## Non-goals

- The board.json does not replace `board.hpp` / `board_config.hpp` (those remain
  the C++ API for firmware code).
- This spec does not define the alloy-cli board publishing workflow (that is
  `alloy-cli-distribution`).
