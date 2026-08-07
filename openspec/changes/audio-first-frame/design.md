## Context

Rialto already supports first-frame notification for video and already has the downstream plumbing needed to propagate a first-frame event from gstplayer through the worker thread, server, IPC transport, and client callback. Audio playback currently lacks equivalent detection, which creates an observability gap even though the existing notification path is capable of carrying an audio first-frame event once one is detected.

This change touches multiple layers: gstplayer element setup, first-frame scheduling, server-side forwarding, IPC transport, and client delivery. The design therefore focuses on how audio first-frame detection is introduced without changing the public callback or protobuf contract and without regressing the existing video path.

## Goals / Non-Goals

**Goals:**
- Add first-audio-frame detection in gstplayer using capability-based discovery instead of platform branching.
- Reuse the existing first-frame task scheduling and propagation path by routing audio notifications with `MediaSourceType::AUDIO`.
- Support audio sink implementations that do not expose callback-based first-frame detection by using a bounded fallback probe.
- Preserve existing public client callbacks and existing `FirstFrameReceivedEvent` IPC transport.
- Keep video first-frame behavior unchanged.

**Non-Goals:**
- Redesign the current first-frame architecture.
- Introduce platform- or vendor-specific branches for audio first-frame support.
- Add a new public callback or a new protobuf event type for audio first-frame notification.
- Change behavior outside the first-frame detection and propagation path.

## Decisions

### Use role-specific capability detection during element setup

Audio first-frame support will be discovered when gstplayer configures pipeline elements. Audio decoder elements will be checked for `first-audio-frame`, while audio sink elements will be checked for `first-audio-frame-callback`.

This keeps detection aligned with the existing setup flow and avoids hard-coding platform names or element allowlists. It also lets decoder and sink behavior diverge cleanly where platform implementations expose different signaling mechanisms.

Alternatives considered:
- Use platform-specific branching: rejected because it scales poorly across vendors and hard-codes knowledge that should remain capability based.
- Use a probe for all audio elements: rejected because native callback/signal support is cheaper and more precise when available.

### Use sink-pad probing only as a fallback for audio sinks

If an audio sink does not expose `first-audio-frame-callback`, Rialto will install a sink pad probe and treat the first valid audio buffer as the first-frame trigger. The probe will be removed immediately after the first valid trigger and also removed during teardown, flush, reset, or pipeline stop.

This provides coverage for sink implementations that cannot emit a callback while keeping probe usage narrow and bounded. Restricting the fallback to audio sinks avoids adding speculative probe behavior to decoder elements where the expected mechanism is explicit capability detection.

Alternatives considered:
- No fallback path: rejected because platforms without sink callback capability would remain unsupported.
- Probe both decoders and sinks: rejected because it broadens runtime overhead and increases ambiguity in which stage constitutes first-frame detection.

### Reuse the existing first-frame scheduling and propagation path

Once audio first-frame is detected, the implementation will schedule the existing first-frame handling flow and identify the source using `MediaSourceType::AUDIO`. Server-side forwarding, IPC serialization, and client dispatch will continue to use the existing `notifyFirstFrameReceived(...)` and `FirstFrameReceivedEvent` path.

This minimizes surface area, keeps the client contract stable, and avoids introducing a parallel audio-specific notification stack.

Alternatives considered:
- Add a dedicated audio-first-frame event type: rejected because it duplicates the current pipeline without adding functional value.
- Dispatch directly from gstplayer to clients: rejected because it bypasses existing threading and session/source mapping behavior.

### Guard emission so exactly one first-frame event is sent per audio source lifecycle

Both callback-based and probe-based detection may observe the same source lifecycle. The implementation will keep a one-shot guard for the relevant audio source/session so only one first-frame event is scheduled and propagated. Cleanup paths will remove probe state and clear any temporary tracking during teardown and reset operations.

This preserves deterministic client behavior and prevents duplicate telemetry or callback delivery.

Alternatives considered:
- Allow duplicate low-level detections and deduplicate later: rejected because duplicates would still traverse more of the pipeline and complicate server/client behavior.

## Risks / Trade-offs

- Duplicate detection from callback and probe paths -> Mitigation: keep a one-shot guard at the first scheduling point and remove fallback probes immediately after the first valid trigger.
- Incorrect trigger on non-audio or non-buffer sink activity -> Mitigation: only treat the first valid audio buffer as a trigger and ignore unrelated pad activity.
- Regression in existing video behavior -> Mitigation: leave the video detection path unchanged and run existing first-frame non-regression coverage alongside new audio tests.
- Threading issues if detection performs too much work on the GStreamer thread -> Mitigation: keep callbacks and probes minimal and schedule work onto the existing worker-thread path.
- Platform variability in element support -> Mitigation: prefer capability-based detection and limit fallback logic to the specific sink case where callback support is absent.

## Migration Plan

1. Implement audio capability detection during gstplayer setup for decoders and sinks.
2. Add sink fallback probe handling, including install, one-shot trigger, and cleanup paths.
3. Route detected audio first-frame events through the existing scheduling and propagation flow using `MediaSourceType::AUDIO`.
4. Add or update unit tests for setup-element detection behavior, fallback probing, and single-emission guarantees.
5. Add or update component coverage for end-to-end server and client delivery of audio first-frame notifications.
6. Validate that existing video first-frame tests remain unchanged.

Rollback is low risk because the change is additive. If regressions are found, audio detection and fallback wiring can be removed while leaving the existing video path and notification contract intact.

## Open Questions

- Which exact gstplayer setup abstraction should own the audio sink probe lifecycle so cleanup is guaranteed across all teardown paths?
- Does any supported platform expose both decoder and sink audio first-frame signals simultaneously, and if so, where should the one-shot guard live to keep behavior deterministic?
- Are there existing test fixtures for sink pad probe behavior that can be extended, or does this change require new unit helpers for probe installation and cleanup validation?