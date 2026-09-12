---
description: Use when editing architecture documentation
applyTo: **/*architecture*.md
---

# Architecture Documentation Instructions

- Known issue: the overall architecture documentation may contain defects.
- Use these repository architecture sources before adding new claims:
  - `docs/architecture-brief.md`
  - `serverManager/architecture-brief.md`
  - `logging/architecture.md`
  - `serverManager/SME-notes.md` (operational and incident context)
- When editing architecture docs, cross-check claims against implementation,
  especially for `serverManager/`, `ipc/`, and `media/*/ipc` flows.
- If uncertain, prefer describing verified behavior from code over speculative
  or outdated statements.
- Keep architecture docs aligned with module boundaries used in this repo.
- If edits affect Server Manager lifecycle/healthcheck behavior, update both
  `docs/architecture-brief.md` and `serverManager/architecture-brief.md` or
  explicitly note why one is intentionally unchanged.
