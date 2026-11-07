# Playback Metrics IPC

## Overview

Playback metrics use two IPC flows: a synchronous snapshot query and an asynchronous event notification. Both are keyed by the media pipeline session and source IDs. Query responses contain a server monotonic timestamp; push events preserve the producer PTS.

## Westeros Bus Messages

The Broadcom Westeros sink posts `media-pipeline-metric` element messages containing `metric_type`, `phase`, `source_id`,
`session_id`, and signed `pts_ns`. Rialto validates the message fields and matches the session ID to the active pipeline.
Snapshot queries are restricted to the active pipeline and an attached video source. Rialto interprets occurrence-phase
video-decode-error messages as one corrupted frame. Started/resolved episode messages do not increment that counter.
Rendered and dropped frame counts are read from the sink's existing `stats` property via `getStats` when processing a
metric message and refreshing a snapshot query. Video-frame-drop messages are still forwarded with their phase, but
do not increment the dropped count. If sink stats are unavailable, Rialto logs a warning and retains the cached rendered
and dropped counts (zero until stats are first available).

Rialto sets the sink's `metrics-session-id` and `metrics-source-id` properties from the active media pipeline identity.
The session ID is stored during pipeline load and the source ID during video source attachment. If the sink does not
yet exist, property assignment is deferred until the video sink's `element-setup` callback. Auto-video-sink children
are configured directly when added, including replacement children. If the video sink already exists when an ID is
assigned, its supported property is set immediately.
The Westeros message does not carry cumulative frame-counter fields; only corruption totals are accumulated by
Rialto from occurrence messages. Sink stats replace the cached rendered and dropped totals without changing the
corruption count or the last metric PTS.

## Functional Gaps and Design Approach

The implementation gaps were mismatches between the producer contract, the query response, and the GStreamer
consumer tests:

- **Source scope:** The producer identity is attached to the video sink, so `GetFrameCounters` supports an attached
    video source only. Audio metric categories may appear in the protocol, but audio snapshot queries are not supported.
    Rejecting an audio source is preferable to returning video sink statistics under an audio source ID. Supporting
    audio snapshots would require producer-side audio identity and a defined audio counter source.
- **Timestamp meaning:** A producer PTS and a server snapshot time are different values. `GetFrameCountersResponse`
    returns the Rialto server's monotonic timestamp for when the cached snapshot was updated. Push notifications retain
    the producer PTS in `pts_ns`.
- **Counter stability:** Rendered, dropped, and corrupted counters in a cached snapshot do not decrease during a
    session/source lifetime. Sink-stat refreshes and metric updates preserve the maximum observed count. Counter reset
    semantics are not defined by this contract; a reset requires a new source/session identity.
- **Message type validation:** The GStreamer consumer's application-bus metric message uses 64-bit unsigned values
    for counters and PTS. Validator fixtures must use those same GTypes so the tests check the message that is actually
    emitted.

The query and push mechanisms are deliberately separate. Reading the MSE sink's `frame-counters` property triggers a
synchronous `GetFrameCounters` query; Rialto does not poll that property automatically. `NotifyMetricEvent` is an
asynchronous push path. In the GStreamer client, the event is converted to a `media-pipeline-metric` application-bus
message for consumers that handle pipeline bus messages. Receiving a push event does not invoke the query property.

The focused regression coverage checks rejection of audio snapshot queries, preservation of counters across stale
sink-stat values, separate PTS and query timestamp values, and the GStreamer bus-message field types.

## Snapshot Query

`GetFrameCounters` takes `session_id` and `source_id`. The server looks up the latest cached snapshot for that pair and returns:

| Field | Type | Meaning |
|---|---|---|
| `rendered_frames` | `uint64` | Rendered frame count |
| `dropped_frames` | `uint64` | Dropped frame count |
| `corrupted_frames` | `uint64` | Corrupted frame count |
| `timestamp` | `uint64` | Server monotonic time when the cached snapshot was last updated, in nanoseconds |

The query timestamp is sampled from the server's monotonic clock when metric data or sink stats update the cache. It is not the producer PTS. An unknown session or non-video source, or an unavailable metrics cache, fails the RPC.

The client IPC stub is:

```cpp
bool getFrameCounters(int32_t sourceId, uint64_t &renderedFrames, uint64_t &droppedFrames,
                      uint64_t &corruptedFrames, uint64_t &timestamp);
```

It supplies its session ID and the caller-provided source ID in the request. Output references are updated only after a successful RPC.

## Push Event

The server sends `NotifyMetricEvent` with these fields:

| Field | Type | Meaning |
|---|---|---|
| `session_id` | `int32` | Owning media pipeline session |
| `source_id` | `int32` | Source that produced the snapshot |
| `rendered_frames` | `uint64` | Rendered frame count |
| `dropped_frames` | `uint64` | Dropped frame count |
| `corrupted_frames` | `uint64` | Corrupted frame count |
| `pts_ns` | `uint64` | Presentation timestamp (PTS) associated with the metric snapshot, in nanoseconds |
| `error_info` | `string` | Metric type name (for example, `METRIC_VIDEO_FRAME_DROP`) |
| `phase` | `uint32` | Westeros metric phase: 0 occurrence, 1 started, 2 resolved |

The event carries the PTS as `pts_ns` and preserves the Westeros `phase`. It does not carry the numeric `MetricType` enum field; the server maps the metric type to its string name in `error_info`.

The server-side notification signature is:

```cpp
void notifyMetricEvent(int32_t sourceId, uint64_t renderedFrames, uint64_t droppedFrames,
                       uint64_t corruptedFrames, uint64_t ptsNs, const std::string &errorInfo, uint32_t phase);
```

The server IPC client fills in its session ID, copies the remaining values into the event, and sends it. The client IPC layer subscribes to this event type when setting up the IPC channel. It dispatches events on its event thread, forwards events for the current session without further filtering, and passes all payload fields, including `pts_ns` and `phase`, to `IMediaPipelineIpcClient::notifyMetricEvent`. `MediaPipeline` forwards that callback to the registered `IMediaPipelineClient` when it is still available.

Applications receive the callback as:

```cpp
void notifyMetricEvent(int32_t sourceId, uint64_t renderedFrames, uint64_t droppedFrames,
                       uint64_t corruptedFrames, uint64_t ptsNs, const std::string &errorInfo, uint32_t phase);
```

## Message Types

The protocol defines these metric categories: `VIDEO_FRAME_DROP`, `AUDIO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `AUDIO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, `AUDIO_GAP`, and `BUFFER_UNDERFLOW`. The push event's `error_info` contains the corresponding string name prefixed with `METRIC_` (or `METRIC_UNKNOWN` for an unrecognized type); the numeric enum is not part of the payload.
