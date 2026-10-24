---
description: Use when working on Rialto unit/component tests
applyTo: tests/**
---

# Test Code Instructions

- Follow existing gtest patterns and fixture style in nearby test files.
- Keep production naming and formatting conventions unless test-specific
  flexibility is already established in the directory.
- For tests, cpplint namespace restrictions are relaxed by `tests/CPPLINT.cfg`.
- Prefer focused tests for changed behavior rather than broad refactors.
- When behavior changes, update/add tests in the nearest corresponding test suite.
- For Server Manager-adjacent tests, align assertions with:
  - `serverManager/architecture-brief.md` canonical state and transition behavior
  - `serverManager/SME-notes.md` known incident signatures and regression checks
