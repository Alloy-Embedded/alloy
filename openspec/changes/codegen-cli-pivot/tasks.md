# Tasks: codegen-cli-pivot

---

## Phase 1 — Simplify alloy_codegen.py

- [x] 1.1 Remove sub-commands: `regen`, `diff`, `coverage`.
      Delete `cmd_regen`, `cmd_diff`, `cmd_coverage` functions entirely.

- [x] 1.2 Remove `_update_artifact_manifest()` and `_sha256_bytes()` helpers.

- [x] 1.3 In `cmd_add` (renamed `cmd_generate`):
      - Remove IR save (step 3: `ir_path.write_text(...)`)
      - Remove coverage report save (`rpt_path.write_text(...)`)
      - Remove `_update_artifact_manifest()` call (step 7)
      - Change output dir from `family_dir` (repo tree) to `args.out`
      - Keep coverage report print to stdout

- [x] 1.4 Replace subcommand CLI with flat `generate` CLI:
      - Removed `sub = p.add_subparsers(...)` scaffolding
      - Flags now on root parser: `--svd`/`--atdf` (mutually exclusive), `--vendor`,
        `--family`, `--device`, `--out` (required), `--patch`, `--strict`, `--dry-run`
      - Prog renamed to `alloy-gen`

- [x] 1.5 Remove now-unused imports: `hashlib`, `subprocess`, `json`, `os`.
      Remove `ROOT` global constant.

- [ ] 1.6 Rename entrypoint in `pyproject.toml` / `setup.cfg` from
      `alloy-codegen` → `alloy-gen` (if applicable).
      Note: no packaging file found yet — defer to alloy-cli-distribution openspec.

---

## Phase 2 — Missing critical emitter templates

The emitter currently has NO template for the most important generated files.
Without these, the CLI cannot produce a usable device package.

- [ ] 2.1 `codegen/templates/runtime/connectors.hpp.j2`:
      Already exists? Check. If not: generate `ConnectorTraits`, `ConnectorSignalTraits`,
      `ConnectorDescriptor`, Guard A + Guard B (from-GPIO), `kConnectors` array.

- [ ] 2.2 `codegen/templates/runtime/routes.hpp.j2`:
      `RouteId` enum, `RouteDescriptor`, `RouteKindId` enum, `kRoutes` array.

- [ ] 2.3 `codegen/templates/runtime/pins.hpp.j2`:
      `PinId` enum from IR pins list.

- [ ] 2.4 `codegen/templates/runtime/types.hpp.j2`:
      `PeripheralId`, `SignalId`, `ConnectionGroupId` enums.

- [ ] 2.5 Verify emitter plan in `alloy_cpp_emit/emitter.py` covers all files above.

---

## Phase 3 — Smoke test new CLI flow

- [ ] 3.1 Run `alloy-gen --svd STM32G071.svd --vendor st --family stm32g0 --device stm32g071rb --out /tmp/gen-test/`
      Verify: headers emitted, no IR/manifest files created, exit 0.

- [ ] 3.2 Run with bad SVD (non-existent file) → exit 1 with clear error.

- [ ] 3.3 Run with `--strict` on device with known missing patch fields → exits 1.

---

## Notes

- Patches directory stays inside the codegen package.
  `PATCHES_ROOT = Path(__file__).resolve().parent / "patches"` — unchanged.

- `alloy_ir_validate` coverage report still prints to stdout.
  Useful for the user to know what signal coverage their MCU has.
  Not saved to file.

- `cmd_add` function can be renamed to `cmd_generate` or `main_generate` once
  the subcommand scaffolding is removed.

- Do NOT remove `alloy_ir_validate` or `alloy_svd_ingest` packages — still used.
