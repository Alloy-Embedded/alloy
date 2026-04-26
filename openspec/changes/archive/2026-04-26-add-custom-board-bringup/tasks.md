# Tasks: Custom Board Bring-Up Path

Tasks are ordered. Do not start a task before the previous one is reviewed.

## 1. Manifest contract
- [x] 1.1 Define the cache-variable contract in `cmake/board_manifest.cmake`:
      `ALLOY_CUSTOM_BOARD_HEADER`, `ALLOY_CUSTOM_LINKER_SCRIPT`,
      `ALLOY_DEVICE_VENDOR`, `ALLOY_DEVICE_FAMILY`, `ALLOY_DEVICE_NAME`,
      `ALLOY_DEVICE_ARCH`, optional `ALLOY_DEVICE_MCU`, `ALLOY_FLASH_SIZE_BYTES`
- [x] 1.2 Add the `BOARD_NAME STREQUAL "custom"` branch in
      `alloy_resolve_board_manifest` that reads the cache variables and emits the same
      output set as in-tree branches
- [x] 1.3 Validate every required variable is set; produce a one-line `FATAL_ERROR`
      naming the first missing variable
- [x] 1.4 Validate `ALLOY_DEVICE_ARCH` against the accepted enum
      (`cortex-m0plus`, `cortex-m4`, `cortex-m7`, `riscv32`, `xtensa`, `avr`, `native` --
      enum widened during impl to match in-tree boards)
- [x] 1.5 Reject relative paths for `ALLOY_CUSTOM_BOARD_HEADER` and
      `ALLOY_CUSTOM_LINKER_SCRIPT` with a clear error
- [x] 1.6 Validate that `${ALLOY_DEVICES_ROOT}/<vendor>/<family>/generated/runtime/devices/<device>` exists and fail fast otherwise

## 2. Configure-time test
- [x] 2.1 Add `tests/custom_board/` containing a synthetic external project that:
      declares a `board.hpp` and a placeholder linker script, reuses the
      `st/stm32g0/stm32g071rb` descriptor tuple, sets `ALLOY_BOARD=custom` and the
      required cache variables, calls `add_subdirectory(${ALLOY_ROOT})`
- [x] 2.2 Wire the test into a `scripts/check_custom_board.py` runner (matches the
      existing `check_downstream_package.py` pattern). CI integration in
      `.github/workflows/build.yml` is a follow-up once `alloy-devices` is available
      to that workflow.
- [x] 2.3 Add a negative test asserting that omitting `ALLOY_CUSTOM_BOARD_HEADER` fails
      at configure time with a diagnostic that names the variable

## 3. Documentation
- [x] 3.1 Add `docs/CUSTOM_BOARDS.md` explaining the contract end-to-end with a working
      example, paired with a pointer to `alloy new --mcu` for the CLI flow
- [x] 3.2 Update `docs/BOARD_TOOLING.md` to link to the custom-board recipe
- [x] 3.3 Cross-link the change from `docs/CMAKE_CONSUMPTION.md`
