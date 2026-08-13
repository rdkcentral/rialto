---
description: Use when working on Rialto Server Manager code or tests
applyTo: serverManager/**
---

# Server Manager Instructions

## Architectural Authority

- `serverManager/` architecture is verified and should be treated as the source
  of truth for Server Manager behavior.
- Prefer existing Server Manager code patterns over generic architecture notes
  if there is any mismatch.
- Before significant lifecycle or recovery changes, review:
  - `serverManager/architecture-brief.md`
  - `serverManager/SME-notes.md`

## Implementation Guidance

- Keep process orchestration and lifecycle logic inside `serverManager/`.
- Avoid leaking Server Manager-specific concerns into unrelated modules.
- Reuse existing configuration and IPC patterns in `serverManager/config/`
  and `serverManager/ipc/`.
- Preserve stable and diagnosable logging around healthcheck, threshold, and
  recovery paths; downstream prompts and skills depend on these markers.

## Testing Expectations

- When changing Server Manager behavior, run relevant unit tests in:
  `tests/unittests/serverManager/`.
- Add or update tests for lifecycle, IPC, and failure-path changes.
- Include tests for shutdown race and healthcheck threshold recovery behavior
  when those paths are modified.
