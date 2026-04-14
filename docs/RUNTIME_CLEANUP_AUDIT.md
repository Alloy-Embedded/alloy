# Runtime Cleanup Audit

## Goal

This audit captures what should be kept, rewritten, or deleted before the
descriptor-driven runtime continues deeper into `connect()`, claims, and driver
rebuilds.

The categories are:

- `keep`: already aligned with the target architecture
- `rewrite`: useful responsibility, wrong shape
- `delete`: conflicts with the new boundary or duplicates it

## Top-Level Audit

| Path | Status | Why | Next Action |
|---|---|---|---|
| [`src/core`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/core) | keep | generic runtime utilities still belong in `alloy` | keep pruning only if specific APIs are dead |
| [`src/device`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/device) | keep | this is the new import boundary | expand around published descriptors |
| [`src/arch`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/arch) | keep | target home for startup and architecture runtime | continue moving startup ownership here |
| [`src/hal/api`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/api) | delete | legacy split API tree has been removed from the active runtime path | keep docs/comments from recreating it |
| [`src/hal/core`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/core) | rewrite | contains useful concepts mixed with handwritten hardware knowledge | keep generic pieces, remove cross-vendor registries |
| [`src/hal/dma`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/dma) | rewrite | DMA now consumes typed runtime-lite bindings and driver semantics, but the runtime still needs a fuller transfer engine and cleanup of residual vendor-specific corners | keep converging on the same descriptor-driven execution model used by GPIO/UART/I2C/SPI |
| [`src/hal/interface`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/interface) | delete | old abstraction layer has been removed | keep docs and comments from recreating it |
| [`src/hal/deprecated`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/deprecated) | delete | explicitly obsolete | delete once no includes remain |
| [`src/hal/vendors`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/vendors) | rewrite then delete large parts | currently mixes temporary low-level helpers with public runtime behavior | preserve only small private adapters that survive descriptor-driven rebuild |
| [`src/startup`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/startup) | delete | startup ownership is moving to `src/arch` fed by `alloy-devices` | replace with shared `src/arch/cortex_m` runtime |
| [`boards`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards) | rewrite | boards should stay, but become declarative and side-effect safe | rebuild `board.hpp` and `board.cpp` around runtime APIs |
| [`examples`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples) | rewrite | examples still teach the wrong path in places | rebuild on the single public API |
| [`cmake/boards`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/cmake/boards) | delete gradually | legacy board CMake glues encode startup and codegen assumptions | replace with manifest-driven selection |
| [`cmake/platforms`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/cmake/platforms) | rewrite | still useful for toolchain and CPU flags, but not for family startup selection | keep only platform/toolchain concerns |

## High-Priority Rewrite Targets

These files or areas actively fight the target architecture:

| Path | Status | Reason |
|---|---|---|
| [`boards/same70_xplained/board_config.cpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_config.cpp) | delete | dead legacy SAME70 bring-up path with raw watchdog/PMC writes | remove it once no build references remain |
| [`src/hal/adc.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/adc.hpp) | delete | stale universal shim that points to the old platform split | remove it with the other dead top-level shims |
| [`src/hal/pwm.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/pwm.hpp) | delete | same as `adc.hpp` | remove it if no active include remains |
| [`src/hal/timer.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/timer.hpp) | delete | same as `adc.hpp` | remove it if no active include remains |
| [`examples/uart_logger/main.cpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/uart_logger/main.cpp) | keep | already uses `board::make_debug_uart()` and the logger sink on the runtime path | preserve it as the canonical UART example |

## Keep With Minimal Churn

These areas do not need broad redesign right now:

| Path | Why |
|---|---|
| [`src/core`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/core) | mostly runtime-generic support code |
| [`cmake/alloy_devices.cmake`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/cmake/alloy_devices.cmake) | already aligned with the target build boundary |
| [`cmake/board_manifest.cmake`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/cmake/board_manifest.cmake) | correct direction for compact board selection |
| [`tests/compile_tests/test_device_import_layer.cpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tests/compile_tests/test_device_import_layer.cpp) | good smoke target for foundational descriptor integration |

## Delete-First Candidates

These should not accumulate more usage:

- [`src/hal/deprecated`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/deprecated)
- obsolete top-level shims such as [`src/hal/adc.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/adc.hpp), [`src/hal/pwm.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/pwm.hpp), and [`src/hal/timer.hpp`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/timer.hpp)
- handwritten startup trees under [`src/startup`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/startup)
- new additions to [`src/hal/vendors`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/vendors) unless they are temporary private shims

## Cleanup Order

1. block new direct generated includes outside `src/device`
2. stop adding logic to handwritten signal and startup systems
3. rebuild `connect()` and claims on descriptors
4. rebuild foundational drivers
5. rewrite boards and canonical examples
6. delete dead legacy trees
