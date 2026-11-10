# Playback Metrics

Rialto relays playback metrics posted by the video sink to the application, and lets the application query the frame
counters of a video source.

## Producer contract

The video sink posts a `GST_MESSAGE_ELEMENT` message named `media-pipeline-metric` on the pipeline bus:

| Field | GType | Meaning |
|---|---|---|
| `metric_type` | `guint` | 0 video frame drop, 1 audio frame drop, 2 video decode error, 3 audio decode error, 4 video frame repeat, 5 audio gap, 6 buffer underflow |
| `phase` | `guint` | 0 occurrence, 1 started, 2 resolved |
| `pts_ns` | `gint64` | PTS of the frame the metric refers to, in nanoseconds |

Other fields, such as `session_id` and `source_id`, are ignored. Each media pipeline has its own bus, so the session
is known, and the metric is attributed to the attached video source.

A message is ignored when:
- it is not an element message named `media-pipeline-metric`,
- it was not posted by an element whose class contains `Sink` and `Video`,
- a field is missing, `phase` is greater than 2 or `pts_ns` is negative.

An unknown `metric_type` is forwarded as `PlaybackMetricType::UNKNOWN`.

## Server processing

`GstDispatcherThread` passes the message to the player worker thread, where `HandleBusMessage` parses it and hands
it to `PlaybackMetricsTracker`:

- **Occurrence:** counted. A video decode error occurrence also increases the corrupted frame count. To protect the
  IPC channel during bursts, an occurrence of a type is forwarded at most once per second; occurrences in between are
  merged and the next forwarded notification reports them in `count`.
- **Started:** the PTS is stored and the notification is forwarded.
- **Resolved:** `durationNs` is the PTS difference to the matching started notification (0 if there was none, or if
  the PTS went backwards) and the notification is forwarded.

The tracker is reset when the video source is attached or flushed, which clears the corrupted frame count, pending
occurrences and open episodes.

`MediaPipelineServerInternal` adds the current frame counters of the source and sends the notification to the client.

## Frame counters

```cpp
struct FrameCounters
{
    uint64_t renderedFrames;  // From the sink's "stats" property
    uint64_t droppedFrames;   // From the sink's "stats" property
    uint64_t corruptedFrames; // Video decode errors since the source was attached or last flushed
};
```

`renderedFrames` and `droppedFrames` are the values the sink reports, unchanged. They follow the sink's own reset
behaviour, for example a sink may reset them on flush.

## Application API

Both methods have default implementations, so existing clients and pipelines build without changes.

```cpp
// IMediaPipelineClient: called on the client event thread. The default implementation ignores it.
virtual void notifyPlaybackMetric(int32_t sourceId, const PlaybackMetric &metric) {}

// IMediaPipeline: synchronous. Only video sources are supported.
virtual bool getFrameCounters(int32_t sourceId, FrameCounters &frameCounters);
```

```cpp
struct PlaybackMetric
{
    PlaybackMetricType type;
    PlaybackMetricPhase phase;
    uint64_t ptsNs;
    uint64_t count;              // Events covered by this notification
    uint64_t durationNs;         // RESOLVED only
    FrameCounters frameCounters; // When the notification was sent
};
```

`getFrameCounters` fails if the pipeline is not loaded, the source is not the attached video source, or the sink
stats are unavailable.

## IPC

| Direction | Message | Fields |
|---|---|---|
| Server to client event | `PlaybackMetricEvent` | `session_id`, `source_id`, `type`, `phase`, `pts_ns`, `count`, `duration_ns`, `rendered_frames`, `dropped_frames`, `corrupted_frames` |
| Client to server RPC | `getFrameCounters(GetFrameCountersRequest)` | request: `session_id`, `source_id`; response: `rendered_frames`, `dropped_frames`, `corrupted_frames` |

`type` and `phase` are protobuf enums with the same values as `PlaybackMetricType` and `PlaybackMetricPhase`.

When the client and server versions differ:
- an older server does not know `getFrameCounters`, so the call fails and the client returns false,
- an older client has not subscribed to `PlaybackMetricEvent`, so the event is not delivered.
