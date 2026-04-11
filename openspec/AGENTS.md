# OpenSpec Instructions

Instructions for AI coding assistants using OpenSpec for spec-driven development.

## TL;DR Quick Checklist

- Search existing work: `openspec spec list --long`, `openspec list`
- Decide scope: new capability vs modify existing capability
- Pick a unique `change-id`: kebab-case, verb-led (`add-`, `update-`, `remove-`, `refactor-`, `rebuild-`)
- Scaffold: `proposal.md`, `tasks.md`, `design.md` (when needed), and delta specs per affected capability
- Write deltas: use `## ADDED|MODIFIED|REMOVED Requirements`; include at least one `#### Scenario:` per requirement
- Validate: `openspec validate [change-id] --strict`
- Do not start implementation before the proposal is reviewed

## Three-Stage Workflow

### Stage 1: Creating Changes
Create a proposal when the request:
- adds or removes public capabilities
- changes architecture or ownership boundaries
- changes build selection, board model, startup model, or code generation contracts
- changes public API shape or migration path
- introduces multi-vendor scaling work

Skip proposal for:
- bug fixes that restore intended behavior
- comments, formatting, or typo fixes
- tests for already-specified behavior

Workflow:
1. Read `openspec/project.md`
2. Run `openspec list` and `openspec list --specs`
3. Choose a unique change id
4. Draft `proposal.md`, `tasks.md`, optional `design.md`, and delta specs
5. Run `openspec validate <change-id> --strict`

### Stage 2: Implementing Changes
1. Read `proposal.md`
2. Read `design.md` if present
3. Read `tasks.md`
4. Implement tasks in order
5. Keep the checklist honest
6. Do not mark tasks complete before code, tests, and docs match reality

### Stage 3: Archiving Changes
After deployment:
- archive `changes/<change-id>/`
- update permanent `specs/` if needed
- run `openspec validate --strict`

## Before Any Task

- Read `openspec/project.md`
- Check active changes in `openspec/changes/`
- Prefer modifying an existing capability over creating duplicates
- Use `openspec show <item>` when context is ambiguous

## Directory Structure

```text
openspec/
├── project.md
├── specs/
│   └── <capability>/
│       └── spec.md
└── changes/
    └── <change-id>/
        ├── proposal.md
        ├── design.md
        ├── tasks.md
        └── specs/
            └── <capability>/
                └── spec.md
```

## Spec File Format

Use:
- `## ADDED Requirements`
- `## MODIFIED Requirements`
- `## REMOVED Requirements`

Every requirement must have at least one scenario:

```markdown
#### Scenario: Successful bring-up
- **WHEN** ...
- **THEN** ...
```

Use SHALL or MUST for normative requirements.
