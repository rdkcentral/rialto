## Why

Rialto already reports first-frame receipt for video, but it does not provide equivalent signaling for audio. Adding audio first-frame support closes that observability gap now that the existing first-frame pipeline already covers the worker-thread, server, IPC, and client callback path needed to carry the event end to end.

## What Changes

- Extend gstplayer setup to detect first-audio-frame support using element capabilities instead of platform-specific branching.
- Use `first-audio-frame` for audio decoder elements and `first-audio-frame-callback` for audio sink elements when those capabilities are available.
- Add a sink-only fallback probe when an audio sink does not expose callback support, and remove it immediately after the first valid audio buffer is observed.
- Reuse the existing first-frame scheduling, server forwarding, IPC event, and client callback path for audio by routing the event with `MediaSourceType::AUDIO`.
- Preserve the existing public callback and protobuf contract so the change remains additive and non-breaking.

## Capabilities

### New Capabilities
- `audio-first-frame`: Detect and propagate the first rendered audio frame through the existing first-frame notification pipeline.

### Modified Capabilities

None.

## Impact

Affected areas include gstplayer element setup and first-frame detection, worker-thread task scheduling for first-frame handling, server-side forwarding via `notifyFirstFrameReceived(...)`, IPC transport using `FirstFrameReceivedEvent`, and client-side forwarding to `notifyFirstFrameReceived(sourceId)`.

This change does not introduce a new public callback or protobuf message, but it does require unit and component coverage for audio capability detection, sink probe fallback, one-shot event emission, and video non-regression.
