# Agent Instructions

Follow these instructions when working in this repository. If a request conflicts
with these rules, explain the conflict and use the safest compliant approach.

## Project Overview

Rialto is a C++ media stack with protobuf-based IPC and distinct client/server
areas.

Primary modules:
- `common/`: shared interfaces and utility code.
- `ipc/`: reusable protobuf-over-unix-domain-socket IPC layer.
- `proto/`: Rialto protocol definitions.
- `media/`: media client/server/common/public implementation.
- `serverManager/`: server process lifecycle/orchestration and related IPC.
- `tests/`: unit and component tests.
- `scripts/`: lint, formatting, and validation helpers.

## Architecture Rules

- Keep responsibilities inside module boundaries.
- Treat `serverManager/` architecture and behavior as verified and authoritative.
- Overall architecture documentation may be stale or imperfect.
  If docs conflict with implementation, prefer verified behavior in code,
  especially under `serverManager/`, `ipc/`, and `media/*/ipc`.
- For IPC design context, use `docs/IpcOverview.md` and matching source paths.

## Important Commands

Run commands from repository root.

### Setup / Build

```bash
./build_native.sh
./build_ut.py
./build_ct.py
```

Windows-friendly invocation where needed:

```bash
python build_ut.py
python build_ct.py
python scripts/codeTests.py --help
```

### Testing and Validation

```bash
./build_ut.py                 # Unit tests
./build_ct.py                 # Component tests
./scripts/codeTests.py        # Combined quality checks
./scripts/codeTests.py -l     # cpplint
./scripts/codeTests.py -c     # clang-format dry-run check
./scripts/codeTests.py -p     # cppcheck
./scripts/codeTests.py -v     # valgrind via unit-test flow
```

## Coding and Style Rules

Rialto follows Google C++ style as a baseline with project exceptions.

- Filenames: PascalCase, no underscores.
- Classes/types: PascalCase.
- Functions: lowerCamelCase.
- Variables/parameters: lowerCamelCase (no underscores).
- Data members: `m_` prefix.
- Const/constexpr variables: start with `k`.
- Namespaces: lowercase, no underscores.
- Header guards: uppercase underscore format ending in `_H_`.

Formatting:
- Max line length: 120.
- Opening braces on a new line (Allman style).
- Access specifiers are not indented.
- Use spaces, not tabs (indent width 4).
- Keep pointer/reference alignment consistent with existing code style.

Includes and logging:
- Prefer including local project headers by filename only.
- Avoid `iostream`, `cout`, and `cerr` in production code.
- Use Rialto logging macros and facilities.

## Error Handling and Safety

- Avoid broad exception handling; catch specific exceptions.
- Constructor exceptions are acceptable when protected by a factory path
  that handles failures safely.
- Keep changes minimal and local to the requested scope.
- Do not change public interfaces unless required by the task.
- If behavior changes, update/add tests in the nearest corresponding suite.

## Test Conventions

- Use existing gtest fixture and naming patterns in nearby tests.
- For `tests/`, cpplint namespace restrictions are relaxed by
  `tests/CPPLINT.cfg`.
- Prefer targeted tests for changed behavior before broad refactors.

## File Organization

- Keep shared abstractions in `common/interface` and shared implementations in
  `common/source`.
- Keep IPC transport/protocol glue inside `ipc/` and module-specific IPC under
  `media/*/ipc` and `serverManager/ipc`.
- Keep `.proto` updates in `proto/` and ensure related client/server IPC code
  is updated together.
- Place new tests under the nearest matching subtree in `tests/unittests/` or
  `tests/componenttests/`.

## Completion Checklist

Before declaring work complete:
1. Run at least the relevant targeted checks for changed files.
2. Run broader checks when touching shared interfaces or cross-module behavior.
3. Ensure build/tests/lint are passing for the changed scope.
4. Note any unrun checks explicitly in your final response.
