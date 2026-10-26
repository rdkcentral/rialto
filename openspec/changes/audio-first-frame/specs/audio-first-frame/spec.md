## ADDED Requirements

### Requirement: Audio decoder first-frame detection
The system SHALL detect audio first-frame support on audio decoder elements by checking for the `first-audio-frame` capability during element setup.

#### Scenario: Audio decoder exposes first-frame capability
- **WHEN** an audio decoder element exposes `first-audio-frame` during setup
- **THEN** the system connects that capability for the audio source
- **THEN** the first detected audio frame is scheduled through the existing first-frame handling path

### Requirement: Audio sink first-frame fallback
The system SHALL detect audio first-frame support on audio sink elements by using `first-audio-frame-callback` when available and SHALL install a sink pad probe only when that callback capability is unavailable.

#### Scenario: Audio sink exposes callback capability
- **WHEN** an audio sink element exposes `first-audio-frame-callback` during setup
- **THEN** the system uses that callback capability for first-frame detection
- **THEN** no fallback sink pad probe is installed for that sink

#### Scenario: Audio sink does not expose callback capability
- **WHEN** an audio sink element does not expose `first-audio-frame-callback` during setup
- **THEN** the system installs a sink pad probe for that sink
- **THEN** the first valid audio buffer observed by the probe triggers first-frame handling
- **THEN** the probe is removed immediately after the first valid audio buffer is observed

### Requirement: Single audio first-frame notification
The system MUST emit exactly one first-frame notification for each relevant audio source lifecycle even if both capability-based and probe-based detection paths become active.

#### Scenario: Multiple detection paths observe the same first frame
- **WHEN** capability-based detection and fallback probe detection both observe the same audio source lifecycle
- **THEN** the system emits only one first-frame notification for that audio source

#### Scenario: Audio source ends before first frame
- **WHEN** an audio source is flushed, reset, torn down, or the pipeline stops before the first audio frame is emitted
- **THEN** the system removes any installed fallback probe without emitting a duplicate or stale first-frame notification

### Requirement: Existing first-frame contract reuse
The system SHALL propagate audio first-frame notifications through the existing first-frame pipeline using `MediaSourceType::AUDIO` without introducing a new public callback or protobuf event.

#### Scenario: Audio first frame reaches the client
- **WHEN** the system detects the first frame for an audio source
- **THEN** it forwards the event through the existing worker-thread, server, IPC, and client callback path
- **THEN** the client receives the existing `notifyFirstFrameReceived(sourceId)` callback for the audio source

#### Scenario: Existing protocol remains unchanged
- **WHEN** audio first-frame support is added
- **THEN** the system continues to use the existing `FirstFrameReceivedEvent` transport shape
- **THEN** no new public callback name or protobuf message is required