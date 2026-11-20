# Media Metrics

Rialto delivers media metrics posted by the platform (SoC) pipeline to the application, and lets the application
query exact cumulative frame counters of a video source.

The producer side follows the SoC Media Metrics GStreamer Specification v0.7. The client API follows the Rialto
V1.1 proposal, section 4 (media metrics). Only the metrics part of V1.1 is implemented here.

## Producer contract (SoC Media Metrics v0.7)

Each authoritative SoC element posts a `GST_MESSAGE_ELEMENT` named `media-pipeline-metric` on its pipeline bus.

| Field | GType | Required | Meaning |
|---|---|---|---|
| `metric` | `G_TYPE_UINT` | always | 1 video frame drop, 2 audio frame drop, 3 video decode error, 4 audio decode error, 5 video frame repeat, 6 audio gap, 7 buffer underflow |
| `observation-kind` | `G_TYPE_UINT` | always | 1 occurrence, 2 episode started, 3 episode resolved |
| `pts-ns` | `G_TYPE_INT64` | optional | PTS of the affected frame, or of the episode start. Zero is a valid PTS. |
| `duration-ns` | `G_TYPE_UINT64` | resolved | Monotonic elapsed episode time from the producer (`gst_util_get_timestamp()` domain) |
| `count` | `G_TYPE_UINT64` | resolved repeat/gap | Repeated or substituted frames, greater than zero |

Rialto rejects, with a warning and without affecting playback, a message that:
- has an unknown `metric` or `observation-kind`, or a missing or wrongly typed required field;
- has an invalid shape: an occurrence type with an episode kind, an episode type with the occurrence kind, a resolved
  episode without `duration-ns`, or a resolved repeat/gap without a positive `count`;
- is a buffer underflow from an element whose class is neither `Audio` nor `Video`.

Unknown fields are ignored. Drops, decode errors, repeats and gaps are attributed to the video or audio source by
metric type; buffer underflow is attributed by the class of the producing decoder.

Rialto forwards the producer's observations unchanged. It does not merge occurrences, derive durations from PTS,
reset anything on flush or synthesise boundary resolutions; the producers post those under the v0.7 lifecycle rules.

### Bus delivery

`GstDispatcherThread` installs a bus sync handler that takes `media-pipeline-metric` messages on the posting thread
and hands them to the player worker thread (`HandleBusMessage`). The pipeline flushes its bus when it reaches the
NULL state, so taking metrics at post time keeps the final resolutions that producers post during `stop()`.

## Server processing

1. `HandleBusMessage` validates the message, builds a `MetricEvent` and stamps `observedAtMonotonicNs` (server
   steady clock) and `observedAtUnixNs` (server wall clock) when the server accepts it.
2. `MediaPipelineServerInternal::notifyMetric()` passes it to the session's `MediaMetricsQueue` on the worker thread.
   The queue filters it against the subscription and appends it to a bounded queue (256 entries). The producer
   thread never waits for socket delivery.
3. When the queue becomes non-empty, a delivery task is scheduled on the main thread. It resolves the source id from
   the attached sources and sends `notifyEventsMissed()` first, if entries were lost, then one `notifyMetric()` per
   entry in acceptance order.

On overflow the oldest queued entry is discarded and counted; the count is reported before the next delivered entry.
Entries filtered by the subscription are never counted as missed.

### Subscription

- Each pipeline starts with an empty subscription; nothing is delivered until types are selected.
- `setMetricSubscription()` atomically replaces the complete set. An empty vector disables delivery. Selecting an
  episode type enables both STARTED and RESOLVED.
- It is valid in every state except FAILURE, including before `load()`, and persists across `load()`.
- It returns false, leaving the previous set effective, for an unknown or duplicate type, a FAILURE pipeline or an
  IPC failure.
- Queued entries of removed types are discarded. A type in both sets keeps its episode state. A removed episode type
  gets no later RESOLVED.
- When an episode type is added while one of its episodes is active, a STARTED is delivered at enablement and the
  later RESOLVED duration is limited to the subscribed interval. The producer `count` is forwarded unchanged.

## Frame counters

```cpp
struct VideoFrameCounters
{
    uint32_t size{sizeof(VideoFrameCounters)};
    uint64_t renderedFrames{0};
    uint64_t droppedFrames{0};
    uint64_t corruptedFrames{0};
};

virtual bool getFrameCounters(int32_t sourceId, VideoFrameCounters &out) const;
```

