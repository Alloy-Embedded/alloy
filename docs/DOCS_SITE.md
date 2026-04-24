# Docs Site

The public docs site is a MkDocs Material build of the existing `docs/*.md` tree.

## Local Build

```bash
python3 -m pip install mkdocs-material
mkdocs build --strict
```

`--strict` fails on broken internal links or missing nav entries. The build output lands under
`site/` (gitignored).

## Local Preview

```bash
mkdocs serve
```

Serves the site at <http://127.0.0.1:8000/> with live reload.

## Release Gate

`scripts/check_docs_site.py` runs `mkdocs build --strict` and is registered as the `docs-site`
release gate in [RELEASE_MANIFEST.json](RELEASE_MANIFEST.json). Any broken link, missing nav
entry, or build failure fails the gate.

## Publish

`.github/workflows/docs.yml` builds and deploys to GitHub Pages on push to `main`. The workflow
uses the same `mkdocs build --strict` command as the gate, so the live site never diverges from
what the gate accepts locally.

## Navigation Structure

- **User Guide** — quickstart, board tooling, cookbook, support matrix, CMake consumption,
  migration guide (the first surface a new adopter sees)
- **Reference** — runtime async model, runtime device boundary, porting a new board / platform
- **Internals** — architecture, code generation, release discipline and checklist, runtime
  cleanup audit, this page

See [mkdocs.yml](https://github.com/lgili/alloy/blob/main/mkdocs.yml) at the repo root for the
exact `nav:` declaration.
