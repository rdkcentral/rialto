# Tasks

Unit tests run with `./build_ut.py -s <suite>`. "Spec" means `specs/capture-hal-binding/spec.md`; "D#" means a decision in `design.md`.

## 1. HAL header (D1)

- [x] 1.1 Add `media/server/capture/include/ICaptureSession.h` with every type from SoC spec §3 and §5.1, including `CAPABILITIES_NOT_AVAILABLE_YET` and the deleted copy/move operations on `CapturedFrame`. Install it and add a pkg-config file for vendor builds. Verify the server builds and the header installs to the staging directory.
- [x] 1.2 Add Rialto-private helpers `isEmpty(const CapturedFrame &)` and `resetReleased(CapturedFrame &)`. Verify with unit tests: a default record is empty; a record with one FD not `-1` is not empty; a released, FD-free, flushed record resets to empty with the flag and metadata cleared.
- [x] 1.3 Add a compile-time test file with `static_assert`s that `CapturedFrame` is not copy- or move-constructible or assignable. Verify it compiles in the unit-test build.

## 2. Loader wrapper (D2, D3)

- [x] 2.1 Add `wrappers/interface/ICaptureHalLibrary.h`, its factory accessor and a mock in the wrappers mocks. Verify the wrappers build and the mock compiles.
- [x] 2.2 Implement the loader, listed with the always-built sources in `wrappers/CMakeLists.txt` (design D2, build mode): library name from `RIALTO_CAPTURE_HAL_LIBRARY` or `lib_capture_hal.so`, `dlopen` on first use, `dlsym("createCaptureSession")`, `isAvailable()`, and `createSession()` returning a session whose deleter deletes it and then drops the shared library handle. Verify with the tests in 3.2.
- [x] 2.3 Add `CaptureSessionGuard`, which wraps each HAL call in `try/catch(...)`, logs, and returns `FATAL_ERROR`. Verify with a unit test using a gmock session that throws.

## 3. Fake HAL and loader tests (D4)

- [x] 3.1 Add the fake HAL shared library under `tests/common/fakeCaptureHal/`: exports `createCaptureSession`, takes scripted results through a control API, records calls, and creates real FDs with `memfd_create`. Verify it builds and `nm -D` shows the unmangled `createCaptureSession`.
- [ ] 3.2 Add loader unit tests: the fake library loads and creates a session; a missing library and a missing symbol report unavailable and log once; an overridden name is used; a null factory result fails without crashing; a session outliving the loader keeps the library loaded until it is deleted (check with `dl_iterate_phdr` or `/proc/self/maps`). Verify with `./build_ut.py -s servermain` (or the suite that hosts the new tests).

## 4. Integration check

- [ ] 4.1 Run `./build_ut.py` for the touched suites with `-val` and confirm valgrind reports no leaks from loading, creating, deleting and unloading the fake HAL twice in one process.

## Workflow follow-up

- Run `openspec validate vfc-capture-hal-binding --strict` before review.
- Agree the installed header path and namespace with the SoC vendors before merge.
