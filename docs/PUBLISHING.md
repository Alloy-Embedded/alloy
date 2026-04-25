# Publishing An Alloy Release

This document is the maintainer runbook. End users do not need to read it; they
install the CLI with `pipx install alloy-cli` and the SDK with
`alloy sdk install <version>`.

## What gets published

Each release ships three things, in this order:

1. **`alloy-cli`** to PyPI -- the user-facing entry point. Built from
   [`tools/alloy-cli/`](../tools/alloy-cli/) and uploaded by the
   [`Release` workflow](../.github/workflows/release.yml).
2. **The runtime tag** on this repository -- `git tag vX.Y.Z` here. The tag is
   the source of truth for `alloy sdk install vX.Y.Z`; the SDK manager git-clones
   this tag straight into `~/.alloy/sdk/vX.Y.Z/runtime/`.
3. **The matching `alloy-devices` SHA**, recorded in
   [`docs/RELEASE_MANIFEST.json`](RELEASE_MANIFEST.json) under
   `alloy_devices.ref`. Users do not pin this themselves -- the SDK installer
   reads it from the runtime checkout immediately after cloning, so installs are
   reproducible across machines.

## One-time setup (PyPI trusted publisher)

The release workflow uses PyPI's OIDC trusted publishing flow, so no API token
is stored anywhere. To enable it:

1. Open <https://pypi.org/manage/account/publishing/> and add a *pending*
   trusted publisher with these fields:
   - PyPI project name: `alloy-cli`
   - Owner: `Alloy-Embedded`
   - Repository: `alloy`
   - Workflow: `release.yml`
   - Environment: `pypi`
2. In this repo's *Settings → Environments*, create an environment named `pypi`.
   No secrets, no required reviewers; the OIDC binding does the auth.

After that, every `vX.Y.Z` tag pushed to `main` publishes the matching wheel
automatically.

## Cutting a release

The CLI's package version is derived from the git tag by `hatch-vcs`. Maintainers
do not edit a version field anywhere; the only thing that matters is the tag.

```bash
# 1. (If the alloy-devices SHA changed since the previous release) update
#    the manifest. The CLI uses this for `alloy sdk install` reproducibility.
$EDITOR docs/RELEASE_MANIFEST.json              # alloy_devices.ref

# 2. Commit any pending changes, tag, push.
git add -A
git commit -m "release: alloy v0.2.0"
git tag -a v0.2.0 -m "alloy v0.2.0"
git push origin main v0.2.0
```

Building from the tag produces `alloy_cli-0.2.0-...whl`. Building from a dirty
working tree or between tags produces a PEP 440 dev version such as
`0.2.1.dev3+g1abcdef.d20260425`, which `pip` will not consider a release. That is
the safety net: the workflow can never publish a wrong-version wheel, because the
version is what it is.

The `Release` workflow then runs in parallel:

- builds the foundational firmware artifacts for STM32G0/F4/F7,
- runs the host test suite,
- generates a changelog from `git log v0.1.0..v0.2.0`,
- builds the alloy-cli wheel and publishes it to PyPI,
- creates the GitHub Release with the firmware tarballs *and* the wheel as
  release assets.

## Verifying after release

```bash
# 1. PyPI took the wheel:
pip index versions alloy-cli                    # should list v0.2.0

# 2. Fresh-machine install works:
pipx install alloy-cli==0.2.0
alloy --version                                  # alloy 0.2.0

# 3. SDK install picks up the tag and the manifest-pinned devices SHA:
alloy sdk install v0.2.0
alloy sdk path                                   # ~/.alloy/sdk/v0.2.0/runtime
cat ~/.alloy/sdk/v0.2.0/manifest.toml            # should record both SHAs

# 4. Full smoke:
alloy new ~/projects/release-smoke --board nucleo_g071rb
cd ~/projects/release-smoke
cmake --preset debug && cmake --build --preset debug
```

## Rolling back

If a release ships broken, do not delete the tag. Instead:

```bash
# Yank the wheel from PyPI (still in the index, but blocked from new installs):
twine yank alloy-cli==0.2.0 --reason "regression in <area>; use 0.2.1"

# Cut a fix release:
$EDITOR tools/alloy-cli/pyproject.toml          # version = "0.2.1"
git tag -a v0.2.1 -m "alloy v0.2.1: hotfix"
git push origin main v0.2.1
```

`alloy sdk install v0.2.0` will continue to work for users who already pinned
to that tag; the yank only affects new `pip` / `pipx` installs of the CLI.

## What the user actually sees

Once everything above is in place, the user experience reduces to:

```bash
pipx install alloy-cli                          # one-time
alloy sdk install v0.2.0                        # fetches runtime + devices
alloy toolchain install arm-none-eabi-gcc       # one-time per host
alloy new ./blink-g071 --board nucleo_g071rb
cd blink-g071
cmake --preset debug && cmake --build --preset debug
```

No clones, no `--alloy-root`, no `ALLOY_ROOT` exports.
