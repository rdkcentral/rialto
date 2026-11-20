# Spec Delta

## Purpose

Defines what `allSourcesAttached()` does on a pipeline with an attached Video Frame Capture, from its preconditions through HAL session creation, westerossink wiring and configuration, to configured READY, and how every failure is rolled back.

## ADDED Requirements

### Requirement: Capture commit runs synchronously before shipping setup
When a capture is attached, `allSourcesAttached()` SHALL complete the capture configuration step, including admission, before shipping setup is scheduled, and SHALL return its result to the caller. Without a capture, `allSourcesAttached()` SHALL behave exactly as before.

#### Scenario: No capture attached
- **WHEN** `allSourcesAttached()` is called on a pipeline without a capture
- **THEN** no HAL session is created and the call behaves exactly as before

#### Scenario: Successful commit
- **WHEN** every configuration step and admission succeed
- **THEN** `allSourcesAttached()` returns `true` and shipping setup is scheduled afterwards

### Requirement: Clear video source required
The capture commit SHALL fail if no video source is attached or if the video source is encrypted.

#### Scenario: Encrypted video
- **WHEN** the attached video source reports DRM
- **THEN** `allSourcesAttached()` returns `false` and no HAL session is created

#### Scenario: No video source
- **WHEN** only an audio source is attached
- **THEN** `allSourcesAttached()` returns `false`

### Requirement: westerossink is the capture sink
The capture commit SHALL fail if the `RIALTO_PLAYBIN_VIDEO_SINK` override selects a sink other than westerossink, or if westerossink cannot be created. On a secondary-video pipeline, the same westerossink instance SHALL carry both `res-usage = 0` and the capture session.

#### Scenario: Sink override
- **WHEN** `RIALTO_PLAYBIN_VIDEO_SINK` names another sink
- **THEN** `allSourcesAttached()` returns `false`

#### Scenario: Secondary video
- **WHEN** a capture pipeline is also a secondary-video pipeline
- **THEN** playbin's `video-sink` is one westerossink with `res-usage = 0` and `rdk-capture-session` set

### Requirement: Session and sink configuration order
The server SHALL create a new HAL session for the pipeline, assign the westerossink to playbin's `video-sink`, set `rdk-capture-session` to that session, and set `codec` and `resolution`, all before the sink changes state. `codec` SHALL be the fixed caps built for the attached video source, as given to appsrc. `resolution` SHALL be the string `"<maxWidth>x<maxHeight>"` from the pipeline's `VideoRequirements`.

#### Scenario: Property values
- **WHEN** an H.264 source is attached to a pipeline created with maximum size 1920x1080
- **THEN** `codec` is set to the source's `video/x-h264` caps and `resolution` to `"1920x1080"` before the sink leaves NULL

#### Scenario: Source size unknown
- **WHEN** the attached source reports width and height 0
- **THEN** `resolution` is still set from `VideoRequirements`

#### Scenario: Ten-bit source
- **WHEN** an HEVC Main10 source is attached
- **THEN** its caps are passed through unchanged in `codec`

### Requirement: READY wait with a timeout
After configuration, the server SHALL drive the sink to READY and wait for it, without enabling capture writes, for at most the configured READY timeout, which defaults to 5 seconds. A failed transition or an expired timeout SHALL fail the commit.

#### Scenario: Sink rejects the configuration
- **WHEN** the sink fails NULL→READY, for example because `resolution` is malformed
- **THEN** `allSourcesAttached()` returns `false` and everything is rolled back

#### Scenario: Sink never reaches READY
- **WHEN** the sink does not reach READY within the timeout
- **THEN** `allSourcesAttached()` returns `false` after the timeout and everything is rolled back

#### Scenario: Configured timeout
- **WHEN** the READY timeout is configured to 2 seconds
- **THEN** a sink that needs 3 seconds fails the commit

### Requirement: Rollback and retry
If any configuration step or admission fails, the server SHALL return the sink to NULL, clear the `video-sink` assignment it made, discard the sink instance that received the session, delete the HAL session only after the sink is at NULL, return `false`, and not record the commit. A later `allSourcesAttached()` on the same pipeline SHALL run the whole step again with a new session.

#### Scenario: Failure then retry
- **WHEN** admission fails, the application detaches the capture, and calls `allSourcesAttached()` again
- **THEN** the second call commits the pipeline without capture and returns `true`

#### Scenario: Retry with the capture
- **WHEN** the first commit fails because of a transient error and the application calls `allSourcesAttached()` again with the capture still attached
- **THEN** a new HAL session and a new sink instance are used, and the failed session was deleted

#### Scenario: Nothing leaks
- **WHEN** a commit fails at any step
- **THEN** no HAL session, sink reference or descriptor remains from that attempt
