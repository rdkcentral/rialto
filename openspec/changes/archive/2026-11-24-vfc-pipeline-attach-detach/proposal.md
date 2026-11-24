## Why

A registered capture (R5) must be bound to a media pipeline so the commit step (R7) knows to route that pipeline's video to capture instead of the video plane. Proposal §3.2 defines `attachVideoFrameCapture()` and `detachVideoFrameCapture()` with strict state preconditions, single-pipeline binding and a `setVideoWindow()` rule, and §3.5 gives the attach/detach column for every shipping state. R4 added the methods, but they always return `false`.

## What Changes

- `attachVideoFrameCapture(capture)` succeeds only in UNKNOWN (loaded): after `load()` and before `allSourcesAttached()`. It returns `false` for a null capture, a capture not created by Rialto's factory, a capture already attached to any pipeline, a pipeline that already has a capture, any other state, or a server without the RPC.
- The pipeline keeps a reference to the attached capture until detach or pipeline destruction. Video and capture may be attached in either order before `allSourcesAttached()`.
- `detachVideoFrameCapture()` succeeds in UNKNOWN (loaded) and STOPPED, and returns `false` in IDLE, PAUSED, PLAYING, SEEKING, SEEK_DONE, END_OF_STREAM and FAILURE, and when nothing is attached.
- Destroying the pipeline detaches implicitly. A detached capture can be attached to a new pipeline.
- `setVideoWindow()` returns `false` while a capture is attached (Q2).
- The server records the binding in the capture registry from R5; destroying a capture that is still bound is treated as an implicit detach.
- Attaching allocates nothing; buffers are created at commit (R7, R8).

## Capabilities

### New Capabilities

- `pipeline-capture-attachment`: When and how a Video Frame Capture is bound to and unbound from a media pipeline, the state rules for each, and the effect of a binding on `setVideoWindow()`.

### Modified Capabilities

None.

## Impact

- **Client:** `media/client/main` `MediaPipeline` (keeps the capture reference and forwards the calls), `MediaPipelineIpc` (new RPCs), `GLVideoFrameCapture` (tracks whether it is attached).
- **Server:** `MediaPipelineModuleService` handlers from R4 get real behaviour; `MediaPipelineServerInternal` stores the attached `capture_id` and checks state; the R5 registry stores the bound session.
- **Tests:** client and server unit tests; a component-test state matrix.
- **Dependencies:** R5.
- **Downstream:** R7 reads the binding at commit; R10 uses detach and destruction as teardown triggers; R14 calls attach from rialto-gstreamer.
