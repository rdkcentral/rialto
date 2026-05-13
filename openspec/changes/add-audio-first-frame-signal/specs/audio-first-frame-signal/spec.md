## ADDED Requirements

### Requirement: First-Audio-Frame Signal Detection

Rialto SHALL detect the first audio frame received by the playback pipeline using a platform-agnostic, capability-based strategy. During element setup for an audio element, Rialto SHALL inspect the element's supported GLib signals by iterating over the signal list and checking for the names `first-audio-frame` and `first-audio-frame-callback`. If a matching signal name is found, Rialto SHALL connect a callback to that signal; this callback SHALL schedule an internal `FirstFrameReceived` task on the Rialto worker task queue without performing any heavy work inline on the GStreamer thread.

#### Scenario: Audio element exposes first-audio-frame signal

- **WHEN** `SetupElement` executes for an audio decoder or sink element that exposes a GLib signal named `first-audio-frame`
- **THEN** Rialto SHALL connect a callback to that signal, and no pad probe SHALL be installed on the element's sink pad

#### Scenario: Audio element exposes first-audio-frame-callback signal

- **WHEN** `SetupElement` executes for an audio decoder or sink element that exposes a GLib signal named `first-audio-frame-callback`
- **THEN** Rialto SHALL connect a callback to that signal, and no pad probe SHALL be installed on the element's sink pad

#### Scenario: No supported first-frame signal is available

- **WHEN** `SetupElement` executes for an audio sink element and neither `first-audio-frame` nor `first-audio-frame-callback` is present in the element's signal list
- **THEN** Rialto SHALL install a static buffer pad probe on the audio sink's sink pad to detect the first audio frame

#### Scenario: Detection is limited to audio elements

- **WHEN** `SetupElement` executes for a non-audio element (e.g., a video sink or video decoder)
- **THEN** Rialto SHALL NOT connect any first-audio-frame signal or install any first-audio-frame pad probe for that element

---

### Requirement: Fallback Pad Probe Lifecycle

When installed as a fallback, the first-audio-frame pad probe SHALL be a buffer-type probe (`GST_PAD_PROBE_TYPE_BUFFER`) and SHALL be self-removing. After the probe fires for the first valid audio buffer, it SHALL return `GST_PAD_PROBE_REMOVE` so that the probe is unregistered immediately. The probe SHALL NOT remain active after the first detection.

#### Scenario: Probe fires and is removed on first buffer

- **WHEN** the fallback pad probe callback is invoked for the first audio buffer on the probed pad
- **THEN** the probe SHALL schedule a `FirstFrameReceived` task on the Rialto worker task queue AND return `GST_PAD_PROBE_REMOVE` to unregister itself

#### Scenario: Probe does not fire on subsequent buffers

- **WHEN** the fallback pad probe has already removed itself after the first audio buffer
- **THEN** subsequent audio buffers SHALL pass through the pad without any first-frame probe activity

---

### Requirement: FirstFrameReceived Task Scheduling

Detection callbacks (signal-based or probe-based) SHALL NOT perform notification logic inline on the GStreamer thread. Instead, they SHALL call `IGstGenericPlayerPrivate::scheduleAudioFirstFrame()`, which SHALL enqueue a `FirstFrameReceived` task on the Rialto worker task queue.

#### Scenario: Signal callback schedules task on worker queue

- **WHEN** the first-audio-frame signal callback is invoked on the GStreamer thread
- **THEN** `IGstGenericPlayerPrivate::scheduleAudioFirstFrame()` SHALL be called, which SHALL enqueue a `FirstFrameReceived` task without executing notification logic on the GStreamer thread

#### Scenario: Probe callback schedules task on worker queue

- **WHEN** the fallback pad probe callback is invoked on the GStreamer thread
- **THEN** `IGstGenericPlayerPrivate::scheduleAudioFirstFrame()` SHALL be called, which SHALL enqueue a `FirstFrameReceived` task without executing notification logic on the GStreamer thread

---

### Requirement: First-Frame Deduplication

The first-audio-frame notification SHALL be emitted at most once per audio source per session. The `FirstFrameReceived` task SHALL check a per-source guard flag before notifying the client. If the flag is already set, the task SHALL be a no-op.

