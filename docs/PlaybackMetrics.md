# Playback Metrics IPC

## Overview

Playback metrics use two IPC flows: a synchronous snapshot query and an asynchronous event notification. Both are keyed by the media pipeline session and source IDs. Query responses contain a server monotonic timestamp; push events preserve the producer PTS.

## Westeros Bus Messages

The Broadcom Westeros sink posts `media-pipeline-metric` element messages with the fields listed in
[Bus Message Fields](#bus-message-fields). Rialto validates the message fields and enriches the event with the session and source IDs registered
for the posting element in the `ElementToSourceMapper`. If the element is not registered, Rialto logs a warning and
uses `0` for both IDs instead of rejecting the message.
Snapshot queries are restricted to the active pipeline and an attached video source. Rialto interprets occurrence (`observation-kind` 1)
video-decode-error messages as one corrupted frame. Started/resolved episode messages do not increment that counter.
Rendered and dropped frame counts are read from the sink's existing `stats` property via `getStats` when processing a
metric message and refreshing a snapshot query. Video-frame-drop messages are still forwarded with their observation kind, but
do not increment the dropped count. If sink stats are unavailable, Rialto logs a warning and retains the cached rendered
and dropped counts (zero until stats are first available).

Decoder and sink elements are registered in the `ElementToSourceMapper` when they are added to the pipeline, and are
unregistered when they are removed. The Westeros sink does not need `metrics-session-id` or `metrics-source-id`
properties.
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
- **Message type validation:** The GStreamer consumer's application-bus metric message uses signed 64-bit `pts-ns` and
    unsigned 64-bit `duration-ns` and `count` values. Validator fixtures must use those same GTypes so the tests check the message that is actually
    emitted.

The query and push mechanisms are deliberately separate. Reading the MSE sink's `frame-counters` property triggers a
synchronous `GetFrameCounters` query; Rialto does not poll that property automatically. `MediaPipelineMetricEvent` is an
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

The server sends `MediaPipelineMetricEvent` with these fields. The enums `MetricType` and `MetricPhase` are nested in
the message, and the wire numbers match the public `MetricType` and `MetricPhase` values:

| Field | Type | Meaning |
|---|---|---|
| `type` | `MetricType` enum (required) | Metric category: 1 (video frame drop) through 7 (buffer underflow), see Message Types |
| `phase` | `MetricPhase` enum (required) | Westeros observation phase: 1 occurrence, 2 episode started, 3 episode resolved |
| `source_id` | `int32` (required) | Source that produced the event |
| `pts_ns` | `int64` (optional) | Presentation timestamp (PTS) associated with the metric event, in nanoseconds |
| `count` | `uint64` (optional) | Metric repetition count |
| `duration_ns` | `uint64` (optional) | Metric duration, in nanoseconds |
| `observed_at_monotonic_ns` | `uint64` (required) | Monotonic timestamp taken by the Rialto server when the metric message was received |
| `observed_at_unix_ns` | `int64` (required) | Unix timestamp taken by the Rialto server when the metric message was received |
| `session_id` | `uint32` (optional) | Owning media pipeline session, used for routing; not part of the public `MetricEvent` |

The event carries the PTS as `pts_ns` and preserves the Westeros observation phase. Cumulative rendered/dropped/corrupted values remain on the snapshot query path and are not required in the event payload.

The public event type is `MetricEvent`:

```cpp
struct MetricEvent
{
    MetricType type{MetricType::VIDEO_FRAME_DROP};
    MetricPhase phase{MetricPhase::OCCURRENCE};
    int32_t sourceId{-1};
    std::optional<int64_t> presentationTimeStampNs;
    std::optional<uint64_t> count;
    std::optional<uint64_t> durationNs;
    uint64_t observedAtMonotonicNs{0};
    int64_t observedAtUnixNs{0};
};
```

Absent `pts_ns`, `count`, and `duration_ns` values stay absent (`std::nullopt` in `MetricEvent`, unset on the wire and
`has_*` false in the proto). They are never sent as `0`, so an absent value cannot be confused with an explicit zero.
`observed_at_monotonic_ns` and `observed_at_unix_ns` are required on the wire. The Rialto server takes them from the `GstMessage` timestamp when it receives the bus message, so they are not set by the GStreamer element or the application.

The server forwards events through the public `IMediaPipelineClient::notifyMetric` call. The server IPC client fills in its session ID in the proto message and passes the `MetricEvent` fields through unchanged. The client IPC layer subscribes to this event type when setting up the IPC channel. It dispatches events on its event thread, converts the proto message to `MetricEvent` (setting optional fields only when present), forwards events for the current session without further filtering, and passes the event to `IMediaPipelineIpcClient::notifyMetric`. `MediaPipeline` forwards that callback to the registered `IMediaPipelineClient` when it is still available.

Applications receive the same call from `IMediaPipelineClient`:

```cpp
virtual void notifyMetric(const MetricEvent &metric) = 0;
```

## Bus Message Fields

The `media-pipeline-metric` structure must be named exactly `media-pipeline-metric`. Field types are checked by
`ValidateMediaPipelineMetric`; a message that fails any check is logged and dropped without changing counters.

| Field | GType | Required | Constraints |
|---|---|---|---|
| `metric` | `G_TYPE_UINT` | Yes | 1 (video frame drop) through 7 (buffer underflow) |
| `observation-kind` | `G_TYPE_UINT` | Yes | 1 (occurrence), 2 (episode-started), or 3 (episode-resolved) |
| `pts-ns` | `G_TYPE_INT64` | No | Presentation timestamp in nanoseconds; may be zero or negative |
| `duration-ns` | `G_TYPE_UINT64` | No | Episode duration in nanoseconds |
| `count` | `G_TYPE_UINT64` | No | Repetition count; must be at least 1 |

Field semantics depend on `observation-kind`:

| `observation-kind` | Allowed optional fields |
|---|---|
| 1 (occurrence) | `pts-ns` only. `duration-ns` and `count` are rejected. |
| 2 (episode-started) | `pts-ns`. No additional restriction. |
| 3 (episode-resolved) | `pts-ns`, `duration-ns`, and `count`. For `BUFFER_UNDERFLOW` (metric 7), `count` is rejected. |

The `session_id` and `source_id` are not in the SoC payload. Rialto enriches them from `GST_MESSAGE_SRC()` through the
`ElementToSourceMapper`.

## Message Types

The protocol defines these metric categories, carried in `metric` as their 1-based numbers:

| `metric` | Category |
|---|---|
| 1 | `VIDEO_FRAME_DROP` |
| 2 | `AUDIO_FRAME_DROP` |
| 3 | `VIDEO_DECODE_ERROR` |
| 4 | `AUDIO_DECODE_ERROR` |
| 5 | `VIDEO_FRAME_REPEAT` |
| 6 | `AUDIO_GAP` |
| 7 | `BUFFER_UNDERFLOW` |

`MetricEvent::phase` carries `MetricPhase::OCCURRENCE` (1), `EPISODE_STARTED` (2), or `EPISODE_RESOLVED` (3), which correspond to the Westeros `observation-kind` values `occurrence`, `episode-started`, and `episode-resolved`. See
[Bus Message Fields](#bus-message-fields) for which optional fields each observation kind may carry.