The server reads `rendered`, `dropped` and `corrupted` from one read of the video sink `stats` property, so the
snapshot is coherent and uses the sink's own monotonic counters. `renderedFrames + droppedFrames` is the total of
presentation outcomes; `corruptedFrames` overlaps them.

`getFrameCounters()` returns false for a non-video or unattached source, before `allSourcesAttached()`, in FAILURE,
or when the sink or server does not return all three counters. It is independent of the subscription. The existing
`getStats()` is unchanged.

## Application API

```cpp
// IMediaPipeline
virtual bool setMetricSubscription(const std::vector<MetricType> &types);
virtual bool getFrameCounters(int32_t sourceId, VideoFrameCounters &out) const;

// IMediaPipelineClient, called on the client event thread. Defaults ignore them.
virtual void notifyMetric(const MetricEvent &metric);
virtual void notifyEventsMissed(uint32_t count);
```

```cpp
enum class MetricType : uint32_t
{
    VIDEO_FRAME_DROP, AUDIO_FRAME_DROP, VIDEO_FRAME_REPEAT, AUDIO_GAP,
    VIDEO_DECODE_ERROR, AUDIO_DECODE_ERROR, BUFFER_UNDERFLOW
};
enum class MetricPhase : uint32_t { OCCURRENCE, STARTED, RESOLVED };

struct MetricEvent
{
    MetricType type;
    MetricPhase phase;
    int32_t sourceId;
    std::optional<int64_t> presentationTimeStampNs;
    std::optional<uint64_t> count;
    std::optional<uint64_t> durationNs;
    uint64_t observedAtMonotonicNs;
    int64_t observedAtUnixNs;
};
```

| Type | Phase | Required | Optional |
|---|---|---|---|
| drops, decode errors | OCCURRENCE | — | `presentationTimeStampNs` |
| `VIDEO_FRAME_REPEAT`, `AUDIO_GAP` | STARTED | — | `presentationTimeStampNs` |
| `VIDEO_FRAME_REPEAT`, `AUDIO_GAP` | RESOLVED | `count`, `durationNs` | `presentationTimeStampNs` |
| `BUFFER_UNDERFLOW` | STARTED | — | `presentationTimeStampNs` |
| `BUFFER_UNDERFLOW` | RESOLVED | `durationNs` | `presentationTimeStampNs` |

The client library discards any other combination with a diagnostic log. Consumers increment by one per OCCURRENCE,
use RESOLVED without requiring a matching STARTED, and treat `notifyEventsMissed()` as a gap in occurrence-derived
aggregates.

`notifyBufferUnderflow()`, `notifyQos()` and `setReportDecodeErrors()` are unchanged.

## IPC

| Direction | Message | Fields |
|---|---|---|
| Client to server RPC | `setMetricSubscription(SetMediaMetricsRequest)` | `session_id = 1`, `repeated types = 2` |
| Client to server RPC | `getStats(GetStatsRequest)` | response adds `corrupted_frames = 3 [default = -1]` |
| Server to client event | `MediaMetricBatchEvent` | `session_id = 1`, `repeated metrics = 2`, `missed_count = 3` |
| Batch entry | `MediaMetricBatchEvent.MetricEvent` | `type = 1`, `phase = 2`, `source_id = 3`, `pts_ns = 4`, `count = 5`, `duration_ns = 6`, `observed_at_monotonic_ns = 7`, `observed_at_unix_ns = 8` |

`MetricType` and `MetricPhase` are protobuf enums nested in `MediaMetricBatchEvent`, with `0` as unknown and the
public enum value plus one on the wire. They are nested to avoid clashing with the public C++ types of the same
name. Optional fields distinguish zero from absence. Each server event currently carries one entry; the client
handles any number of entries in order.

When client and server versions differ:
- an older server does not know `setMetricSubscription` and omits `corrupted_frames`, so those calls return false;
- an older client never subscribes, so it never receives `MediaMetricBatchEvent`.

## Not covered

- Final resolutions posted while the player is destroyed without `stop()` are lost, because the dispatcher is
  destroyed before the pipeline is torn down.
- Subscribing to decode errors does not yet enable the platform decoder's error reporting; `setReportDecodeErrors()`
  still controls it.
- Video Frame Capture and the other non-metrics parts of Rialto V1.1.