#### Scenario: First-frame task fires for the first time

- **WHEN** the `FirstFrameReceived` task executes and the guard flag for the audio source is not yet set
- **THEN** the task SHALL set the guard flag and proceed to notify the client via `IGstGenericPlayerClient::notifyFirstFrameReceived(sourceId)`

#### Scenario: First-frame task fires again (duplicate)

- **WHEN** the `FirstFrameReceived` task executes and the guard flag for the audio source is already set
- **THEN** the task SHALL be a no-op and SHALL NOT call `notifyFirstFrameReceived` again

---

### Requirement: Server-Side First-Frame Propagation

`MediaPipelineServerInternal` SHALL implement `IGstGenericPlayerClient::notifyFirstFrameReceived(int32_t sourceId)`. When invoked, it SHALL enqueue a task on the main thread to call `IMediaPipelineClient::notifyFirstFrameReceived(int32_t sourceId)`.

#### Scenario: Server propagates first-frame notification to IPC client

- **WHEN** `MediaPipelineServerInternal::notifyFirstFrameReceived(sourceId)` is called
- **THEN** a task SHALL be enqueued on the main thread that calls `m_mediaPipelineClient->notifyFirstFrameReceived(sourceId)`

---

### Requirement: IPC First-Frame Event

The server IPC layer (`server::ipc::MediaPipelineClient`) SHALL implement `IMediaPipelineClient::notifyFirstFrameReceived(int32_t sourceId)`. When invoked, it SHALL construct and send a `FirstFrameReceivedEvent` protobuf message containing `session_id` and `source_id`.

The `FirstFrameReceivedEvent` message SHALL be defined in `mediapipelinemodule.proto` as:

```proto
message FirstFrameReceivedEvent {
    optional int32 session_id = 1 [default = -1];
    optional int32 source_id  = 2 [default = -1];
}
```

#### Scenario: Server IPC sends FirstFrameReceivedEvent

- **WHEN** `server::ipc::MediaPipelineClient::notifyFirstFrameReceived(sourceId)` is called
- **THEN** a `FirstFrameReceivedEvent` SHALL be sent to the IPC client with `session_id` set to the active session ID and `source_id` set to the given source ID

---

### Requirement: Client IPC First-Frame Handler

The client IPC layer (`MediaPipelineIpc`) SHALL subscribe to `FirstFrameReceivedEvent`. When a `FirstFrameReceivedEvent` is received, it SHALL verify the `session_id` matches the active session and then call `IMediaPipelineIpcClient::notifyFirstFrameReceived(source_id)`.

#### Scenario: Client IPC receives and dispatches FirstFrameReceivedEvent for this session

- **WHEN** a `FirstFrameReceivedEvent` is received with `session_id` matching the active session
- **THEN** `IMediaPipelineIpcClient::notifyFirstFrameReceived(source_id)` SHALL be called with the `source_id` from the event

#### Scenario: Client IPC ignores event for a different session

- **WHEN** a `FirstFrameReceivedEvent` is received with `session_id` that does not match the active session
- **THEN** the event SHALL be silently ignored and no client callback SHALL be invoked

---

### Requirement: Public Client First-Frame Callback

The public `IMediaPipelineClient` interface SHALL declare:

```cpp
virtual void notifyFirstFrameReceived(int32_t sourceId) = 0;
```

Applications implementing `IMediaPipelineClient` SHALL receive this callback when the first audio frame is detected for the source identified by `sourceId`.

#### Scenario: Application receives first-frame notification with correct source ID

- **WHEN** the first audio frame is detected for an attached audio source
- **THEN** `IMediaPipelineClient::notifyFirstFrameReceived(sourceId)` SHALL be called with the `sourceId` of the audio source that produced the first frame

#### Scenario: Existing playback events are unaffected

- **WHEN** the first-audio-frame feature is active
- **THEN** all existing `IMediaPipelineClient` callbacks (playback state, underflow, QoS, etc.) SHALL continue to behave as before and SHALL NOT be affected by the addition of `notifyFirstFrameReceived`
