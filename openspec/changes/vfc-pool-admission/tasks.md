# Tasks

"Spec A" means `specs/capture-pool-admission/spec.md`; "Spec C" means `specs/video-frame-capture-capabilities/spec.md`; "D#" means a decision in `design.md`. Unit tests use the R2 fake HAL and existing mocks. Build with `-j 6`.

## 1. Wire and public surface (D7)

- [x] 1.1 In `proto/videoframecapturemodule.proto`:
  - rename the enum type to `VideoFrameCaptureWireStatus`, with a comment on the clash as R4 did for `GraphicsAPIType`;
  - add `SESSION_NOT_READY = 7`;
  - add `GetVideoFrameCaptureCapabilitiesRequest { api = 1; capture_id = 2; }`, the interim response and `rpc getCapabilities`.

  Rename the type's uses in the server status mapping, the service, the client IPC and their tests. Verify: the full build and `./build_ut.py -s serveripc`, `-s clientipc` pass unchanged.
- [x] 1.2 Add `VideoFrameCaptureStatus`, `VideoFrameCaptureCaps`, `GLVideoFrameCaptureCaps`, `VulkanVideoFrameCaptureCaps` and the two pure virtual `getCapabilities(out, status)` declarations to `media/public/include/IVideoFrameCapture.h`, as the interim header has them. Add stub overrides in `GLVideoFrameCapture`/`VulkanVideoFrameCapture` that return `FATAL_ERROR`, and update any mocks. Verify: the full build passes and `./build_ut.py -s client` passes.

## 2. Admission (D1–D4)

- [x] 2.1 Move the READY timeout parser into a shared server helper, `captureReadyTimeout()`, used by `GstGenericPlayer`. Verify: `./build_ut.py -s servergstplayer` passes unchanged, including the configured-timeout test.
- [x] 2.2 Add `videoRequirements`, `capabilities` and `slotCount` to `CaptureSessionHandle`. Pass `m_kVideoRequirements` through `CaptureCommit::run()`. Add the capabilities to the `CaptureSessionRuntime` constructor. Verify: `./build_ut.py -s servermain` and `-s servercapture` pass, and a commit test checks that the handle passed to admission carries the pipeline's requirements.
- [x] 2.3 Add the pure function `chooseSlotCount()` and the T parser. Verify with unit tests covering every row of the Spec A slot-count table, the invalid-target scenario with its warning, and the `[1,1]` failure.
- [x] 2.4 Implement `CaptureAdmission::admit()` with an injected clock and sleep: the capabilities query with the retry window, validation, the size check, slot selection, `createPool` once, pool validation, the pool ID counter, the handle outputs and the info log. Install it as the default factory and delete `UnavailableCaptureAdmission`. Verify with `./build_ut.py -s servermain` against every Spec A scenario:
  - ready on the third try;
  - never ready at a 200 ms budget, with the log text;
  - a HAL error with no retry;
  - inverted limits;
  - 3840x2160 against 1920x1088, and 1920x1080 against 1920x1088;
  - `createPool` failing with `NO_RESOURCES` and not called again;
  - the wrong slot count;
  - two admissions with distinct non-zero pool IDs.

## 3. Server queries (D5, D6)

- [x] 3.1 Add `CaptureSessionRuntime::capabilities()`, which calls the HAL under the runtime lock and leaves the session state unchanged. Verify with `./build_ut.py -s servermain`:
  - a `NO_RESOURCES` answer leaves the next acquire calling the HAL;
  - a query made while an acquire is blocked inside the fake HAL waits for it, and no overlap is detected.
- [x] 3.2 Add the `getCapabilities` handler to `VideoFrameCaptureModuleService`: unknown capture or wrong API, no runtime, fatal runtime, the live query and the reply values. Verify with `./build_ut.py -s serveripc`:
  - `INVALID_ARGUMENT` for an unknown capture and for the wrong API;
  - `SESSION_NOT_READY` with no runtime;
  - `FATAL_ERROR` for a fatal runtime;
  - `OK` with N, 1, 0, 0 and the single fourcc and modifier.
- [ ] 3.3 Add the `getPool` handler, sharing the frame-independent part of `fillPoolMetadata()`. Verify with `./build_ut.py -s serveripc`:
  - `OK` with three slot layouts, the pool ID, format, modifier, limits, no colour and no descriptors;
  - `SESSION_NOT_READY` before commit;
  - `INVALID_ARGUMENT` for an unknown capture;
  - the acquire reply's pool metadata is unchanged.

## 4. Client query (D8)

- [ ] 4.1 Add `IVideoFrameCaptureIpc::getCapabilities()` and its implementation, mapping wire status one-to-one and IPC failure to `FATAL_ERROR`. Verify with `./build_ut.py -s clientipc`: each wire status maps across, the request carries the capture ID and API, and a failed controller maps to `FATAL_ERROR`.
- [ ] 4.2 Implement `GLVideoFrameCapture::getCapabilities()`. Verify with `./build_ut.py -s client`:
  - the returned value and `status` agree;
  - `out` is unchanged on `SESSION_NOT_READY` and `FATAL_ERROR`;
  - `OK` fills the common fields and every `size` field.
- [ ] 4.3 Expose the fourcc→VkFormat mapping from `VulkanFrameImporter` and implement `VulkanVideoFrameCapture::getCapabilities()` with the device check. Verify with `./build_ut.py -s client` and the Vulkan API mock:
  - NV12 linear and supported gives `[VK_FORMAT_G8_B8R8_2PLANE_420_UNORM]`;
  - an unsupported device or modifier gives an empty list with `OK`;
  - an unknown fourcc is omitted.

## 5. Integration

- [ ] 5.1 Add server component tests with the fake HAL and the real admission. Verify with `./build_ct.py`:
  - commit succeeds, and the first acquire reports the admitted pool ID;
  - `RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS=2` gives `createPool(2)`;
  - a `[1,1]` HAL fails `allSourcesAttached()`, rolls back, and a retry with `[1,4]` succeeds;
  - "not ready" pushed twice still commits;
  - `getCapabilities` returns `SESSION_NOT_READY` before commit and after stop, and `OK` with N after commit.

  Existing delivery tests keep their stand-in admission and pass unchanged.
- [ ] 5.2 Add a client component test with the IPC server stub (as in R11–R13): `getCapabilities` before and after commit returns `SESSION_NOT_READY` and then `OK`, with `maxConcurrentFrames` equal to the stubbed N. Verify with `./build_ct.py`.

## Workflow follow-up

- Run `openspec validate vfc-pool-admission --strict` before review.
- Raise with the spec authors, alongside A2: `capture_id = 2` in the capabilities request, the proto enum rename (interim `VideoFrameCaptureStatus` clashes with the public enum), and `maxConcurrentFrames` = admitted N.
- Merge R7, R9, R10 and R13 with or after this change.
