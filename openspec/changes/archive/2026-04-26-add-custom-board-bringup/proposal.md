# Add Custom Board Bring-Up Path

## Why
Today, board selection in the runtime is closed: `cmake/board_manifest.cmake` hardcodes
the supported board names in an `if/elseif` chain. An unknown name resolves to "not
found", the device contract is not assembled, and the build fails before reaching the
runtime layer. This works for the in-tree foundational boards but blocks the principal
intended use of Alloy: a downstream user picking an MCU and writing a board for *their
own* hardware.

The project-scaffolding work (see `add-project-scaffolding-cli`) wants every scaffolded
project to own its board: the `board/` directory lives in the user's repository, the user
edits pin assignments freely, and the alloy runtime is consumed without modification.
That is consistent with the project's stated architecture (`board → public HAL → device
contract`) but requires the runtime to accept a board declared *outside* its tree.

This change introduces a single, narrow extension to the board manifest contract: a
custom-board bring-up path. The custom board is still declarative, still uses the
descriptor-driven runtime services, and still delegates clocks/startup/connectors to the
device contract. The only thing that changes is *where the board declaration lives*.

## What Changes
- Extend `cmake/board_manifest.cmake` with an explicit `ALLOY_BOARD=custom` resolution
  branch. When that branch is taken, the manifest reads its outputs (board header,
  linker script, vendor, family, device, arch, MCU, flash size) from a documented set
  of cache variables that the consuming project sets *before* `add_subdirectory(alloy)`.
- Define and document the public custom-board contract:
  - required: `ALLOY_CUSTOM_BOARD_HEADER`, `ALLOY_CUSTOM_LINKER_SCRIPT`,
    `ALLOY_DEVICE_VENDOR`, `ALLOY_DEVICE_FAMILY`, `ALLOY_DEVICE_NAME`,
    `ALLOY_DEVICE_ARCH`,
  - optional: `ALLOY_DEVICE_MCU`, `ALLOY_FLASH_SIZE_BYTES`,
  - validation: every required variable must be set; `ALLOY_DEVICE_VENDOR/FAMILY/NAME`
    must resolve to an existing descriptor tree under `ALLOY_DEVICES_ROOT`.
- Add a focused configure/compile test that exercises the custom-board branch against
  a known good descriptor (e.g., `st/stm32g0/stm32g071rb`) using a synthetic external
  board folder, so future changes cannot silently break the contract.
- Document the path in `docs/BOARD_TOOLING.md` (or a new `docs/CUSTOM_BOARDS.md`) so
  users who do not use the CLI scaffolder still have a supported recipe.

## Impact
- Affected specs:
  - `board-bringup` (extended with the custom-board contract requirement)
- Affected code:
  - `cmake/board_manifest.cmake` (new branch + cache-variable contract)
  - `cmake/alloy_devices.cmake` (no logic change; verifies that the custom branch's
    vendor/family/device flow through the existing descriptor pipeline unchanged)
  - new `tests/custom_board/` configure test
  - `docs/` (new or extended doc explaining the contract)
- Out of scope for this change:
  - generating a default `board/` skeleton in user projects (that is the CLI's job;
    tracked under `add-project-scaffolding-cli` task 5b)
  - per-vendor toolchain or linker generation tools (the user supplies the linker
    script for now; later iterations can derive it from descriptor data)
  - changing the in-tree foundational boards (they keep their dedicated branches in
    `alloy_resolve_board_manifest`)
