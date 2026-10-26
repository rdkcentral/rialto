# Playback Metrics IPC

## Overview

Playback metrics use two IPC flows: a synchronous snapshot query and an asynchronous event notification. Both are keyed by the media pipeline session and source IDs. The IPC layer passes counter values through without aggregation.

## Snapshot Query

`GetFrameCounters` takes `session_id` and `source_id`. The server looks up the latest cached snapshot for that pair and returns:

| Field | Type | Meaning |
|---|---|---|
| `rendered_frames` | `uint64` | Rendered frame count |
| `dropped_frames` | `uint64` | Dropped frame count |
| `corrupted_frames` | `uint64` | Corrupted frame count |
| `timestamp` | `uint64` | Snapshot timestamp in nanoseconds |

An unknown session or source, or an unavailable metrics cache, fails the RPC. The timestamp is returned unchanged; the service does not convert its units.

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
| `error_info` | `string` | Optional error details |

`NotifyMetricEvent` has no timestamp field. The `MetricType` enum is defined in the protocol but is not included in this event payload.

The server-side notification signature is:

```cpp
void notifyMetricEvent(int32_t sourceId, uint64_t renderedFrames, uint64_t droppedFrames,
                       uint64_t corruptedFrames, const std::string &errorInfo);
```

The server IPC client fills in its session ID, copies the remaining values into the event, and sends it. The client IPC layer subscribes to this event type when setting up the IPC channel. It dispatches events on its event thread, forwards events for the current session without further filtering, and passes all payload fields to `IMediaPipelineIpcClient::notifyMetricEvent`. `MediaPipeline` forwards that callback to the registered `IMediaPipelineClient` when it is still available.

Applications receive the callback as:

```cpp
void notifyMetricEvent(int32_t sourceId, uint64_t renderedFrames, uint64_t droppedFrames,
                       uint64_t corruptedFrames, const std::string &errorInfo);
```

## Message Types

The protocol defines these metric categories: `VIDEO_FRAME_DROP`, `AUDIO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `AUDIO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, `AUDIO_GAP`, and `BUFFER_UNDERFLOW`. The current query and push payloads carry counter snapshots; they do not include a metric category field.
