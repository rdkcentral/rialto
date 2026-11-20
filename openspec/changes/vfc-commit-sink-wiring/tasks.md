# Tasks

"Spec" means `specs/capture-commit-configuration/spec.md`; "D#" means a decision in `design.md`. Unit tests use mocked gst/glib wrappers and the R2 fake HAL.

## 1. Hand-over interface

- [ ] 1.1 Add `ICaptureAdmission { bool admit(CaptureSessionHandle &) }` with a mock; R8 implements it. Verify the server builds with the mock injected.

## 2. Sink setup in the player (D2–D5)

- [ ] 2.1 Refactor `setWesterossinkSecondaryVideo()` into `createWesterossink(bool secondaryVideo)` used by both paths. Verify existing `./build_ut.py -s servergstplayer` tests pass unchanged.
- [ ] 2.2 Add `GstGenericPlayer::configureCaptureSink(session, caps, resolution, timeout)` on the worker thread: create the sink, check the three properties exist, set `rdk-capture-session`, `codec` and `resolution`, assign `video-sink`, drive to READY and wait. Verify call order with mocks: properties before any state change, READY last.
- [ ] 2.3 Add rollback: sink to NULL, clear `video-sink`, unref, return failure. Verify with mocks that a failed READY, a timeout and a missing property each leave no sink reference and no `video-sink`.
- [ ] 2.4 Build `codec` with `MediaSourceVideoCapsBuilder` and `resolution` from `VideoRequirements`; read the timeout from `RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS` (default 5000). Verify: H.264 caps and `"1920x1080"`; width 0 source still sets resolution; HEVC Main10 caps unchanged; a configured 2 s timeout is used.

## 3. Commit orchestration (D1, D4)

- [ ] 3.1 Add `CaptureCommit::run()`: clear-source and video-present checks, sink override check, session creation, `configureCaptureSink`, `admit`, and full rollback with session deletion after NULL. Call it from `allSourcesAttachedInternal()`, setting `m_wasAllSourcesAttachedCalled` only on success. Verify with `./build_ut.py -s servermain`: encrypted source, no video source, override, sink failure, admission failure each return `false` with no leaks; success returns `true` and schedules shipping setup after.
- [ ] 3.2 Verify retry: after a failure, a second call creates a new session and a new sink; after detach, the second call commits without capture.
- [ ] 3.3 Verify no change without a capture: existing `allSourcesAttached` unit and component tests pass unchanged.

## 4. Integration

- [ ] 4.1 Add server component tests with the fake HAL and mocked admission: success, each failure, secondary-video pipeline with one sink carrying both settings. Verify with `./build_ct.py`.
- [ ] 4.2 On target, with the proprietary playbin and the vendor westerossink: commit, then play to PAUSED, and confirm the assigned sink instance is still playbin's `video-sink` and never passed through NULL (sink state-change log). Record the result in the PR.

## Workflow follow-up

- Run `openspec validate vfc-commit-sink-wiring --strict` before review.
- Merge together with or after R8 so the admission hook has a real implementation.
