# Design

## Context

See proposal.md for the motivation. Today:

- **Commit.** `CaptureCommit::run()` (R7) creates the HAL session and drives westerossink to configured READY through `IGstGenericPlayer::configureCaptureSink()`. It then calls `ICaptureAdmission::admit(CaptureSessionHandle &)`, and rolls back if that call fails. The default `ICaptureAdmissionFactory::instance()` creates `UnavailableCaptureAdmission`, which refuses every session.
- **Runtime.** On success, `MediaPipelineServerInternal` builds a `CaptureSessionRuntime` from the handle's `session`, `poolId` and `pool` (R9). `MediaPipelineService::allSourcesAttached()` gives the runtime to the capture registry.
  - `VideoFrameCaptureService::captureRuntime()` returns the active runtime, or null when the capture is uncommitted or retired.
  - The runtime serializes every HAL call under `m_mutex` and exposes `isFatal()`.
- **Validation.** R3 provides `validateCapabilities()` and `validatePool()`. `CaptureSessionGuard` wraps the HAL calls.
- **Server IPC.** `VideoFrameCaptureModuleService` answers acquire and release, but not `getPool`. The client already calls `getPool` (`CaptureCore::importNewFrame`) when an acquisition carries no pool metadata. `fillPoolMetadata()` builds a `GetVideoFrameCapturePoolResponse` from the runtime plus a frame.
- **READY timeout.** `GstGenericPlayer.cpp` parses `RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS` in a file-local helper.
- **Vulkan formats.** `VulkanFrameImporter.cpp` holds the fourcc→VkFormat mapping and `imageFormatSupported()`.
- **Name clash.** The proto package is `firebolt.rialto`. R4 already renamed the proto `GraphicsAPIType` to avoid clashing with the public header.

## Goals / Non-Goals

**Goals:**
- Replace the stand-in admission without changing `ICaptureAdmission`, `CaptureCommit`'s rollback, or R7's READY milestone.
- Expose capabilities and the pool layout through the existing runtime and registry lookups.

**Non-Goals:**
- PAUSED, the 0.94 sink property shapes, and the HAL enumerator rename (see proposal).
- Fetching the pool in the client before `allSourcesAttached()` returns.

## Decisions

### D1. One admission class, installed as the default factory
`CaptureAdmission` implements `ICaptureAdmission`. `ICaptureAdmissionFactory::instance()` creates it by default, and `UnavailableCaptureAdmission` is deleted. Its dependencies are injected for unit tests: a clock/sleep function for the retry loop, and an environment reader.

*Why:* R7's commit and the pipeline keep their code paths, and component tests can still replace the factory.

### D2. Inputs on the handle, not a new interface
`CaptureSessionHandle` gains:
- `VideoRequirements videoRequirements` (input), set by `CaptureCommit::run()` from a new parameter that `MediaPipelineServerInternal` passes (`m_kVideoRequirements`);
- `capture::CaptureCapabilities capabilities` and `std::uint32_t slotCount` (outputs).

`CaptureSessionRuntime`'s constructor takes the capabilities, and stores them for `getPool` and for the capabilities reply.

*Alternative:* a per-pipeline admission created with the requirements. Rejected, because the factory signature, `CaptureCommit` construction and the component-test factory would all change.

### D3. Retry loop
Retry runs on the main thread, which is already blocked for the commit:
- call `getCapabilities()` through `CaptureSessionGuard`;
- on `CAPABILITIES_NOT_AVAILABLE_YET`, sleep 10 ms and retry, until the READY timeout has elapsed since admission started;
- on any other non-`OK`, stop at once.

The budget is the full READY timeout again, not what the READY wait left over. That keeps the two waits independent and each bounded.

The parser moves from `GstGenericPlayer.cpp` to a shared server helper, `captureReadyTimeout()`, with unchanged behaviour. Both callers use it.

The timeout log states that the HAL never became ready at configured READY, and that a vendor needing PAUSED would produce this failure.

