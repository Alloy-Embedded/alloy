## 1. OpenSpec Baseline

- [x] 1.1 Add runtime-tooling delta covering the hosted docs site surface

## 2. MkDocs Configuration

- [x] 2.1 Add `mkdocs.yml` with `mkdocs-material` theme
- [x] 2.2 Split navigation into User Guide / Reference / Internals
- [x] 2.3 Add `docs/index.md` landing page

## 3. Publishing And CI

- [x] 3.1 Add `.github/workflows/docs.yml` GitHub Pages workflow on push to `main`
- [x] 3.2 Add `scripts/check_docs_site.py` that runs `mkdocs build --strict`
- [x] 3.3 Register the new `docs-site` release gate in `docs/RELEASE_MANIFEST.json`

## 4. Docs

- [x] 4.1 Add `docs/DOCS_SITE.md` describing local build and publish flow

## 5. Validation

- [x] 5.1 Run `scripts/check_docs_site.py` locally
- [x] 5.2 `openspec validate publish-alloy-docs-site --strict`
