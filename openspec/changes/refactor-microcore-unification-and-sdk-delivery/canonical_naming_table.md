# Canonical Naming Table

This table defines canonical identifiers for build/runtime integration during the migration window.

## CMake Variables

| Domain | Canonical | Legacy Alias | Notes |
|---|---|---|---|
| Project root | `MICROCORE_ROOT` | `ALLOY_ROOT` | Alias remains for compatibility. |
| Board | `MICROCORE_BOARD` | `ALLOY_BOARD` | Canonical board selector for CLI/CMake. |
| Platform | `MICROCORE_PLATFORM` | `ALLOY_PLATFORM` | Canonical HAL platform selector. |
| MCU | `MICROCORE_MCU` | `ALLOY_MCU` | Set by board config. |
| Architecture | `MICROCORE_ARCH` | `ALLOY_ARCH` | Set by board config. |
| Build tests | `MICROCORE_BUILD_TESTS` | `ALLOY_BUILD_TESTS` | Legacy variable deprecated. |
| Minimal build | `MICROCORE_MINIMAL_BUILD` | `ALLOY_MINIMAL_BUILD` | Legacy variable deprecated. |
| HAL library target variable | `MICROCORE_HAL_LIBRARY` | `ALLOY_HAL_LIBRARY` | CMake variable alias maintained. |
| Platform source dir | `MICROCORE_PLATFORM_DIR` | `ALLOY_PLATFORM_DIR` | Alias exported for old modules/tests. |

## Compile Definitions

| Domain | Canonical | Legacy Alias | Notes |
|---|---|---|---|
| Board define | `MICROCORE_BOARD_<BOARD>` | `ALLOY_BOARD_<BOARD>` | Both emitted during migration. |
| Platform define | `MICROCORE_PLATFORM_<PLATFORM>` | `ALLOY_PLATFORM_<PLATFORM>` | Both emitted during migration. |
| RTOS enable | `UCORE_RTOS_ENABLED` | `ALLOY_RTOS_ENABLED` | Legacy define still emitted where needed. |

## Runtime / Namespace

| Domain | Canonical | Legacy Alias | Notes |
|---|---|---|---|
| Root namespace | `ucore::` | `alloy::` (historical docs/code) | New code should use `ucore::`. |
| Generated namespace | `ucore::generated::<mcu>` | older `alloy::generated::<mcu>` docs | Canonical is `ucore::generated`. |

## Public CMake Target Terms

| Domain | Canonical Term | Current Internal |
|---|---|---|
| HAL library term | `microcore::hal` (target contract term) | `microcore_hal` (`alloy-hal` legacy alias) |
| Board target term | `microcore::board::<board>` (target contract term) | `microcore_board_<board>` |
| RTOS target term | `microcore::rtos` (target contract term) | `microcore_rtos` |

`microcore::*` target names are canonical contract terms for the SDK-facing direction. Legacy aliases (`alloy::hal`, `alloy::board::<board>`, `alloy::rtos`, and `alloy-hal`) remain available during migration.
