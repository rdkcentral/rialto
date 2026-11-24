## Why

The capture commit (R7) drives westerossink to configured READY and then hands the HAL session to an admission step that does not exist yet: `UnavailableCaptureAdmission` refuses every session, so no capture can be committed and R7, R9, R10 and R13 cannot merge. SoC spec v0.94 and the V1.1 proposal now define the capabilities status and the public status-returning query (spec-author question A2), which unblocks this change.

This change completes R8 against the existing implementation: it keeps R7's READY milestone, its `codec`/`resolution` sink properties and its `ICaptureAdmission` hand-over unchanged.

## What Changes

- **Pool admission** (proposal 3.3 steps 8–9, SoC spec 3.1–3.2, 4.4): a real `ICaptureAdmission`, installed as the default factory, that runs on the main thread after configured READY:
  1. calls the HAL `getCapabilities()`; while it answers "not ready yet", retries within the existing `RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS` budget (default 5000 ms, SoC spec 3.1 retry guidance), then fails;
  2. validates the record with the R3 validator;
  3. checks `VideoRequirements::maxWidth/maxHeight` against `maximumContentSize`;
  4. chooses N = clamp(T, max(2, minimumSlots), maximumSlots), with T from `RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS` (default 3; roadmap decision A3+A22, option C); `maximumSlots < 2` fails;
  5. calls `createPool(N)` once, validates the pool, assigns a non-zero pool ID and hands the session, pool and capabilities to the commit.
  Any failure returns `false`; R7's existing rollback then returns the sink to NULL and deletes the session.
- **Capabilities query, server** (proposal 3.1, 3.4): the `getCapabilities` RPC answers from the capture's committed session by calling the HAL through the session runtime. It reports the admitted N as `max_concurrent_frames`, 1 resource per frame, no protected content or HDR, and the HAL format and modifier.
- **Pool layout, server** (proposal 3.4): the `getPool` RPC, which the client already calls, now returns the admitted structural layout.
- **Public API** (proposal 3.1, interim header): `VideoFrameCaptureStatus` (with `SESSION_NOT_READY`), `VideoFrameCaptureCaps`, `GLVideoFrameCaptureCaps`, `VulkanVideoFrameCaptureCaps`, and `getCapabilities(out, status)` on the GL and Vulkan captures, exactly as the interim header declares them (status returned and set).
- **Wire** (interim proto): `GetVideoFrameCaptureCapabilitiesRequest`/`Response`, `rpc getCapabilities`, and `VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY = 7`. Two local decisions to confirm with the spec authors:
  - the request gains `capture_id = 2`, because `api` alone cannot reach the pipeline's HAL session;
  - the proto enum type is renamed `VideoFrameCaptureWireStatus`. The proto package is `firebolt.rialto`, so the interim name clashes with the new public `firebolt::rialto::VideoFrameCaptureStatus`, the same clash R4 resolved for `GraphicsAPIType`. Value names and numbers are unchanged, so the wire is unaffected.
- **Not in this change:** driving the sink to PAUSED and the 0.94 `codec`/`resolution-width`/`resolution-height` property shapes (R7 keeps READY and its N4 properties); renaming the HAL `CAPABILITIES_NOT_AVAILABLE_YET` enumerator (same value as 0.94's `SESSION_NOT_READY`, so the vendor ABI already matches); fetching the pool before `allSourcesAttached()` returns (the client keeps fetching it on first import, and the first acquisition already carries it); A8, A21.

## Capabilities

### New Capabilities

- `capture-pool-admission`: How a configured capture session is admitted: the capabilities query and its retry window, validation, the size check, the slot-count rule, pool creation, pool identity and the failure result.
- `video-frame-capture-capabilities`: The public `getCapabilities()` query and its wire form, which status it reports in each capture state, which values it reports, and the `getPool` reply.

### Modified Capabilities

None. The R7 commit's requirements are unchanged: it already fails and rolls back when admission fails.

## Impact

- **Server main:** new `CaptureAdmission` in place of `UnavailableCaptureAdmission`. `CaptureSessionHandle` gains the pipeline's `VideoRequirements` and the admitted `CaptureCapabilities`. `CaptureCommit::run()` passes the requirements through. The READY timeout parser is shared with `GstGenericPlayer` and its behaviour does not change.
- **Server runtime:** `CaptureSessionRuntime` keeps the admitted capabilities and gains a capabilities query, serialized with its other HAL calls.
- **Server IPC:** `getCapabilities` and `getPool` handlers in `VideoFrameCaptureModuleService`, plus the renamed proto enum type in the existing status mapping.
- **Client:** `getCapabilities()` on `GLVideoFrameCapture` and `VulkanVideoFrameCapture`, and `IVideoFrameCaptureIpc::getCapabilities()`. The Vulkan formats are mapped from the DRM fourccs with the existing `VulkanFrameImporter` mapping and checked against the device.
- **Tests:** unit tests with the fake HAL and mocks. The component tests replace the stand-in `AcceptingAdmission` with the real admission where the test is about admission. Existing delivery tests keep working.
- **Dependencies:** R2, R3, R7, and R9 (session runtime). R7, R9, R10 and R13 merge with or after this change. Their stubbed admission needs no rebase, because the `ICaptureAdmission` interface is unchanged.
- **Risk:** a vendor that follows 0.94 literally and becomes ready only at PAUSED keeps answering "not ready" at READY. Admission then times out and `allSourcesAttached()` returns `false`, with a log line that names the case.
