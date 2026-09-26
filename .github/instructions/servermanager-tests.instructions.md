---
description: Use when working on Rialto Server Manager unit tests
applyTo: tests/unittests/serverManager/**
---

# Server Manager Test Instructions

- Keep test naming and fixture patterns consistent with nearby tests.
- Prefer targeted unit tests for behavior changes before broad integration changes.
- Cover lifecycle transitions, IPC interactions, and failure paths.
- Update or add tests whenever Server Manager behavior or contracts change.
- Use `serverManager/architecture-brief.md` transition matrix as baseline for
  expected state-flow behavior.
- Prioritize regression coverage for known risk areas from `serverManager/SME-notes.md`:
  - healthcheck timeout accumulation and threshold-triggered recovery
  - teardown/reconnect race during shutdown
  - cleanup correctness around inactive/deactivate paths
