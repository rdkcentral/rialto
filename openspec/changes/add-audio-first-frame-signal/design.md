## Context

Rialto is a C++ media pipeline middleware that sits between an application and a GStreamer-based playback backend. Events originating in the GStreamer layer are propagated up through a layered architecture:

```
GStreamer signal/probe
       │
       ▼
IGstGenericPlayerPrivate::schedule*()   [GstGenericPlayer – worker thread schedule]
       │
       ▼
Generic task (enqueued to worker task queue)
       │
       ▼
IGstGenericPlayerClient::notify*()      [MediaPipelineServerInternal]
       │
       ▼
IMediaPipelineClient::notify*()         [server/ipc MediaPipelineClient]
       │ (protobuf event via IPC)
       ▼
Client IPC (MediaPipelineIpc::on*())
       │
       ▼
IMediaPipelineIpcClient::notify*()      [MediaPipeline – public layer]
       │
       ▼
IMediaPipelineClient::notify*()         [application callback]
```

This pattern is established and used consistently for underflow, playback errors, source flushed, and other events. The first-audio-frame notification follows the same path end-to-end.

Rialto does not currently expose a first-audio-frame notification. The gap means applications cannot reliably determine when audio output is available.

Detection occurs in `SetupElement` – the task responsible for connecting element-level signals. The underflow detection already uses a capability-based signal search via `getUnderflowSignalName` / `Utils.cpp`. First-audio-frame detection must use the same pattern.

## Goals / Non-Goals

**Goals:**
- Detect the first audio frame at the GStreamer layer using a platform-agnostic, capability-based strategy.
- Schedule first-frame handling on the Rialto worker task queue, never inline on a GStreamer thread.
- Propagate the notification through all existing Rialto layers to the public client callback.
- Ensure the notification fires at most once per source per session.
- Keep the fallback pad probe installed only when no supported first-frame signal is available, and remove it immediately after the first detection.

**Non-Goals:**
- Video first-frame notification (out of scope for this change).
- Platform-specific detection branches (no vendor/SoC identity checks).
- Changes to playback state machine semantics.
- Render-time measurement or QoS policy.

## Decisions

### Decision 1: Capability-based signal detection, same pattern as underflow

**Choice:** Introduce a `getFirstAudioFrameSignalName()` utility (analogous to `getUnderflowSignalName`) that iterates over the GObject signal list of an audio element and returns the first match from the supported set `{ "first-audio-frame", "first-audio-frame-callback" }`.

**Rationale:** The underflow detection utility in `Utils.cpp` already proves this pattern. It avoids any SoC/vendor identity check and works for any backend that exposes one of these signals on an audio decoder or sink. Reusing the same structure keeps the implementation consistent and extensible (new signal names can be added to the candidate set later).

**Alternatives considered:**
- Hard-code element name prefixes (e.g., `brcmaudiosink`): rejected – this violates platform-agnosticism and requires maintenance whenever a new SoC is added.
- Inspect GObject properties instead of signals: rejected – the backend exposes first-frame as a signal, not a property; also inconsistent with underflow.

### Decision 2: Pad probe fallback on audio sink pad

**Choice:** If no supported first-frame signal is found on the audio element in `SetupElement`, install a `GST_PAD_PROBE_TYPE_BUFFER` static probe on the audio sink's sink pad. The probe callback schedules the first-frame task and immediately removes itself (returns `GST_PAD_PROBE_REMOVE`).

**Rationale:** A buffer probe on the sink pad is the lowest-cost generic mechanism. It fires on the first real audio buffer, is self-removing, and does not interfere with the pipeline. The probe is only installed as a fallback so there is no runtime overhead on backends that expose a native signal.

**Alternatives considered:**
- Always use a pad probe (drop signal support): rejected – wastes a probe slot on backends that already expose a reliable signal.
- Use a GStreamer bus message: rejected – the pipeline bus is not reliable for first-frame ordering relative to data flow.

### Decision 3: Scheduling via `IGstGenericPlayerPrivate::scheduleAudioFirstFrame()`

