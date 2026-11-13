## Why
Rialto already forwards structured playback metric events to `IMediaPipelineClient::notifyMetric`, but the reference-player CLI does not currently record or assert those callbacks. There is also no deterministic way for the player test to trigger the four requested metric types through the real server-to-client notification path, making end-to-end coverage unreliable.

## What Changes
- Add a reference-player CLI expectation for a named playback metric; the run passes only if that metric type is received during the test window.
- Add a reference-player fault-stimulus option that manipulates the media it feeds to provoke video frame drop, video decode error, video frame repeat, or buffer underflow in the real pipeline.
- Require the resulting metric to arrive through normal server processing, IPC, and the `notifyMetric` callback; the player must not fabricate metric callbacks.
- Preserve the established metric categories, callback shape, and legacy `notifyBufferUnderflow()` behavior.

## Impact
- Affected specs: `reference-player-metric-validation`, `reference-player-fault-injection`.
- Related existing contract: `docs/PlaybackMetrics.md`.
- Affected code: `tests/refplayer/src/frontend/cli`, `tests/refplayer/src/core/PipelineCallbacks.*`, and `tests/refplayer/src/core/MediaFeeder.*`. The existing server metric producer and IPC notification path are exercised without adding a new server injection RPC or public Rialto API.
- **Breaking changes:** None intended. CLI options and the test-only control path are additive; no production `IMediaPipeline` API change is proposed.