## Why

Strong architecture and better docs are not enough to become the top embedded runtime. The repo
also needs production-grade discipline:

- explicit support tiers
- release gates
- version compatibility with `alloy-devices`
- binary and latency budgets
- migration notes for breaking changes

Without this, users cannot judge whether `alloy` is stable enough for real products.

## What Changes

- define support tiers and release criteria for boards and peripheral classes
- define compatibility policy between `alloy` and `alloy-devices`
- define release gates for foundational validation, examples, and zero-overhead budgets
- require migration notes and compatibility notes for breaking public changes
- publish a support matrix and release checklist

## Outcome

After this change, `alloy` should behave like a runtime that can be adopted deliberately:

- users know what is supported
- maintainers know what must pass before release
- breaking changes are managed instead of implied

## Impact

- Affected specs:
  - `runtime-release-discipline` (new)
  - `runtime-device-boundary`
  - `zero-overhead-runtime`
- Affected code and process:
  - CI/workflows
  - release docs
  - support matrix docs
  - compatibility checks