**Choice:** Add `scheduleAudioFirstFrame()` to `IGstGenericPlayerPrivate` (mirroring `scheduleAudioUnderflow()`). The GStreamer callback/probe calls this method; `GstGenericPlayer` enqueues a new `FirstFrameReceived` task on the worker task queue.

**Rationale:** All event scheduling from GStreamer callbacks into Rialto uses this indirection. It decouples the GStreamer thread from Rialto business logic and keeps the task queue as the single scheduling point.

**Alternatives considered:**
- Call the client notification inline from the GStreamer callback: rejected – violates Rialto threading model and risks re-entrancy issues.

### Decision 4: One-time guard in `StreamInfo` (or task)

**Choice:** The `FirstFrameReceived` task checks a `firstAudioFrameReceived` boolean flag (stored in `StreamInfo` for the audio source). If already set, the task is a no-op. The flag is set to `true` on first execution.

**Rationale:** Both signal and probe paths may fire more than once in edge cases (e.g., seek resets the pipeline). Deduplication in the task is the natural place because the task has access to `GenericPlayerContext`.

**Alternatives considered:**
- Deduplicate in the signal/probe callback: possible but the GStreamer thread should do minimal work; task is the better place.

### Decision 5: New protobuf message `FirstFrameReceivedEvent`

**Choice:** Add to `mediapipelinemodule.proto`:
```proto
message FirstFrameReceivedEvent {
    optional int32 session_id = 1 [default = -1];
    optional int32 source_id  = 2 [default = -1];
}
```

**Rationale:** All source-specific server-to-client events carry `session_id` + `source_id` (see `BufferUnderflowEvent`, `SourceFlushedEvent`). Using the same shape keeps client-side deserialization uniform.

### Decision 6: Source ID mapping

**Choice:** The audio source ID is obtained from `GenericPlayerContext::streamInfo` (keyed by `MediaSourceType::AUDIO`). The `StreamInfo` struct holds the attached source ID; the task retrieves it and passes it through the call chain.

**Rationale:** Existing events (underflow, flush) follow the same approach. This avoids introducing a separate lookup mechanism.

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| Signal fires more than once (e.g., after seek) | `firstAudioFrameReceived` guard in `FirstFrameReceived` task; consider resetting on Stop/Seek. |
| Pad probe triggers on non-data buffers (gaps, events) | Restrict probe type to `GST_PAD_PROBE_TYPE_BUFFER`; only fire on actual buffers. |
| Element setup order varies across backends | Detection runs during `SetupElement::execute()`, which fires for every element. Guard on `isAudio()` to narrow scope. |
| Source ID unavailable at first-frame time | Task reads from `GenericPlayerContext::streamInfo`; audio source must be attached before first frame – this is guaranteed by the pipeline setup sequence. |
| Probe lifetime extends past element destruction | Probe self-removes on first call (`GST_PAD_PROBE_REMOVE`); no dangling reference. |

## Migration Plan

This change is purely additive:
- No existing proto messages, interfaces, or task types are modified.
- Existing clients that do not implement `notifyFirstFrameReceived` in their `IMediaPipelineClient` will need to add a (possibly empty) override due to the pure-virtual addition – this is a binary-compatible extension under normal Rialto versioning.
- No migration of stored data or existing sessions is required.

Rollout:
1. Merge backend detection + task + server notification.
2. Merge proto change + IPC server/client layers.
3. Merge public interface change + unit and component tests.
4. Validate end-to-end on a reference backend.

## Open Questions

1. Should `firstAudioFrameReceived` be reset when the pipeline is seeked or restarted, to allow re-notification on the next play? (Conservative: no reset for now; revisit if consumers need it.)
2. For `first-audio-frame-callback`, is a GLib signal connection sufficient, or does it require a special action-signal invocation? (Assume standard `g_signal_connect` unless testing reveals otherwise.)
3. Should the probe also check that the pad is linked to a valid audio sink before installing? (Not needed – `SetupElement` already identifies audio sinks via `isAudioSink`.)
