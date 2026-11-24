## Why

Rialto has no way to reach the SoC capture HAL that every video frame capture feature depends on. The SoC spec (§3, §5.1, §8.1) defines the `ICaptureSession` interface, its value types and a vendor library with an `extern "C"` factory, but Rialto has neither the interface definition nor a loader. Later capture changes (record validation, admission, acquire/release, teardown) need both, plus a fake HAL to test against without vendor hardware.

## What Changes

- Add the capture HAL interface to Rialto exactly as spec §3 and §5.1 define it: `ICaptureSession` with `getCapabilities`, `createPool`, `acquireCurrentFrame` and `releaseFrame`; `CaptureStatus` including `CAPABILITIES_NOT_AVAILABLE_YET`; `CaptureCapabilities`, `CapturePool`, `CaptureSlotLayout`, `FramePlane`, `DmaBufObject`, `ExportedDmaBuf`, colour types and `CapturedFrame`.
- `CapturedFrame` is non-copyable and non-movable, its destructor closes nothing and releases nothing, and it has a defined empty state and a reset for a released, FD-free record.
- Add a HAL loader in the server. It opens `lib_capture_hal.so` by default (name overridable per platform), resolves the unmangled `createCaptureSession` factory, creates sessions, and keeps the library loaded until every session it created has been deleted.
- A missing library or factory symbol reports capture as unavailable instead of failing the server.
- Add a fake capture HAL shared library for tests, scriptable per call, which later capture changes extend.
- No public API, proto or pipeline behaviour changes in this change.

## Capabilities

### New Capabilities

- `capture-hal-binding`: How Rialto defines the capture HAL interface and its value types, discovers and loads the vendor library, creates and deletes sessions, and keeps the library loaded through dependent cleanup.

### Modified Capabilities

None.

## Impact

- **Code:** a new HAL interface header and a loader wrapper following the existing `wrappers/` pattern (dlopen-based, as `OcdmSession.cpp` does); server CMake to build and install the header for vendor builds.
- **Tests:** new unit tests under `tests/unittests/media/server/` and a fake HAL `.so` built only for tests.
- **Dependencies:** `libdl` (already used). No new third-party dependency.
- **Downstream:** R3 (record validation), R5 (registry: "HAL present" check), R8/R9/R10 use the loader and fake HAL.