### D4. Slot count and pool ID
- **Slot count.** `chooseSlotCount(min, max, T)` is a pure function, so the roadmap table is a direct unit test.
- **T.** T is parsed from `RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS` at each admission.
- **Pool ID.** Pool IDs come from a process-wide `std::atomic<uint32_t>` starting at 1 that skips 0 on wrap-around. They are unique among live sessions, which is what `sessionForRelease()` needs.

### D5. Capabilities RPC through the runtime
`CaptureSessionRuntime::capabilities(CaptureCapabilities &out)` calls the HAL under `m_mutex`. It does not touch `m_fatal`, the slots or the flush state. The IPC handler works through these cases in order:
1. **Unknown capture, or API mismatch** → `INVALID_ARGUMENT`. The registry entry holds the API.
2. **No active runtime** → `SESSION_NOT_READY`.
3. **`runtime->isFatal()`** → `FATAL_ERROR`.
4. **Otherwise** → the live HAL call, mapped with the existing `toWireStatus()`. On `OK`, the handler fills:
   - `max_concurrent_frames` = `slotCount()`;
   - `max_resources_per_frame` = 1;
   - protected content and HDR = 0;
   - `drm_fourccs` = `[drmFormat]` and `drm_modifiers` = `[drmModifier]`.

*Alternative:* answer from the admission-time snapshot without calling the HAL. Rejected, because proposal 3.1 says the query invokes the HAL, and the runtime already serializes the call safely.

Retired and never-committed captures both return `SESSION_NOT_READY`, because `captureRuntime()` does not distinguish them. Its lookup stays unchanged.

### D6. getPool handler
The handler applies the same capture and runtime checks as D5. It then fills the reply from:
- the runtime's pool ID and pool;
- the stored capabilities (format, modifier, slot limits).

`fillPoolMetadata()` is split so the frame-independent part is shared, and the colour description stays frame-only.

### D7. Proto and status names
The proto changes are:
- `VideoFrameCaptureStatus` is renamed `VideoFrameCaptureWireStatus`, with enumerator names and numbers kept;
- `VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY = 7` is added;
- `GetVideoFrameCaptureCapabilitiesRequest { api = 1; capture_id = 2; }` and the interim response are added;
- `rpc getCapabilities` is added.

Existing references to the old type name are renamed mechanically: the server status mapping, the service, the client IPC and their tests.

The public header gains the interim declarations verbatim. The client maps wire to public status one-to-one.

### D8. Client query
`IVideoFrameCaptureIpc::getCapabilities(captureId, api, CaptureCapabilitiesReply &)` returns the wire status. An IPC failure becomes `FATAL_ERROR`.

`GLVideoFrameCapture::getCapabilities()` and `VulkanVideoFrameCapture::getCapabilities()` fill a local caps struct, and copy it to `out` only on `OK`. Both the return value and `status` are set.

The Vulkan capture maps each fourcc with the importer's mapping, which is exposed from `VulkanFrameImporter` as a free function, and keeps a format only if `imageFormatSupported()` passes for the reported modifier on the capture's physical device.

## Risks / Trade-offs

- [A vendor ready only at PAUSED (0.94 wording)] → admission times out at READY. The D3 log names the cause, and the failure is a clean rollback. Revisit when R7 moves to PAUSED.
- [Main thread blocked for the READY wait plus up to one more timeout] → bounded by configuration, and affects only capture pipelines.
- [Spec authors reject `capture_id = 2` or the enum rename] → both are local to this change's proto and IPC code. The public API is unaffected.
- [`maxConcurrentFrames` reported as N, not a ceiling] → this matches the A2 proposal. If the authors keep "ceiling", it is a one-line change in D5.

## Migration Plan

This change only adds to the wire. Merge R7, R9, R10 and R13 with or after this change. Their stand-in admission in component tests may stay for delivery-focused tests.
