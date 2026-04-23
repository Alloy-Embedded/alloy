## Context

The repo already has unusually strong validation for its stage, but it still lacks a public
production contract.

Users need explicit answers to questions like:

- which boards are tier-1 versus experimental?
- which peripheral classes are supported where?
- which `alloy-devices` publish is compatible with this `alloy` release?
- what must pass before a release can claim zero-overhead and runtime-boundary integrity?

This change makes those answers first-class.

## Goals

- explicit support tiers
- explicit compatibility policy with `alloy-devices`
- explicit release gates
- explicit migration discipline for breaking changes

## Non-Goals

- inventing enterprise process overhead
- promising support for boards not covered by validation
- freezing public APIs before they are documented and tested

## Decision 1: Support Tiers Must Be Explicit

Boards and peripheral classes SHALL have explicit support tiers, for example:

- foundational / tier-1
- experimental
- deprecated

The exact names may change, but the tiers must be documented.

## Decision 2: Release Claims Must Follow Validation Reality

Release notes and support tables SHALL only claim support that matches the validation matrix.

If a board or peripheral is not covered by the declared release gate, it must not be described as
equally supported.

## Decision 3: Runtime/Devices Compatibility Must Be Declared

Each `alloy` release SHALL declare the compatible `alloy-devices` contract range or version policy.

This keeps the runtime/device boundary operational as a product boundary.

## Decision 4: Breaking Changes Need Migration Notes

Any breaking change in:

- public HAL shape
- build/selection story
- board migration path
- runtime/device contract expectations

SHALL include migration notes and at least one updated example.

## Decision 5: Zero-Overhead Needs Budgets, Not Just Intent

Zero-overhead claims SHALL be backed by explicit regression gates such as:

- assembly checks
- binary-size budgets
- focused hot-path checks

## Validation

At minimum this change must add:

- documented support tiers
- compatibility declaration checks
- release checklist docs
- CI enforcement for foundational release gates
