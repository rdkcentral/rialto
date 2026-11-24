## Why

The application sees frames only through `acquireCurrentFrame()` and shared frame objects. Proposal §3.4 makes the shared frame reference the lock lease: the last reference releases the HAL slot exactly once. §3.6 and SoC spec §6.1–§6.2 define a client import cache keyed by allocation identity and description, invalidation before lookup, FD closing on cache hits, and deferred destruction of imports that older frames still use. This logic is the same for GL and Vulkan, so it belongs in one graphics-independent core that the importers (R12, R13) plug into.

## What Changes

- `acquireCurrentFrame()` follows proposal §3.4: a new selection returns `{changed = true, frame}` and becomes the current frame; `NO_NEW_FRAME` returns `{false, current frame}` with no new lock; `NO_FRAME` clears the current frame and returns `{false, nullptr}`. `NO_RESOURCES` or an IPC failure returns `{false, current frame}`; `FATAL_ERROR` or `UNSUPPORTED` clears it and returns `{false, nullptr}`.
- Repeated unchanged acquisitions share one frame object and one lock.
- Dropping the last reference to a frame, on any thread, sends exactly one `releaseFrame` with its slot and token; releases are serialized per capture. Frames stay valid after the pipeline or capture is gone, and late releases still succeed.
- Import cache keyed by session and invalidation period, ordered object IDs and planes, the full import description (format, modifier, size, colour) and the graphics context; at most N reusable entries, evicting unused ones.
- On a cache hit the received FDs are closed. On `dma_buf_objects_flushed` the old entries are invalidated before lookup; entries still used by held frames are retired and destroyed only after those frames are released; retired entries never return to the cache.
- A failed import closes the received FDs, releases the slot, and does not undo the invalidation.
- Frame accessors: `frameIndex()` is the slot, `textureWidth/Height` the backing size, `visibleWidth/Height` the visible size, `layout()` is `RGBA` and `numResources()` is 1 (A8 proposal part 1). The header documents that the visible region starts at the texture origin (A21 proposal).
- The graphics import itself sits behind an importer interface; R12 provides GL.

## Capabilities

### New Capabilities

- `client-frame-lease`: How the client acquires frames, represents them to the application, leases and releases their slots, and caches graphics imports across acquisitions, independently of the graphics API.

### Modified Capabilities

None.

## Impact

- **Client:** a capture core in `media/client/main` used by `GLVideoFrameCapture` (R5); `VideoFrameCaptureIpc` gains acquire, release and pool calls; an `IFrameImporter` interface and a mock; header documentation in `IVideoFrameCapture.h`.
- **Tests:** unit tests with a mock IPC and a mock importer; end-to-end component tests after R9.
- **Dependencies:** R4 for unit tests; R9 for end-to-end tests; R8 for the admitted count N (the core reads it from the pool description).
- **Downstream:** R12 and R13 implement `IFrameImporter`.
