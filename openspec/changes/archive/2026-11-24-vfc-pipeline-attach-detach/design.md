# Design

## Context

See proposal.md for the motivation. R4 added the public methods (defaults return `false`), the attach/detach RPCs (`session_id`, `capture_id`) and failing server handlers. R5 added the per-connection registry and `GLVideoFrameCapture` on the client. The shipping server tracks load and commit in `MediaPipelineServerInternal` (`m_wasAllSourcesAttachedCalled`), and playback state for each session.

## Goals / Non-Goals

**Goals:**
- State checks in one place, on the server, so client and server agree.
- Client-side rejection of null and foreign captures without an IPC round trip.

**Non-Goals:**
- Any sink, HAL session or pool work (R7, R8).
- Runtime capture/video-plane switching (Q1: not in V1.1).

## Decisions

### D1. The server owns the state rules
`MediaPipelineServerInternal::attachVideoFrameCapture(captureId)` accepts only when loaded and not committed; `detachVideoFrameCapture()` accepts when loaded and not committed, or in STOPPED. It also asks the registry to bind or unbind (`CaptureEntry::attachedSessionId`), which rejects a capture already bound elsewhere. Both run on the main thread, like other pipeline operations.

*Why:* only the server knows the exact playback state, and §3.5 is a server-state table.
*Alternative:* client tracks state from notifications. Rejected: races with in-flight state changes.

### D2. The client checks null, foreign and local binding first
`MediaPipeline::attachVideoFrameCapture` returns `false` for null or for a capture that does not `dynamic_cast` to the client's capture implementation (foreign). It holds the `shared_ptr` only after the server accepts, and releases it on successful detach or pipeline destruction. `GLVideoFrameCapture` keeps a weak "attached to" marker so a second attach is rejected locally, and the server check still covers races.

### D3. Implicit detach on destruction
Server pipeline destruction calls the registry's unbind for its capture. The registry's destroy path (R5) unbinds the capture from its pipeline if still bound. Both are idempotent.

### D4. `setVideoWindow()`
`MediaPipelineServerInternal::setVideoWindow` returns `false` while `m_attachedCaptureId` is set. The client forwards as today.

## Risks / Trade-offs

- [rialto-gstreamer sets `setVideoWindow` routinely] → it will now fail while capturing; R14 must skip the call when the capture property is set, and that is listed in R14.
- [Attach in UNKNOWN (loaded) racing a state change] → the server serializes on its main thread, so the check and the bind are atomic.

## Migration Plan

None. Pipelines without a capture are unchanged.
