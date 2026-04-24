## 1. OpenSpec Baseline

- [x] 1.1 Add runtime-tooling delta for the four new entry points

## 2. `alloyctl compile-commands`

- [x] 2.1 Emit or symlink `compile_commands.json` from the selected build dir to the repo root
- [x] 2.2 Document the flow in `docs/BOARD_TOOLING.md`

## 3. `alloyctl info`

- [x] 3.1 Print JSON with alloy version, pinned `alloy-devices` ref, board tiers, required gates,
      detected tool versions, git sha
- [x] 3.2 Cover JSON schema by a host test (`scripts/check_alloyctl_tooling_parity.py`)

## 4. `alloyctl doctor`

- [x] 4.1 Check cmake, arm-none-eabi-gcc, openocd, python deps, and `alloy-devices` ref alignment
- [x] 4.2 Non-zero exit on any failed check, with actionable hint per failure

## 5. `alloyctl new`

- [x] 5.1 Scaffold a starter firmware tree for one foundational board (CMake + blink surface)
- [x] 5.2 Include `docs/CMAKE_CONSUMPTION.md` pointer in the generated README

## 6. Docs

- [x] 6.1 Update `docs/BOARD_TOOLING.md` with the four new subcommands
- [x] 6.2 Mention `alloyctl doctor` in `docs/QUICKSTART.md`

## 7. Validation

- [x] 7.1 `openspec validate extend-alloyctl-tooling-parity --strict`
