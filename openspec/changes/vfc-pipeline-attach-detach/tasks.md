# Tasks

"Spec" means `specs/pipeline-capture-attachment/spec.md`; "D#" means a decision in `design.md`.

## 1. Server (D1, D3, D4)

- [x] 1.1 Add `attachVideoFrameCapture(captureId)` and `detachVideoFrameCapture()` to `MediaPipelineServerInternal` with the state rules, storing `m_attachedCaptureId`, and registry bind/unbind with "already bound elsewhere" rejection. Verify with `./build_ut.py -s servermain` tests for every state in the spec.
- [x] 1.2 Implement the R4 `MediaPipelineModuleService` handlers by calling the service layer, validating that `capture_id` belongs to the calling connection. Verify with `./build_ut.py -s serveripc`: success, wrong state, foreign ID, unknown session.
- [x] 1.3 Unbind on pipeline destruction and on capture destroy while bound. Verify with unit tests that both leave the capture attachable to a new pipeline.
- [x] 1.4 Make `setVideoWindow()` return `false` while a capture is attached. Verify with a unit test before attach, while attached and after detach.

## 2. Client (D2)

- [x] 2.1 Add the two RPCs to `MediaPipelineIpc`. Verify with `./build_ut.py -s clientipc`, including an old server that fails the call.
- [x] 2.2 Implement `MediaPipeline::attachVideoFrameCapture/detachVideoFrameCapture`: reject null and foreign captures locally, hold the `shared_ptr` only after server success, drop it on detach and destruction; track the attached marker in `GLVideoFrameCapture`. Verify with `./build_ut.py -s client`, including that the capture outlives the application's last reference while attached.

## 3. Component tests

- [ ] 3.1 Add a component-test state matrix that drives a real pipeline through UNKNOWN (created), loaded, committed, IDLE, PAUSED, PLAYING, SEEKING, END_OF_STREAM, STOPPED and FAILURE and checks the attach and detach result for each row of proposal §3.5. Verify with `./build_ct.py`.
- [ ] 3.2 Add component tests for one capture on two pipelines, two captures on one pipeline, reuse after detach and after pipeline destruction, and `setVideoWindow()` while attached. Verify with `./build_ct.py`.

## Workflow follow-up

- Run `openspec validate vfc-pipeline-attach-detach --strict` before review.
