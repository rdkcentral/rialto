# Design

## Context

See proposal.md for the motivation. The only existing runtime-loaded library is OCDM: `wrappers/source/OcdmSession.cpp` opens `libocdm.so` with `dlopen` and keeps the handle for the process lifetime. External libraries are reached through `wrappers/interface/I*.h` factories, so tests can substitute mocks. The SoC spec makes three things normative for Rialto: the interface types (§3, §5.1), the `extern "C" createCaptureSession` factory in `lib_capture_hal.so` (§8.1), and keeping the library loaded through session deletion and dependent cleanup (§4.6).

## Goals / Non-Goals

**Goals:**
- One copy of the HAL header that vendors build against and Rialto compiles with.
- A loader behind an interface, so server code and tests never call `dlopen` directly.
- A fake HAL that runs in unit and component tests with no SoC.

**Non-Goals:**
- Validating what the HAL returns (R3).
- Calling `getCapabilities`, `createPool`, acquire or release in a real flow (R7–R10).
- Runtime version negotiation; spec §8.1 says it is not required.

## Decisions

### D1. The HAL header is a Rialto-owned, installed header
Put `ICaptureSession.h` (all §3 and §5.1 types, in a `firebolt::rialto::capture` namespace or the namespace the spec headers use) in a new `media/server/capture/include/` directory and install it for vendor builds, with a pkg-config entry. Rialto compiles against the same file.

*Why:* the spec is the source of truth, and one installed copy stops vendor and Rialto copies drifting.
*Alternative:* each vendor supplies the header. Rejected: two copies of an ABI-critical header.

`CapturedFrame` declares all four copy/move operations deleted, as in the spec. Add `isEmpty()` and `resetReleased()` as free helper functions in a Rialto-private header, not members, so the vendor-facing header stays exactly as specified.

### D2. Loader as a wrapper: `ICaptureHalLibrary`
Add `wrappers/interface/ICaptureHalLibrary.h` with a factory accessor like the other wrappers. Its implementation:
- reads the library name from `RIALTO_CAPTURE_HAL_LIBRARY`, falling back to `lib_capture_hal.so` (environment variables reach the server through `rialto-config.json` `environmentVariables`, like other `RIALTO_*` settings);
- calls `dlopen(name, RTLD_NOW | RTLD_LOCAL)` once, on first use, and `dlsym("createCaptureSession")`;
- exposes `isAvailable()` and `createSession()`, which returns a `std::unique_ptr<ICaptureSession, Deleter>`.

The deleter deletes the session through the virtual destructor and then drops a `std::shared_ptr` to the library handle. The handle's own deleter calls `dlclose`. So the library unloads only after the loader and every session are gone.

*Why:* a shared handle owned by each session satisfies §4.6 without a global counter, and works when sessions outlive pipelines (R10).
**Build mode.** Unlike the other wrappers, whose implementations are compiled only outside `UnitTests` and `ComponentTests` builds, the loader source is listed with the always-built accessors in `wrappers/CMakeLists.txt`. It depends only on `libdl`, so it is safe in test builds, and the loader tests (task 3.2) need the real `dlopen` path against the fake HAL library. Higher layers still use `ICaptureHalLibrary` through its mock.

*Alternative:* never `dlclose`, as OCDM does. Viable and simpler, but it hides leaks in valgrind runs and keeps a vendor library mapped after capture is no longer used. Kept as the fallback if a vendor library is not safe to unload: a `RIALTO_CAPTURE_HAL_NO_UNLOAD` switch is cheap to add later without changing the spec.

### D3. Exception guard at the boundary
Calls into the session go through a thin `CaptureSessionGuard` in the server that wraps each call in `try/catch(...)`, logs, and returns `FATAL_ERROR`. This is the only place Rialto calls HAL methods.

*Why:* §8.1 forbids exceptions crossing the boundary, but Rialto must still not crash if a vendor violates it.

### D4. Fake HAL library
Build `tests/common/fakeCaptureHal/` as a shared library exporting `createCaptureSession`. Its session returns scripted results from a queue that the test fills through an exported `extern "C"` control function (or a small control header), records every call for assertions, and creates real FDs with `memfd_create` so FD tests see real descriptors. Later changes add scripts (pools, frames, invalidation) without changing the library's interface.

*Alternative:* gmock only. Rejected for loader tests, which must exercise `dlopen` and `dlsym` on a real `.so`. Unit tests of higher layers can still mock `ICaptureHalLibrary`.

## Risks / Trade-offs

- [Vendor library not safe to `dlclose`] → D2 fallback switch, and a test that loads and unloads the fake library twice.
- [ABI mismatch between vendor build and Rialto] → out of scope at run time (spec §8.1); the installed header and pkg-config version make mismatches visible at build time.
- [Header namespace or include path differs from vendor expectations] → agree the installed path with the SoC vendors before R2 lands; it is the only externally visible choice here.

## Migration Plan

None. Nothing calls the loader yet; it is inert until R5 uses it.
