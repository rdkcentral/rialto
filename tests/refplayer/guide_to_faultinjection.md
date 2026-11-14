# Reference-player Fault Injection Guide

The reference player can alter its media feeding to create four bounded fault stimuli. A stimulus is not a metric event: the decoder, renderer, or Rialto metric producer must detect and report the condition. A target that does not support or report a metric will fail the corresponding expectation.

## Prerequisites

- Use a target build of `rialto_reference_player` connected to a running Rialto server. Activate the `RefPlayer` application in the server manager and set `RIALTO_SOCKET_PATH`; setup steps are in the [reference-player README](README.md#running-on-target).
- Use a target-qualified local H.264 MP4 with at least 16 video samples. The CLI targets video sample 15 for drop/decode-error and sample 8 for repeat/underflow. The repeat stimulus needs an earlier frame to have been fed.
- Run with active playback. `--inject-fault` requires `--url`, `--expect-metric`, and cannot be combined with `--no-playback`.

## Stimuli

| Fault stimulus | How the reference player triggers it | Expected metric |
|---|---|---|
| `VIDEO_FRAME_DROP` | Holds video sample 15 until playback position passes that sample's presentation deadline, then sends the sample instead of omitting it. The wait is bounded; if it times out, the sample is sent unchanged. | `VIDEO_FRAME_DROP` |
| `VIDEO_DECODE_ERROR` | Corrupts the first valid length-prefixed H.264 slice in video sample 15 by changing its NAL header before sending the access unit. The input must contain a suitable H.264 slice; otherwise feeding fails. | `VIDEO_DECODE_ERROR` |
| `VIDEO_FRAME_REPEAT` | Holds video sample 8 until playback is active and playback position reaches its PTS, with a preceding frame already fed. The renderer may reuse that preceding frame if the next frame is unavailable at presentation time. This timing stimulus does not guarantee a repeat is rendered or reported. | `VIDEO_FRAME_REPEAT` |
| `BUFFER_UNDERFLOW` | Once playback is active, pauses video sample delivery at sample 8 for two seconds. This is intended to starve decoder input, not to simulate a need-data or legacy underflow callback. | `BUFFER_UNDERFLOW` |

These actions create input conditions only. Device scheduling, decoder/renderer behavior, and platform metric support determine whether the matching event is emitted. In particular, a feeder stall is not proof that `VIDEO_FRAME_REPEAT` or `BUFFER_UNDERFLOW` occurred.

## Running a Single Scenario

Pair the stimulus with its matching expected metric:

```bash
rialto_reference_player \
  --url file:///media/metric-test.mp4 \
  --inject-fault VIDEO_FRAME_REPEAT \
  --expect-metric VIDEO_FRAME_REPEAT \
  --timeout-ms 30000
```

Use the corresponding row values to run another scenario. The CLI arms its expectation before loading and playing the media. It passes the metric check only after receiving at least one `notifyMetric` callback of the expected type before the timeout. Matching is by type, regardless of phase; the CLI does not require a particular event count or `count` field value.

For every metric callback, the CLI prints the received type, phase, source ID, optional presentation timestamp, count and duration, and observation timestamps. Absent optional values are printed as `absent`. A successful check also prints `Received expected metric <TYPE>`; a missing event, playback failure, or timeout returns a failing exit status. Legacy `notifyBufferUnderflow()` and need-data callbacks do not satisfy the metric expectation.

To run all four matching scenarios in sequence, use the [target integration helper](run_fault_metric_tests.sh). It continues after a failed case and exits unsuccessfully if any expected callback is missing.