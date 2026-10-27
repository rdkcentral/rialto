# Playback Metrics IPC

## Overview

Playback metrics use two IPC flows: a synchronous snapshot query and an asynchronous event notification. Both are keyed by the media pipeline session and source IDs. The IPC layer passes counter values and timestamps through without aggregation.

## Snapshot Query

`GetFrameCounters` takes `session_id` and `source_id`. The server looks up the latest cached snapshot for that pair and returns:

| Field | Type | Meaning |
|---|---|---|
| `rendered_frames` | `uint64` | Rendered frame count |
| `dropped_frames` | `uint64` | Dropped frame count |
| `corrupted_frames` | `uint64` | Corrupted frame count |
| `timestamp` | `uint64` | Presentation timestamp (PTS) associated with the cached snapshot, in nanoseconds |

The timestamp is taken from the metric message's `pts_ns` value and returned unchanged; the service does not convert its units. An unknown session or source, or an unavailable metrics cache, fails the RPC.

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

The event carries the PTS as `pts_ns`. It does not carry the numeric `MetricType` enum field; the server maps the metric type to its string name in `error_info`.

The server-side notification signature is:

```cpp
void notifyMetricEvent(int32_t sourceId, uint64_t renderedFrames, uint64_t droppedFrames,
                       uint64_t corruptedFrames, uint64_t ptsNs, const std::string &errorInfo);
```

The server IPC client fills in its session ID, copies the remaining values into the event, and sends it. The client IPC layer subscribes to this event type when setting up the IPC channel. It dispatches events on its event thread, forwards events for the current session without further filtering, and passes all payload fields, including `pts_ns`, to `IMediaPipelineIpcClient::notifyMetricEvent`. `MediaPipeline` forwards that callback to the registered `IMediaPipelineClient` when it is still available.

Applications receive the callback as:

```cpp
void notifyMetricEvent(int32_t sourceId, uint64_t renderedFrames, uint64_t droppedFrames,
                       uint64_t corruptedFrames, uint64_t ptsNs, const std::string &errorInfo);
```

## Message Types

The protocol defines these metric categories: `VIDEO_FRAME_DROP`, `AUDIO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `AUDIO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, `AUDIO_GAP`, and `BUFFER_UNDERFLOW`. The push event's `error_info` contains the corresponding string name prefixed with `METRIC_` (or `METRIC_UNKNOWN` for an unrecognized type); the numeric enum is not part of the payload.
