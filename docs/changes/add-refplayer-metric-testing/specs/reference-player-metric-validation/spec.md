## ADDED Requirements
### Requirement: Reference-player CLI validates expected playback metrics
The reference-player CLI SHALL accept `--expect-metric <type>` for `VIDEO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, or `BUFFER_UNDERFLOW`. When this option is supplied with a media URL, the CLI SHALL pass only if at least one `notifyMetric` callback with the requested type is received during the metric wait window; otherwise it SHALL exit unsuccessfully. Matching is by type, independent of phase.

#### Scenario: Expected metric is received
- **WHEN** a playback run requests an expected metric type and a matching `notifyMetric` event arrives before the timeout
- **THEN** the CLI records the event and may complete successfully if its other checks pass

#### Scenario: Expected metric is absent
- **WHEN** playback completes or the metric wait times out without a matching event
- **THEN** the CLI reports the missing type and exits unsuccessfully

#### Scenario: Other metric type arrives
- **WHEN** one or more metric callbacks arrive but none has the requested type
- **THEN** those callbacks do not satisfy the expectation and the run fails if no matching event arrives before timeout

#### Scenario: Invalid CLI input
- **WHEN** the requested type is unsupported or `--expect-metric` is used without a media URL
- **THEN** the CLI reports an argument error and exits unsuccessfully before connecting to Rialto

### Requirement: Reference-player receives metrics through the public callback
The reference player SHALL observe playback metric events through `IMediaPipelineClient::notifyMetric` and SHALL NOT treat `notifyQos`, `notifyBufferUnderflow`, or need-data callbacks as substitutes for a metric event.

#### Scenario: Metric callback is delivered
- **WHEN** Rialto delivers a `MetricEvent` to the active pipeline client
- **THEN** the reference player records its type and preserves the event for expectation matching

#### Scenario: Legacy underflow callback occurs without a metric
- **WHEN** `notifyBufferUnderflow` is called but no `BUFFER_UNDERFLOW` `notifyMetric` event is delivered
- **THEN** the `BUFFER_UNDERFLOW` expectation remains unsatisfied