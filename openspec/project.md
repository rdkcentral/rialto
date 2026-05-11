# Rialto Project Constitution

## Tech Stack
- **Language:** C++ (standard compliant with RDK guidelines).
- **Build System:** CMake.
- **Testing:** GTest (Google Test) for Unit Tests.
- **Documentation:** Doxygen.
- **Formatting:** Clang-format (see `.clang-format` in root).

## Coding Standards
- **Naming:** Follow Rialto/RDK naming conventions (typically `camelCase` for methods, `PascalCase` for classes).
- **Architecture:** Maintain the separation between `ipc`, `serverManager`, and `media` pipeline logic.
- **Style:** Strictly follow the rules defined in `CPPLINT.cfg`.

## Project Rules
- All new features MUST include corresponding unit tests in the `tests/` directory.
- Use the provided logging framework in `logging/` for all trace and error output.
- Every commit or change must be compatible with the Yocto build environment.
- Reference the official [Rialto Coding Guidelines](https://rdkcentral.com) for specific implementation patterns.

