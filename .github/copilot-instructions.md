# Rialto Copilot Instructions

## Project Overview

Rialto is a C++ media stack with IPC, media client/server components, logging,
common utilities, wrappers, and Server Manager integration.

Primary top-level areas:
- `common/`: shared utilities and interfaces.
- `ipc/`: generic protobuf-over-unix-domain-socket IPC library.
- `proto/`: Rialto protocol definitions.
- `media/`: media client/server/common/public code.
- `serverManager/`: process/service orchestration and server-manager specific code.
- `tests/`: unit and component tests.
- `scripts/`: quality and validation tooling.

## Architecture

- Primary architecture references for this repository:
  - `docs/architecture-brief.md` for repository-level architecture and container boundaries.
  - `serverManager/architecture-brief.md` for verified Server Manager behavior, lifecycle, and healthcheck flow.
  - `serverManager/SME-notes.md` for operations guidance, incident context, and known failure signatures.
  - `logging/architecture.md` for logging sink, level, and routing behavior.
- Document precedence when guidance conflicts:
  1. Verified implementation in code.
  2. `serverManager/architecture-brief.md` for Server Manager behavior.
  3. `docs/architecture-brief.md` and module briefs.

- Respect module boundaries: keep IPC concerns in `ipc/` and `media/*/ipc`,
  shared infrastructure in `common/`, and media/server logic in `media/server/`.
- Treat `serverManager/` architecture and implementation as verified and
  authoritative.
- The overall architecture documentation may contain known defects.
  If architectural docs conflict with code in `serverManager/` or established
  module behavior, prefer the verified implementation and raise a note in your
  response.
- For IPC behavior and ownership boundaries, use `docs/IpcOverview.md` and
  code under `ipc/`, `media/client/ipc/`, and `media/server/ipc/` as references.
- For healthcheck/restart and shutdown reasoning, prefer log-marker expectations
  documented in `serverManager/architecture-brief.md` and `serverManager/SME-notes.md`.

## Coding Guidelines

Rialto follows Google C++ Style as a baseline, with these project-specific rules
(from Rialto Coding Guidelines):

### Naming and File Conventions
- Filenames use PascalCase (UpperCamelCase), no underscores.
- Type/class names use PascalCase.
- Function names use lowerCamelCase (including non-accessor methods).
- Variable and parameter names use lowerCamelCase (no underscores).
- Data members use `m_` prefix (for example: `m_mediaPlayer`).
- `const`/`constexpr` variables should start with `k`.
- `const` data members use `m_k...` naming when class-member scoped.
- Namespace names are lowercase with no underscores.
- Header guards follow uppercase underscore style ending with `_H_`.

### Formatting
- Use max line length of 120.
- Put opening curly braces on a new line.
- Do not indent access specifiers (`public:`, `private:`, `protected:`).
- Use spaces, not tabs (`UseTab: Never`, indent width 4).
- Keep pointer alignment as `Type *name` / `Type &name` (right-aligned symbols).

### Includes and Dependencies
- Prefer including project headers by filename only (no directory prefix),
  unless third-party include requirements need full paths.
- Keep include order and formatting compatible with `.clang-format`
  (`SortIncludes: true`).
- Avoid introducing new dependencies unless necessary and justified.

### Error Handling and Logging
- Do not use `iostream`, `cout`, or `cerr` in production code.
- Use project logging facilities instead of stdout/stderr streams.
- Avoid blanket exception handling; use specific handling where required.
- Constructor exceptions are acceptable only when the class has a factory path
  that can safely handle construction failures.

### File Header
- Preserve existing copyright/license header blocks in edited files.
- New C/C++ files should include the standard Rialto header used in nearby files.

### TODO and Comments
- TODO comments must use uppercase `TODO` with Jira ID format,
  for example: `TODO (LLDEV-23451): short description`.

## Validation

Before declaring work complete, run checks relevant to changed files.
From repository root:

1. Build targeted code paths and tests as needed:
   - `./build_ut.py` for unit tests.
   - `./build_ct.py` for component tests.
   - `./build_native.sh` for native build validation (when applicable).
2. Run source quality checks:
   - `./scripts/codeTests.py`
  - or scoped checks via flags (`--cpplint`, `--clang`, `--cppcheck`, etc.).
  - avoid merging if checks fail for tabs, `iostream/cout/cerr`, or constant naming checks.
3. If tests are impacted, ensure updated/related tests pass.
4. Do not claim completion if build/lint/tests fail.

## Change Scope and Safety

- Keep changes minimal and local to the task.
- Preserve existing public interfaces unless change is requested.
- Update tests when behavior changes.
- If guidance conflicts, apply the most specific relevant instruction:
  repository -> scoped `.instructions.md` -> file-local expectations.

## Prompt Authoring Resources

- For reusable prompt-writing patterns, see:
  `.github/prompts/05-prompt-fundamentals-cheatsheet.md`.
