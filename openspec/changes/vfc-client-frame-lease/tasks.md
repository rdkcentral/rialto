# Tasks

Unit tests run with `./build_ut.py -s client` (mock IPC and mock importer). "Spec" means `specs/client-frame-lease/spec.md`; "D#" means a decision in `design.md`.

## 1. Core and interfaces (D1)

- [x] 1.1 Add `IFrameImporter` (`importFrame`, `retire`, `drain`) and a mock; add `CaptureCore` owning the IPC, current frame, cache and release mutex; make `GLVideoFrameCapture` delegate `acquireCurrentFrame()` to it. Verify the client builds and existing tests pass.
- [x] 1.2 Add acquire, release and getPool to `VideoFrameCaptureIpc` (FDs read from `exported_dma_bufs`). Verify with `./build_ut.py -s clientipc`.

## 2. Acquisition and lease (D2, D5)

- [x] 2.1 Implement the status mapping and current-frame handling. Verify: new frame `changed = true`; repeat returns the same object with no new lock; `NO_FRAME` returns null; `NO_RESOURCES` and IPC failure keep the current frame; `FATAL_ERROR` clears it.
- [x] 2.2 Implement `VideoFrame` with the lease token and accessors (`frameIndex`, PTS, texture and visible size, `RGBA`, one resource). Verify accessor values for a 1920x1088 backing with a 1280x720 visible size.
- [x] 2.3 Implement release on last reference, serialized per capture, surviving capture destruction. Verify: one release per frame with the right slot and token; two threads dropping last references produce two non-overlapping releases; a frame outliving the capture still sends its release.

## 3. Cache and invalidation (D3, D4)

- [x] 3.1 Implement the cache key, hit/miss, LRU eviction of unused entries bounded by N, and FD closing on hits and borrowed imports. Verify: a hit closes N FDs; a colour-only change misses; the cache never exceeds N.
- [x] 3.2 Implement invalidation before lookup, retired entries passed to `retire()` after their last frame, and no restore after a failed import. Verify: a flushed frame with the same IDs re-imports; a retired entry is retired only after release; failed import leaves the invalidation applied.
- [x] 3.3 Implement failed-import handling: close FDs, release the slot, keep the previous frame. Verify with a failing mock importer.

## 4. Header documentation

- [x] 4.1 Document in `IVideoFrameCapture.h` that the visible region starts at the texture origin, that `layout()` is `RGBA` with one resource in V1.1, and that the frame reference is the lease. Verify the Doxygen build has no warnings for the file.

## 5. End-to-end (after R9)

- [ ] 5.1 Add client component tests against the server with the fake HAL: acquire, repeat, release, flush with reused IDs, late release after `stop()`. Verify with `./build_ct.py`.

## Workflow follow-up

- Run `openspec validate vfc-client-frame-lease --strict` before review.
- If the A8 or A21 proposals change, update the accessor and header-documentation tasks.
