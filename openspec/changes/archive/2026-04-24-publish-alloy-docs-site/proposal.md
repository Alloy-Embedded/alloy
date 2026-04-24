## Why

The repo ships 15 markdown files under `docs/` that already cover quickstart, board tooling,
cookbook, support matrix, runtime async model, architecture, and release discipline. There is no
hosted docs site: users have to open files on GitHub, and the user-facing docs are mixed with
internal design notes (architecture, release discipline, runtime cleanup audit, release manifest).

Competing HAL/runtime projects (Zephyr, ESP-IDF, PlatformIO, Arduino, embedded-hal) all ship a
browsable docs site. Without one, discoverability for new adopters is behind the competition and
"is this project real?" signal is weaker than the work actually justifies.

## What Changes

- Add `mkdocs.yml` at the repo root using the `mkdocs-material` theme (markdown-native; keeps
  existing `docs/*.md` as source of truth).
- Split the navigation into user-facing and internal sections so new adopters land on
  quickstart, cookbook, support matrix, and the async model write-up first.
- Add `docs/index.md` as the site landing page.
- Add a GitHub Pages publishing workflow that builds and deploys on push to `main`.
- Add a `docs-site` release gate script that runs `mkdocs build --strict` locally so broken
  links and missing pages fail CI instead of landing on prod.
- Document the docs-site build and deploy flow in `docs/BOARD_TOOLING.md` or a dedicated
  `docs/DOCS_SITE.md`.

## Outcome

A browsable https://... docs site is live, the navigation clearly separates user guide from
internal notes, and a release gate prevents the site from silently breaking.

## Impact

- Affected specs:
  - `runtime-tooling`
- Affected code and docs:
  - `mkdocs.yml` (new)
  - `docs/index.md` (new)
  - `docs/DOCS_SITE.md` (new)
  - `.github/workflows/docs.yml` (new)
  - `scripts/check_docs_site.py` (new)
  - `docs/RELEASE_MANIFEST.json` (new `docs-site` gate)
