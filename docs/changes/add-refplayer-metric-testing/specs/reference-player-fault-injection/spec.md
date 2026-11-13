## ADDED Requirements
### Requirement: Reference player creates controlled pipeline fault stimuli
The reference-player CLI SHALL accept `--inject-fault <type>` for `VIDEO_FRAME_DROP`, `VIDEO_DECODE_ERROR`, `VIDEO_FRAME_REPEAT`, or `BUFFER_UNDERFLOW` during active playback. It SHALL create the stimulus by modifying media-feed timing or one encoded access unit, not by fabricating a `MetricEvent` or directly invoking `notifyMetric`. The run SHALL use `--expect-metric <type>` to determine whether the real pipeline emitted the corresponding metric.

#### Scenario: Stimulate a presentation-deadline drop
- **WHEN** the player injects `VIDEO_FRAME_DROP`
- **THEN** it delays a valid video sample past its presentation deadline rather than omitting the sample before Rialto receives it

#### Scenario: Stimulate a decoder error
- **WHEN** the player injects `VIDEO_DECODE_ERROR`
- **THEN** it corrupts one selected encoded video access unit and leaves metric production to the authoritative decoder and server path

#### Scenario: Stimulate a renderer repeat
- **WHEN** the player injects `VIDEO_FRAME_REPEAT`
- **THEN** it stalls delivery of the next video sample while the preceding frame is available, without treating structured cadence such as 3:2 or 2:2 as a fault

#### Scenario: Stimulate decoder input starvation
- **WHEN** the player injects `BUFFER_UNDERFLOW` during active playback
- **THEN** it pauses feeder delivery long enough to create decoder input starvation, independently of need-data flow control and `notifyBufferUnderflow()`

#### Scenario: Stimulus does not produce a metric
- **WHEN** the target pipeline does not detect or report the requested fault
- **THEN** the expected-metric check fails and the CLI does not report a successful metric test

#### Scenario: Normal playback
- **WHEN** no fault stimulus is requested
- **THEN** the feeder preserves its normal timing and sample data

### Requirement: Fault stimuli preserve metric ownership
The server-side producer SHALL decide whether a real pipeline condition qualifies as a playback metric and SHALL deliver such events through the existing notification path. Reference-player fault stimuli SHALL NOT be interpreted as proof that a metric occurred.

#### Scenario: Pipeline reports a qualifying fault
- **WHEN** the decoder or renderer detects and reports a qualifying fault after a player stimulus
- **THEN** Rialto validates and forwards the event through the existing IPC path to `IMediaPipelineClient::notifyMetric`

#### Scenario: Stimulus resembles an excluded frame removal
- **WHEN** a video sample is removed by seek, source flush, EOS cleanup, or source reconfiguration
- **THEN** that removal is not classified as `VIDEO_FRAME_DROP`