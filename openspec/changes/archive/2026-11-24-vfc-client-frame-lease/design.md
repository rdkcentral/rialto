# Design

## Context

See proposal.md for the motivation. R5 added `GLVideoFrameCapture` and `VideoFrameCaptureIpc`; R9 defines the acquire reply (slot, token, metadata, FDs, flush flag, optional pool metadata) and release. The admitted count N arrives with the pool description (R8's `getPool`, or `pool_metadata` on acquisition). The SoC spec §6.2 cache rules and A9 (no GL calls on release) constrain the design; GL and Vulkan specifics stay out.

## Goals / Non-Goals

**Goals:**
- All lease, cache and invalidation logic in one graphics-independent core, unit-testable with mocks.
- Release path safe on any thread and free of graphics calls.

**Non-Goals:**
- EGL/GL and Vulkan calls (R12, R13). How queued graphics deletions run (R12's A9 design).

## Decisions

### D1. `CaptureCore` plus `IFrameImporter`
`CaptureCore` owns the IPC, the current frame, the cache and a release queue. It calls `IFrameImporter`:
- `importFrame(description, fds) -> ImportHandle` (consumes or borrows FDs; reports which);
- `retire(ImportHandle)` (queue for destruction; must not need a graphics context);
- `drain()` (called at the start of each acquisition, where the context is current).

`GLVideoFrameCapture` (and later the Vulkan capture) composes a core with its importer.

### D2. Frame object and lease
`VideoFrame` (implementing `IGLVideoFrame`) holds a `shared_ptr<CacheEntry>` and a `LeaseToken { captureCore weak_ptr, pool_id, slot, token }`. Its destructor calls `core->release(token)`, which sends `releaseFrame` under a per-capture release mutex. If the core is gone (capture destroyed), the token keeps a `shared_ptr` to the IPC channel so the late release is still sent.

*Why:* the destructor of the last `shared_ptr` is the only reliable "last reference" signal, and it runs on the dropping thread.

### D3. Cache key and entries
Key: `{invalidationPeriod, ordered (objectId, plane offset/length/stride), drmFormat, drmModifier, backingSize, colour, graphicsContext}`. Entries carry a use count from held frames. Eviction picks unused entries, least recently used first. The `invalidationPeriod` counter increments on every flushed frame, so old entries can never match again; entries with a use count move to a retired list and are passed to `retire()` when their last frame is released.

### D4. FD handling
On a hit, the core closes the received FDs. On a miss, the importer reports whether it consumed them (Vulkan) or borrowed them (EGL); the core closes borrowed FDs after the import returns. On import failure the core closes any still-owned FDs and releases the slot.

### D5. Status mapping
`OK` → import and install; `NO_NEW_FRAME`, `NO_RESOURCES`, IPC failure → current frame, `changed = false`; `NO_FRAME`, `FATAL_ERROR`, `UNSUPPORTED` → clear current frame, null.

## Risks / Trade-offs

- [Release sent while the client is shutting down] → failures are logged at debug; server disconnect cleanup (R10) releases anyway.
- [Admitted count unknown before the first pool description] → the cache bound applies once the description arrives; before that, no frames exist.

## Migration Plan

None. Only applies to captures attached to committed pipelines.
