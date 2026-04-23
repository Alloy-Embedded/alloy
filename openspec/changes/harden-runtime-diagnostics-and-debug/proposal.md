## Why

`alloyctl explain/diff` já existe, mas ainda está no primeiro corte. Para ser realmente melhor que
as libs atuais, falta um diagnóstico muito mais útil para:

- connector conflict
- clock/profile selection
- board recovery/debug
- migration between targets

Hoje a UX já não é ruim. Ainda não é uma vantagem decisiva.

## What Changes

- enriquecer o diagnóstico de connector/clock/resource
- adicionar alternatives mais fortes e diffs mais úteis
- endurecer a história de debug/recovery para boards fundacionais

## Outcome

Depois dessa change, o tooling deixa de ser só wrapper e vira uma parte forte do produto.

## Impact

- Affected specs:
  - `runtime-tooling`
  - `runtime-release-discipline`
- Affected code and docs:
  - `scripts/alloyctl.py`
  - `scripts/check_runtime_tooling.py`
  - `docs/BOARD_TOOLING.md`
  - `docs/QUICKSTART.md`
  - `docs/MIGRATION_GUIDE.md`
